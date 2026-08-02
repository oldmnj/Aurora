module;

#include <algorithm>
#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

module launcher.base;

namespace launcher {

Result<void> Config::Validate() const {
    if (this->path.cache_directory.empty() || this->path.log_directory.empty() ||
            this->path.runtime_directory.empty() || this->path.temp_directory.empty() 
        /*||
            !std::filesystem::exists(this->path.cache_directory) ||
            !std::filesystem::exists(this->path.runtime_directory) ||
            !std::filesystem::exists(this->path.log_directory) ||
            !std::filesystem::exists(this->path.temp_directory)
    */) {
        return std::unexpected<Error>(
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument, "路径参数无效"});
    } else if (this->network.timeout <= std::chrono::seconds{0}) {
        return std::unexpected<Error>{
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument, "超时参数不是正数"}
        };
    } else if (this->runtime.worker_threads <= 0) {
        return std::unexpected<Error>{
                Error{ErrorCategory::Config, ErrorCode::InvalidArgument,
                      "runtime: 线程数不能为非正整数"}
        };
    } else {
        return {};
    }
    return std::unexpected<Error>{
            Error{ErrorCategory::Config, ErrorCode::InternalError, "未知错误"}
    };
}

void Config::Reset() {
    this->path    = PathConfig{};
    this->runtime = RuntimeConfig{};
    this->network = NetworkConfig{};
    this->logger  = LoggerConfig{};
}

namespace details {
static SharedPtr<Config> g_config;
// 此处把g_config写在details命名空间可以不被导出符号，以免cppm中的static成员变量会导出符号
}  // namespace details
void ConfigManager::Load() {
    details::g_config          = std::make_shared<Config>();
    details::g_config->path    = PathConfig{};
    details::g_config->network = NetworkConfig{};
    details::g_config->runtime = RuntimeConfig{};
    details::g_config->logger  = LoggerConfig{};
}

const Config &ConfigManager::Get() { return *details::g_config; }

namespace {
inline Path GetPathFromJson(const nlohmann::json &obj, const char *key, const char *default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_string()) {
        return Path(it->get<std::string>());
    }
    return Path(default_val);
}

inline LogLevel GetLogLevelFromJson(
        const nlohmann::json &obj, const char *key, LogLevel default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_string()) {
        String level_str = it->get<String>();
        if (level_str == "trace")
            return LogLevel::Trace;
        if (level_str == "debug")
            return LogLevel::Debug;
        if (level_str == "info")
            return LogLevel::Info;
        if (level_str == "warn")
            return LogLevel::Warn;
        if (level_str == "error")
            return LogLevel::Error;
        if (level_str == "critical")
            return LogLevel::Critical;
        if (level_str == "off")
            return LogLevel::Off;
    }
    return default_val;
}

inline std::chrono::seconds GetSecondsFromJson(
        const nlohmann::json &obj, const char *key, std::chrono::seconds default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_number()) {
        return std::chrono::seconds(it->get<u32>());
    }
    return default_val;
}

inline u32 GetU32FromJson(const nlohmann::json &obj, const char *key, u32 default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_number()) {
        return it->get<u32>();
    }
    return default_val;
}

inline bool GetBoolFromJson(const nlohmann::json &obj, const char *key, bool default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_boolean()) {
        return it->get<bool>();
    }
    return default_val;
}
}  // namespace

