/**
 * @file config.cc
 * @brief 配置相关的实现
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <chrono>

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
                 "arg of path is invalid"}
        );
    } else if (this->network.timeout <= std::chrono::seconds{0}) {
        return Err(
                {ErrorCategory::Config, ErrorCode::InvalidArgument,
                 "the timeout must > 0"}
        );
    } else if (this->runtime.worker_threads <= 0) {
        return Err(
                {ErrorCategory::Config, ErrorCode::InvalidArgument,
                 "runtime: the worker_threads cannot be '0'"}
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
}  // namespace launcher
