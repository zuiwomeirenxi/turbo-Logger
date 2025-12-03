#include "turbolog/Logger.h"
#include <iostream>
#include <cstdlib>
#include <csignal>
#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>
#endif

namespace turbolog {

Logger::Logger()
    : _ofs()
    , _level(LogLevel::DEBUG)
    , running_(true)
    , current_buffer_(std::make_shared<LogBuffer<DefaultBufferSize>>())
    , next_buffer_(std::make_shared<LogBuffer<DefaultBufferSize>>())
{
    worker_thread_ = std::thread([this]{ threadFunc(); });
}

Logger::~Logger() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
        if (current_buffer_ && current_buffer_->length() > 0) {
            buffers_queue_.push_back(current_buffer_);
            current_buffer_ = std::make_shared<LogBuffer<DefaultBufferSize>>();
        }
        cond_.notify_all();
    }
    if (worker_thread_.joinable()) worker_thread_.join();
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::append(const char* data, std::size_t len) {
    if (len == 0) return;
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_buffer_->avail() >= len) {
        current_buffer_->append(data, len);
        return;
    }
    // Buffer full: push current to queue
    buffers_queue_.push_back(current_buffer_);
    // 双缓冲切换：优先复用 next_buffer_
    if (next_buffer_) {
        current_buffer_ = std::move(next_buffer_);
    } else {
        current_buffer_ = std::make_shared<LogBuffer<DefaultBufferSize>>();
    }
    // next_buffer_ 置空，等待后台线程回收
    next_buffer_.reset();
    current_buffer_->append(data, len);
    cond_.notify_one();
}

void Logger::threadFunc() {
    std::shared_ptr<LogBuffer<DefaultBufferSize>> newPtr1 = std::make_shared<LogBuffer<DefaultBufferSize>>();
    std::shared_ptr<LogBuffer<DefaultBufferSize>> newPtr2 = std::make_shared<LogBuffer<DefaultBufferSize>>();
    std::vector<std::shared_ptr<LogBuffer<DefaultBufferSize>>> buffers_to_write;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::seconds(3), [this]{ return !buffers_queue_.empty() || !running_; });
            if (current_buffer_ && current_buffer_->length() > 0) {
                buffers_queue_.push_back(current_buffer_);
                current_buffer_ = nullptr;
            }
            if (!current_buffer_) {
                current_buffer_ = newPtr1;
                newPtr1.reset();
            }
            if (!next_buffer_) {
                next_buffer_ = newPtr2;
                newPtr2.reset();
            }
            buffers_queue_.swap(buffers_to_write);
            if (!running_ && buffers_to_write.empty()) break;
        }
        // IO区：写盘并处理滚动
        for (const auto& buf : buffers_to_write) {
            if (buf && buf->length() > 0 && _ofs.is_open()) {
                _ofs.write(buf->data(), static_cast<std::streamsize>(buf->length()));
                written_bytes_ += buf->length();
            }
        }
        if (_ofs.is_open()) _ofs.flush();
        // 日志滚动：超出 roll_size_ 时重命名并新建
        if (written_bytes_ > roll_size_ && _ofs.is_open() && !log_file_name_.empty()) {
            _ofs.flush();
            _ofs.close();
            // 生成带时间戳的新文件名
            char ts[32];
            std::time_t t = std::time(nullptr);
            std::tm tm;
#if defined(_WIN32)
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            std::snprintf(ts, sizeof(ts), ".%04d%02d%02d-%02d%02d%02d",
                tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            std::string new_name = log_file_name_ + ts;
            std::rename(log_file_name_.c_str(), new_name.c_str());
            _ofs.open(log_file_name_, std::ios::out | std::ios::app | std::ios::binary);
            written_bytes_ = 0;
        }
        // 回收缓冲，防止内存爆炸
        if (buffers_to_write.size() > 2) buffers_to_write.resize(2);
        if (!newPtr1 && !buffers_to_write.empty()) {
            newPtr1 = buffers_to_write.back();
            buffers_to_write.pop_back();
        }
        if (!newPtr2 && !buffers_to_write.empty()) {
            newPtr2 = buffers_to_write.back();
            buffers_to_write.pop_back();
        }
        if (newPtr1) newPtr1->reset();
        if (newPtr2) newPtr2->reset();
        buffers_to_write.clear();
    }
}

void Logger::flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (current_buffer_ && current_buffer_->length() > 0) {
        buffers_queue_.push_back(current_buffer_);
        current_buffer_ = std::make_shared<LogBuffer<DefaultBufferSize>>();
    }
    cond_.notify_one();
}

void Logger::set_roll_size(off_t sz) {
    roll_size_ = sz;
}

void Logger::set_log_file(const std::string& name) {
    log_file_name_ = name;
}

void Logger::init(const std::string& filename) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (_ofs.is_open()) return;
    _ofs.open(filename, std::ios::out | std::ios::app | std::ios::binary);
    log_file_name_ = filename;
    std::signal(SIGSEGV, crashHandler);
}

void Logger::Log(LogLevel level, const char* file, int line, const char* format, ...) {
    // Fast path: format outside lock, append via append()
    using namespace std::chrono;
    auto now = system_clock::now();
    auto us = duration_cast<microseconds>(now.time_since_epoch());
    auto s = duration_cast<seconds>(us);
    std::time_t tt = s.count();
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    int micro = static_cast<int>((us - s).count());

    char ts[64];
    std::snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec, micro);

    std::array<char, DefaultBufferSize> msgbuf{};
    va_list args;
    va_start(args, format);
    int n = std::vsnprintf(msgbuf.data(), msgbuf.size(), format, args);
    va_end(args);
    if (n < 0) return;
    std::size_t msglen = static_cast<std::size_t>(n);
    if (msglen >= msgbuf.size()) msglen = msgbuf.size() - 1;

    const char* lvlstr = LogLevelToString(level);
    char header[256];
    int hn = std::snprintf(header, sizeof(header), "[%s] [%s] %s:%d ", ts, lvlstr, file, line);
    if (hn < 0) return;
    std::size_t hlen = static_cast<std::size_t>(hn);
    if (hlen >= sizeof(header)) hlen = sizeof(header) - 1;

    append(header, hlen);
    append(msgbuf.data(), msglen);
    const char nl = '\n';
    append(&nl, 1);
}

void crashHandler(int signal) {
    std::cerr << "\n[TurboLog] Caught signal " << signal << ", flushing logs...\n";
    Logger::instance().flush();
#if defined(__linux__) || defined(__APPLE__)
    void* array[32];
    int size = backtrace(array, 32);
    char** symbols = backtrace_symbols(array, size);
    std::cerr << "[TurboLog] Stack trace:" << std::endl;
    for (int i = 0; i < size; ++i) {
        std::cerr << symbols[i] << std::endl;
    }
    free(symbols);
#endif
    std::cerr << "[TurboLog] Abort.\n";
    std::abort();
}

} // namespace turbolog

// Current implementation is header-only style for performance and simplicity.
// This .cpp is provided to keep translation unit structure ready for future expansions.
