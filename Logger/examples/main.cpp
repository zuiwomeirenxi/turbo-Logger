#include <iostream>
#include <chrono>
#include <string>
#include "turbolog/LogMacros.h"

int main() {
    using namespace turbolog;

    Logger::instance().set_log_file("test.log");
    Logger::instance().set_roll_size(1 * 1024 * 1024); // 1MB for test
    Logger::instance().init("test.log");
    Logger::instance().setLevel(LogLevel::INFO);

    constexpr int N = 120000; // 足够写满5MB以上
    std::string bigmsg(80, 'X'); // 每行约100字节

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        LOG_INFO("Rolling test line %d: %s", i, bigmsg.c_str());
    }
    auto t1 = std::chrono::steady_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double per_line_us = (double)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / N;

    std::cout << "Wrote " << N << " lines to test.log in " << ms << " ms" << std::endl;
    std::cout << "Avg per line: " << per_line_us << " us" << std::endl;
    std::cout << "Check for test.log and test.log.* files (should see ~5+ files of ~1MB each)." << std::endl;
    return 0;
}
