/*

SPDX-License-Identifier: MIT

Copyright (c) 2026 oldmnj oldmnj@163.com

This is the core kernel module of the launcher.

For license details, see the LICENSE file in the root directory.
*/
module;


#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <utility>

export module launcher.base:logger;
import :error;
import :types;
import :config;

namespace launcher {

export class Logger {
  private:
    Logger()  = default;
    ~Logger() = default;

  public:
    static Result<void> Initialize(const LoggerConfig &config);

    static bool IsInitialized() noexcept;

    static void Shutdown();

    static void SetLevel(LogLevel Level) noexcept;

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
