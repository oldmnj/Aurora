/**
 * @file config.cc
 * @brief 配置相关的实现
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <algorithm>
#include <chrono>
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
        return Err(
                {ErrorCategory::Config, ErrorCode::InvalidArgument,
                 "路径参数无效"}
        );
    } else if (this->network.timeout <= std::chrono::seconds{0}) {
        return Err(
                {ErrorCategory::Config, ErrorCode::InvalidArgument,
                 "超时参数不是正数"}
        );
    } else if (this->runtime.worker_threads <= 0) {
        return Err(
                {ErrorCategory::Config, ErrorCode::InvalidArgument,
                 "runtime: 线程数不能为非正整数"}
        );
    } else {
        return {};
    }
}

void Config::Reset() {
    this->path    = PathConfig{};
    this->runtime = RuntimeConfig{};
    this->network = NetworkConfig{};
    this->logger  = LoggerConfig{};
}

/// 注: 原有ConfigManager及其所有实现全部删除，config将于上层context/app管理
/*
namespace details {
static SharedPtr<Config> g_config;
//
此处把g_config写在details命名空间可以不被导出符号，以免cppm中的static成员变量会导出符号
}  // namespace details
void ConfigManager::Load() {
    details::g_config          = std::make_shared<Config>();
    details::g_config->path    = PathConfig{};
    details::g_config->network = NetworkConfig{};
    details::g_config->runtime = RuntimeConfig{};
    details::g_config->logger  = LoggerConfig{};
}

const Config &ConfigManager::Get() { return *details::g_config; }
/*
 * 需检查空指针，
 * 不过马上就要写Context，暂时不修
 */
/*
namespace {
inline Path GetPathFromJson(
        const nlohmann::json &obj, const char *key, const char *default_val
) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_string()) {
        return Path(it->get<std::string>());
    }
    return Path(default_val);
}

inline LogLevel GetLogLevelFromJson(
        const nlohmann::json &obj, const char *key, LogLevel default_val
) {
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
        const nlohmann::json &obj, const char *key,
        std::chrono::seconds default_val
) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_number()) {
        return std::chrono::seconds(it->get<u32>());
    }
    return default_val;
}

inline u32
GetU32FromJson(const nlohmann::json &obj, const char *key, u32 default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_number()) {
        return it->get<u32>();
    }
    return default_val;
}

inline bool
GetBoolFromJson(const nlohmann::json &obj, const char *key, bool default_val) {
    auto it = obj.find(key);
    if (it != obj.end() && it->is_boolean()) {
        return it->get<bool>();
    }
    return default_val;
}
}  // namespace

Result<void> ConfigManager::Load(Path config_path) {
    if (config_path.empty()) {
        return Err({ErrorCategory::IO, ErrorCode::ParseError, "路径参数为空"});
    }

    if (!std::filesystem::exists(config_path)) {
        return Err(
                Error{ErrorCategory::IO, ErrorCode::ParseError,
                      "参数提供的路径无效"}
        );
    }
    std::ifstream config_file{config_path};
    if (!config_file.is_open()) {
        return Err(Error{ErrorCategory::IO, ErrorCode::IOError,
"文件打开失败"});
    }
    nlohmann::json config_json;
    config_file >> config_json;

    if (auto it = config_json.find("path");
        it != config_json.end() && it->is_object()) {
        auto &path_config = *it;

        auto get_path = [&](const char *key, const char *default_val) -> Path {
            if (auto iter = path_config.find(key);
                iter != path_config.end() && iter->is_string()) {
                return Path(iter->get<std::string>());
            }
            return Path(default_val);
        };

        details::g_config->path = PathConfig{
                .cache_directory   = get_path("cache_dir", "./cache"),
                .temp_directory    = get_path("temp_dir", "./tmp"),
                .log_directory     = get_path("log_dir", "./log"),
                .runtime_directory = get_path("runtime_dir", "./runtime")
        };
    }

    if (auto it = config_json.find("logger");
        it != config_json.end() && it->is_object()) {
        auto &logger_config       = *it;

        details::g_config->logger = LoggerConfig{
                .level = GetLogLevelFromJson(
                        logger_config, "level", LogLevel::Info
                ),
                .flush_immediately = GetBoolFromJson(
                        logger_config, "flush_immediately", false
                ),
                .console_output =
                        GetBoolFromJson(logger_config, "console_output", true),
                .file_output =
                        GetBoolFromJson(logger_config, "file_output", true),
                .file_name = GetPathFromJson(
                        logger_config, "file_name", "launcher.log"
                ),
                .is_async = GetBoolFromJson(logger_config, "is_async", false)
        };
    }

    if (auto it = config_json.find("network");
        it != config_json.end() && it->is_object()) {
        auto &network_config       = *it;

        details::g_config->network = NetworkConfig{
                .timeout = GetSecondsFromJson(
                        network_config, "timeout", std::chrono::seconds{30}
                ),
                .retry_count = GetU32FromJson(network_config, "retry_count", 3),
                .verify_ssl =
                        GetBoolFromJson(network_config, "verify_ssl", true)
        };
    }

    if (auto it = config_json.find("runtime");
        it != config_json.end() && it->is_object()) {
        auto &runtime_config       = *it;

        details::g_config->runtime = RuntimeConfig{
                .worker_threads = GetU32FromJson(
                        runtime_config, "worker_threads",
                        std::max(4u, std::thread::hardware_concurrency())
                ),
                .debug_mode =
                        GetBoolFromJson(runtime_config, "debug_mode", false),
                .enable_cache =
                        GetBoolFromJson(runtime_config, "enable_cache", true)
        };
    }

    return {};
}

Result<void> ConfigManager::Save(Path config_path) {
    if (config_path.empty()) {
        return Err(
                Error{ErrorCategory::IO, ErrorCode::ParseError, "路径参数为空"}
        );
    }

    /*
        if (!std::filesystem::exists(config_path)) {
            return Err(
                    Error{ErrorCategory::IO, ErrorCode::IOError,
                          "参数提供的路径无效"}
            );
        }
    */
