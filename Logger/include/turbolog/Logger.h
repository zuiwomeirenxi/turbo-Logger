#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <chrono>
#include <array>
#include <thread>
#include <condition_variable>
#include <vector>
#include <memory>
#include <mutex>
#include <sys/types.h>

#include "turbolog/LogBuffer.h"

namespace turbolog {

enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

inline const char* LogLevelToString(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "INFO";
    }
}

void crashHandler(int signal);

class Logger {
public:
    static constexpr int DefaultBufferSize = 4096;

    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    static Logger& instance();

    void init(const std::string& filename);
    void setLevel(LogLevel lvl) { _level = lvl; }
    LogLevel level() const { return _level; }
    bool isEnabled(LogLevel lvl) const { return static_cast<int>(lvl) >= static_cast<int>(_level); }

    void Log(LogLevel level, const char* file, int line, const char* format, ...);
    void flush();
    void set_roll_size(off_t sz);
    void set_log_file(const std::string& name);
    void stop();

private:
    void append(const char* data, std::size_t len);
    void threadFunc();

    std::ofstream _ofs;
    LogLevel _level { LogLevel::DEBUG };

    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread worker_thread_;
    bool running_ {false};
    std::shared_ptr<LogBuffer<DefaultBufferSize>> current_buffer_;
    std::shared_ptr<LogBuffer<DefaultBufferSize>> next_buffer_;
    std::vector<std::shared_ptr<LogBuffer<DefaultBufferSize>>> buffers_queue_;

    off_t written_bytes_ = 0;
    off_t roll_size_ = 100 * 1024 * 1024;
    std::string log_file_name_;
};

} // namespace turbolog
