module;

#include <expected>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

module aurora.service;
import launcher.base;

namespace launcher {
/*
namespace {
static
}
*/

namespace details {
static SharedPtr<spdlog::logger> g_logger;

spdlog::logger *GetLogger() { return g_logger.get(); }

static LogLevel current_level = LogLevel::Info;

spdlog::level::level_enum ToSpdlogLevel(LogLevel log_level) {
    switch (log_level) {
    case LogLevel::Trace:
        return spdlog::level::level_enum::trace;

    case LogLevel::Debug:
        return spdlog::level::debug;

    case LogLevel::Info:
        return spdlog::level::level_enum::info;

    case LogLevel::Warn:
        return spdlog::level::level_enum::warn;
    case LogLevel::Error:
        return spdlog::level::level_enum::err;

    case LogLevel::Critical:
        return spdlog::level::level_enum::critical;

    case LogLevel::Off:
        return spdlog::level::level_enum::off;

    default:
        return spdlog::level::level_enum::off;
        break;
    }
}
}  // namespace details

Result<void> Logger::Initialize(const LoggerConfig &config) {
    if (details::g_logger) {
        return {};
    }


    Vector<spdlog::sink_ptr> sinks;


    if (config.console_output) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    }


    if (config.file_output) {
        sinks.push_back(
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        config.file_name.string()
                )
        );
    }


    if (sinks.empty()) {
        return Err(
                {ErrorCategory::Runtime, ErrorCode::InvalidArgument,
                 "Logger没有任何输出目标"}
        );
    }


    auto logger = std::make_shared<spdlog::logger>(
            "launcher", sinks.begin(), sinks.end()
    );


    logger->set_level(details::ToSpdlogLevel(config.level));


    if (config.flush_immediately) {
        logger->flush_on(spdlog::level::trace);
    }


    details::g_logger      = logger;


    details::current_level = config.level;


    return {};
}

bool Logger::IsInitialized() noexcept {
    if (details::g_logger) {
        return true;
    }
    return false;
}

void Logger::Shutdown() {
    if (details::g_logger) {
        spdlog::drop("launcher");
        details::g_logger.reset();
    }
    // 这里可能有点问题，但是马上会写Context统一调动
}

LogLevel Logger::Level() noexcept { return details::current_level; }

void Logger::SetLevel(LogLevel level) noexcept {
    if (details::g_logger) {
        details::g_logger->set_level(details::ToSpdlogLevel(level));
    }
    details::current_level = level;
}

}  // namespace launcher
