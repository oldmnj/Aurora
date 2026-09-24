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
import :result;


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

export struct ModuleLogRule {
    String module;
    LogLevel level;
};

export struct MirrorRule {
    String from;
    String to;
};

export struct ThirdPartyServer {
    String name;
    String yggdrasil_base_url;
};

u32 DefaultWorkerThreads() {
    u32 hw = std::thread::hardware_concurrency();
    return std::min(std::max(hw, 4u), 8u);
}

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
    Vector<ModuleLogRule> module_levels;
    u32 recent_capacity = 4096;
    bool json           = true;
    bool console        = true;
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

export struct AuthConfig {
    String default_service = "offline";  // offline / microsoft / <皮肤站名>
    String client_id;  // 微软 OAuth ClientID（空则不启用微软登录）
    String redirect_uri     = "http://127.0.0.1:0";  // 0 = 随机端口
    bool enable_device_code = true;                  // 设备码回退开关
    std::chrono::seconds device_code_poll_interval = std::chrono::seconds{5};
    u32 device_code_max_polls                      = 120;  // 10 分钟上限
    bool persist_flow       = false;   // auth_flow.json 持久化
    bool auto_refresh       = true;    // 启动时自动刷新过期令牌
    String token_encryption = "auto";  // auto/os_keychain/derived
    Vector<ThirdPartyServer>
            third_party_servers;  // 皮肤站（yggdrasil，12.2/32.5）
};


/**
 * @brief 运行时相关都配置
 */
export struct RuntimeConfig {
    // 线程/运行调优（13 章、27.21、附录 H）
    u32 worker_threads          = DefaultWorkerThreads();  // min(max(4,hw),8)
    bool debug_mode             = false;
    bool enable_cache           = true;
    u32 download_concurrency    = 8;
    bool low_memory_mode        = false;  // 低内存模式：Xmx 上限 2G + 降并发
    u32 max_concurrent_launches = 1;      // 并发启动上限
    String java_path;                     // 空 → 自动探测
    String java_args  = "-Xmx2G";
    u32 min_memory_mb = 1024;
    u32 max_memory_mb = 2048;
    Path game_dir;  // 空 → 默认 .minecraft 风格目录
    bool keep_jvm_args           = true;
    bool auto_download_jre       = false;
    String jre_mirror            = "";  // 空 = 官方
    u32 window_width             = 854;
    u32 window_height            = 480;
    bool fullscreen              = false;
    String server_address        = "";  // 进服地址（host[:port]）
    bool export_official_profile = false;
    bool multi_instance          = false;
    bool online_mode             = true;  // 会话 join
};

export struct PluginConfig {
    bool enabled     = true;
    Path plugins_dir = "plugins";
    Vector<String> disabled_plugins;             // 按 id 禁用
    bool allow_network                  = true;  // JS 插件网络权限
    bool allow_launch                   = true;
    bool allow_fs                       = false;
    u32 quickjs_heap_limit_mb           = 128;
    std::chrono::seconds plugin_timeout = std::chrono::seconds{30};
    bool sandbox_strict = true;  // 严格沙箱（禁 eval/new Function）
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
