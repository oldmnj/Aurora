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

/**
 * @brief 与日志有关的配置
 */
export struct LoggerConfig {
    LogLevel level         = LogLevel::Info;
    bool flush_immediately = false;  // 每条日志立即Flush
    bool console_output    = true;   // 是否终端打印
    bool file_output       = true;   // 是否输出文件
    Path file_name         = "launcher.log";
    bool is_async          = false;
};

/**
 * @brief 与网络相关的配置
 */
export struct NetworkConfig {
    std::chrono::seconds timeout = std::chrono::seconds{30};
    u32 retry_count              = 3;
    bool verify_ssl              = true;
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
