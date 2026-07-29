/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

#include <algorithm>
#include <chrono>
#include <thread>

export module launcher.base;
export import :types;
export import :error;


//
namespace launcher {
export enum class LogLevel {
    Trace,
    Debug,  // 基于spdlog包装
    Info,
    Warn,
    Error,
    Critical,
    Off
};
/*
export class Logger {
    static void Initialize();
    static void Shutdown();
    static void Trace();
    static void Debug();
    static void Info();
    static void Warn();
    static void Error();
    static void Critical();
};
*/

}  // namespace launcher

namespace launcher {
export struct Version {
    u32 major{};
    u32 minor{};
    u32 patch{};

    constexpr auto operator<=>(const Version &) const = default;

    [[nodiscard]]
    String ToString() const;
};

export enum class Platform { Windows, Linux, MacOS, Android, IOS, Unknown };

export enum class Architecture { X86, X64, ARM, ARM64, RISCV64, Unknown };

export [[nodiscard]] Platform CurrentPlatform() noexcept;

export [[nodiscard]] Architecture CurrentArchitecture() noexcept;

export [[nodiscard]] bool Is64Bit() noexcept;

export [[nodiscard]] bool IsLittleEndian() noexcept;

export constexpr StringView ToString(Platform) noexcept;
export constexpr StringView ToString(Architecture) noexcept;
}  // namespace launcher
//


// Config
namespace launcher {

export struct PathConfig {
    Path cache_directory   = "./cache";    // 下载缓存目录
    Path temp_directory    = "./tmp";      // 下载临时目录，程序退出后须删除
    Path log_directory     = "./log";      // 日志输出目录
    Path runtime_directory = "./runtime";  // SDK 工作目录，任何相对路径都要基于此处而言
};

export struct LoggerConfig {
    LogLevel level         = LogLevel::Info;
    bool flush_immediately = false;  // 每条日志立即Flush
    bool console_output    = true;   // 是否终端打印
    bool file_output       = true;   // 是否输出文件
};

export struct NetworkConfig {
    std::chrono::seconds timeout = std::chrono::seconds{30};
    u32 retry_count              = 3;
    bool verify_ssl              = true;
};

export struct RuntimeConfig {
    u32 worker_threads = std::max(4u, std::thread::hardware_concurrency());  // 下载线程数
    bool debug_mode    = false;  // Logger: Debug & Trace
    bool enable_cache  = true;   // 是否缓存下载
};

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
