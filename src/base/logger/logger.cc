module;

#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

module launcher.base;
import :config;

namespace launcher {
/*
namespace {
static
}
*/

namespace details {
static SharedPtr<spdlog::logger> g_logger;

spdlog::logger &GetLogger() { return *g_logger; }

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
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    Vector<spdlog::sink_ptr> sinks;

    if (config.console_output) {
        sinks.push_back(console);


        auto logger = std::make_shared<spdlog::logger>("launcher", sinks.begin(), sinks.end());

        logger->set_level(details::ToSpdlogLevel(config.level));

        details::g_logger = logger;

        spdlog::register_logger(logger);

        return {};
    }
}

void Logger::Shutdown() {
    if (details::g_logger) {
        spdlog::drop("launcher");
        details::g_logger.reset();
    }
}

}  // namespace launcher
