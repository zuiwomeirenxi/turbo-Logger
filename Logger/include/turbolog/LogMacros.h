#pragma once

#include "turbolog/Logger.h"

// Macros that check level before formatting to avoid unnecessary cost.
// Usage: LOG_INFO("Connected from %s", ip);

#define LOG_DEBUG(fmt, ...) \
    do { \
        if (::turbolog::Logger::instance().isEnabled(::turbolog::LogLevel::DEBUG)) \
            ::turbolog::Logger::instance().Log(::turbolog::LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define LOG_INFO(fmt, ...) \
    do { \
        if (::turbolog::Logger::instance().isEnabled(::turbolog::LogLevel::INFO)) \
            ::turbolog::Logger::instance().Log(::turbolog::LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define LOG_WARN(fmt, ...) \
    do { \
        if (::turbolog::Logger::instance().isEnabled(::turbolog::LogLevel::WARN)) \
            ::turbolog::Logger::instance().Log(::turbolog::LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

#define LOG_ERROR(fmt, ...) \
    do { \
        if (::turbolog::Logger::instance().isEnabled(::turbolog::LogLevel::ERROR)) \
            ::turbolog::Logger::instance().Log(::turbolog::LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)
