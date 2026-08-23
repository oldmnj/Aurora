/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
/**
 * @file launcher.base.config.cppm
 * @brief 定义需要的config
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <algorithm>
#include <chrono>
#include <thread>

export module launcher.base:config;
import :types;
import :error;

// Config
namespace launcher {
/**
 * @brief 输出日志等级
 */
export enum class LogLevel {
    Trace,
    Debug,  // 基于spdlog包装
    Info,
    Warn,
    Error,
    Critical,
    Off
};
/**
 * @brief与路径有关都配置
 */
export struct PathConfig {
    Path cache_directory = "./cache";  // 下载缓存目录
    Path temp_directory  = "./tmp";    // 下载临时目录，程序退出后须删除
    Path log_directory   = "./log";    // 日志输出目录
    Path runtime_directory =
            "./runtime";  // SDK 工作目录，任何相对路径都要基于此处而言
};

export struct ModuleLogLevel {
    String module;
    LogLevel level;
};

export struct MirrorRule {
    String from;
    String to;
};

/**
 * @brief 与日志有关的配置
 */
export struct LoggerConfig {
    LogLevel level = LogLevel::Info;
    String pattern = "[%Y-%m-%d %H:%M:$S.%e] [%l] [%n] %v";
    Path log_dir;  ///< 空: 默认logs/
    String file_name       = "launcher.log";
    u64 max_file_size      = 16'777'216;  ///< 16*1024*1024
    u32 max_file_count     = 5;
    bool is_async          = true;
    bool flush_immediately = false;
    Vector<ModuleLogLevel> module_levels;
    u32 recent_capacity = 4096;
};

export struct DownloadConfig {
    u32 max_concurrent                   = 8;
    u32 per_host_limit                   = 4;
    u32 retry_count                      = 3;
    std::chrono::seconds timeout         = std::chrono::seconds{30};
    std::chrono::seconds connect_timeout = std::chrono::seconds{10};
    bool resume                          = true;
    String user_agent                    = "AuroraLauncher/0.2.0";
    Vector<MirrorRule> mirrors;
    bool verify_checksum     = true;
    bool keep_part_on_cancel = false;
    Path temp_dir;
    u64 max_partial_bytes = 0;
};

/**
 * @brief 与网络相关的配置
 */
export struct NetworkConfig {
    String proxy;
    std::chrono::seconds timeout         = std::chrono::seconds{30};
    std::chrono::seconds connect_timeout = std::chrono::seconds{10};
    u32 max_connections                  = 32;
    bool verify_tls                      = true;
    std::chrono::seconds dns_timeout     = std::chrono::seconds{5};
    bool retry_on_5xx                    = true;
    bool allow_insecure_download         = true;
};

/**
 * @brief 运行时相关都配置
 */
export struct RuntimeConfig {
    u32 worker_threads =
            std::max(4u, std::thread::hardware_concurrency());  // 下载线程数
    bool debug_mode   = false;  // Logger: Debug & Trace
    bool enable_cache = true;   // 是否缓存下载
};

/**
 * @brief 存储具体的配置信息
 */
export class Config {
  public:
    Config() = default;
    Result<void> Validate() const;  // 验证配置是否合法
    void Reset();

    PathConfig path;
    LoggerConfig logger;
    NetworkConfig network;
    RuntimeConfig runtime;
};
}  // namespace launcher
