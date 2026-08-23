/*

SPDX-License-Identifier: MIT

Copyright (c) 2026 oldmnj oldmnj@163.com

This is the core kernel module of the launcher.

For license details, see the LICENSE file in the root directory.
*/
module;

#include <atomic>
#include <chrono>
#include <fmt/format.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <utility>

export module aurora.service:logger;
import launcher.base;

namespace launcher {

export struct LogField {
    String key;
    String value;
    bool sensitive = false;
};

export struct LogRecord {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    String module;
    String message;
    Vector<LogField> fields;
    bool sensitive = false;
    String thread_name;
    u64 sequence;
};

export class LoggerImpl {
  private:
    Vector<SharedPtr<spdlog::sinks::sink>> sinks_;
    LoggerConfig config_;
    std::atomic<LogLevel> level_{LogLevel::Info};
    std::atomic<u32> level_version_{0};
    Vector<ModuleLogRule> module_levels_;

    struct Buffer {
        LogRecord *items;
        u32 count, capacity;
    };

    std::mutex active_mu_;

    Buffer active_, ready_, flush_;

    std::atomic<u64> dropped_count_{0};

    details::RecentLogRing ring_;
};

export class Logger {
  private:
    SharedPtr<LoggerImpl> impl_;

  public:
    static Result<void> Initialize(const LoggerConfig &config);

    static bool IsInitialized() noexcept;

    static void Shutdown();

    static void SetLevel(LogLevel level) noexcept;

    static LogLevel Level() noexcept;

    template <typename... Args>
    static void Trace(fmt::format_string<Args...> fmt, Args &&...args);


    template <typename... Args>
    static void Debug(fmt::format_string<Args...> fmt, Args &&...args);

    template <typename... Args>
    static void Info(fmt::format_string<Args...> fmt, Args &&...args);

    template <typename... Args>
    static void Warn(fmt::format_string<Args...> fmt, Args &&...args);

    template <typename... Args>
    static void Error(fmt::format_string<Args...> fmt, Args &&...args);

    template <typename... Args>
    static void Critical(fmt::format_string<Args...> fmt, Args &&...args);
};
namespace details {
spdlog::logger *GetLogger();
spdlog::level::level_enum ToSpdlogLevel(LogLevel);
}  // namespace details

}  // namespace launcher

namespace launcher {
template <typename... Args>
void Logger::Trace(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->trace(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Debug(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->debug(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Info(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->info(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Warn(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->warn(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Error(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->error(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
void Logger::Critical(fmt::format_string<Args...> fmt, Args &&...args) {
    if (auto logger = details::GetLogger())
        logger->critical(fmt, std::forward<Args>(args)...);
}
}  // namespace launcher
