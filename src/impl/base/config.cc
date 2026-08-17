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
}  // namespace launcher