/*
    std::ofstream config_file(config_path);

    if (!config_file.is_open()) {
        return Err(Error{ErrorCategory::IO, ErrorCode::IOError,
"文件打开失败"});
    }

    auto &config = *details::g_config;
    // 这里需要防止nullptr，不过马上就要写Context，到时候删
    nlohmann::json fconfig;
    fconfig["path"]["cache_dir"]   = config.path.cache_directory.string();
    fconfig["path"]["temp_dir"]    = config.path.temp_directory.string();
    fconfig["path"]["log_dir"]     = config.path.log_directory.string();
    fconfig["path"]["runtime_dir"] = config.path.runtime_directory.string();

    String level_str               = "info";
    switch (config.logger.level) {
    case LogLevel::Trace:
        level_str = "trace";
        break;
    case LogLevel::Debug:
        level_str = "debug";
        break;
    case LogLevel::Info:
        level_str = "info";
        break;
    case LogLevel::Warn:
        level_str = "warn";
        break;
    case LogLevel::Error:
        level_str = "error";
        break;
    case LogLevel::Off:
        level_str = "off";
        break;
    case LogLevel::Critical:
        level_str = "critical";
        break;
    }
    fconfig["logger"]["level"]             = level_str;
    fconfig["logger"]["flush_immediately"] = config.logger.flush_immediately;
    fconfig["logger"]["console_output"]    = config.logger.console_output;
    fconfig["logger"]["file_output"]       = config.logger.file_output;
    fconfig["logger"]["file_name"]         = config.logger.file_name.string();
    fconfig["logger"]["is_async"]          = config.logger.is_async;

    fconfig["network"]["timeout"]          = config.network.timeout.count();
    fconfig["network"]["retry_count"]      = config.network.retry_count;
    fconfig["network"]["verify_ssl"]       = config.network.verify_ssl;

    fconfig["runtime"]["worker_threads"]   = config.runtime.worker_threads;
    fconfig["runtime"]["debug_mode"]       = config.runtime.debug_mode;
    fconfig["runtime"]["enable_cache"]     = config.runtime.enable_cache;

    try {
        config_file << fconfig.dump(4);
    } catch (...) {
        return Err(
                Error{ErrorCategory::IO, ErrorCode::IOError,
                      "在写入config的json文件时失败，也可能是系统问题，权限不够"
                      "等等"}
        );
    }

    return {};
}
*/

}  // namespace launcher