Result<void> ConfigManager::Load(Path config_path) {
    if (config_path.empty()) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::ParseError, "路径参数为空"}
        };
    }

    if (!std::filesystem::exists(config_path)) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::ParseError, "参数提供的路径无效"}
        };
    }
    std::ifstream config_file{config_path};
    if (!config_file.is_open()) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::IOError, "文件打开失败"}
        };
    }
    nlohmann::json config_json;
    config_file >> config_json;

    if (auto it = config_json.find("path"); it != config_json.end() && it->is_object()) {
        auto &path_config = *it;

        auto get_path     = [&](const char *key, const char *default_val) -> Path {
            if (auto iter = path_config.find(key); iter != path_config.end() && iter->is_string()) {
                return Path(iter->get<std::string>());
            }
            return Path(default_val);
        };

        details::g_config->path = PathConfig{.cache_directory = get_path("cache_dir", "./cache"),
                .temp_directory                               = get_path("temp_dir", "./tmp"),
                .log_directory                                = get_path("log_dir", "./log"),
                .runtime_directory = get_path("runtime_dir", "./runtime")};
    }

    if (auto it = config_json.find("logger"); it != config_json.end() && it->is_object()) {
        auto &logger_config       = *it;

        details::g_config->logger = LoggerConfig{
                .level             = GetLogLevelFromJson(logger_config, "level", LogLevel::Info),
                .flush_immediately = GetBoolFromJson(logger_config, "flush_immediately", false),
                .console_output    = GetBoolFromJson(logger_config, "console_output", true),
                .file_output       = GetBoolFromJson(logger_config, "file_output", true),
                .file_name         = GetPathFromJson(logger_config, "file_name", "launcher.log"),
                .is_async          = GetBoolFromJson(logger_config, "is_async", false)};
    }

    if (auto it = config_json.find("network"); it != config_json.end() && it->is_object()) {
        auto &network_config       = *it;

        details::g_config->network = NetworkConfig{
                .timeout = GetSecondsFromJson(network_config, "timeout", std::chrono::seconds{30}),
                .retry_count = GetU32FromJson(network_config, "retry_count", 3),
                .verify_ssl  = GetBoolFromJson(network_config, "verify_ssl", true)};
    }

    if (auto it = config_json.find("runtime"); it != config_json.end() && it->is_object()) {
        auto &runtime_config = *it;

        details::g_config->runtime =
                RuntimeConfig{.worker_threads = GetU32FromJson(runtime_config, "worker_threads",
                                      std::max(4u, std::thread::hardware_concurrency())),
                        .debug_mode   = GetBoolFromJson(runtime_config, "debug_mode", false),
                        .enable_cache = GetBoolFromJson(runtime_config, "enable_cache", true)};
    }

    return {};
}

Result<void> ConfigManager::Save(Path config_path) {
    if (config_path.empty()) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::ParseError, "路径参数为空"}
        };
    }

    if (!std::filesystem::exists(config_path)) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::IOError, "参数提供的路径无效"}
        };
    }

    std::ofstream config_file(config_path);

    if (!config_file.is_open()) {
        return std::unexpected<Error>{
                Error{ErrorCategory::IO, ErrorCode::IOError, "文件打开失败"}
        };
    }

    auto &config = *details::g_config;
    nlohmann::json fconfig{
            {"path",    {{"cache_dir", config.path.cache_directory.string()},
                             {"temp_dir", config.path.temp_directory.string()},
                             {"log_dir", config.path.log_directory.string()},
                             {"runtime_dir", config.path.runtime_directory.string()}}},
            {"logger",  {{"level",
                                [&]() -> String {
                                    switch (config.logger.level) {
                                    case LogLevel::Trace:
                                        return "trace";
                                    case LogLevel::Debug:
                                        return "debug";
                                    case LogLevel::Info:
                                        return "info";
                                    case LogLevel::Warn:
                                        return "warn";
                                    case LogLevel::Error:
                                        return "error";
                                    case LogLevel::Critical:
                                        return "critical";

                                    case LogLevel::Off:
                                        return "off";
                                    }
                                    return "info";
                                }()},
                               {"flush_immediately", config.logger.flush_immediately},
                               {"console_output", config.logger.console_output},
                               {"file_output", config.logger.file_output},
                               {"file_name", config.logger.file_name.string()},
                               {"async", config.logger.is_async}}                  },
            {"network", {{"timeout", config.network.timeout.count()},
                                {"retry_count", config.network.retry_count},
                                {"verify_ssl", config.network.verify_ssl}}        },
            {"runtime", {{"worker_threads", config.runtime.worker_threads},
                                {"debug_mode", config.runtime.debug_mode},
                                {"enable_cache", config.runtime.enable_cache}}    }
    };

    try {
        if (!config_file.is_open()) {
            return std::unexpected<Error>{
                    Error{ErrorCategory::IO, ErrorCode::IOError, "文件打开失败"}
            };
        }
        config_file << fconfig.dump(4);
    } catch (...) {
    }

    return {};
}

}  // namespace launcher
