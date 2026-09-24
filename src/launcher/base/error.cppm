/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
/**
 * @file launcher.base.error.cppm
 * @brief 提供统一的错误处理类型
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <fmt/format.h>
#include <source_location>

export module launcher.base:error;
import :types;

namespace launcher {

export enum class ErrorCode {
    // 基础状态
    Ok,
    Unknown,
    Cancelled,
    InternalError,

    // 参数与状态校验
    InvalidArgument,
    InvalidState,
    Unsupported,

    // IO 与文件系统
    IOError,
    FileNotFound,
    FileTooLarge,
    FileAlreadyExists,
    DiskFull,
    PermissionDenied,

    // 网络
    NetworkError,
    Timeout,
    ConnectionFailed,
    ConnectionReset,
    RateLimited,

    // 解析与数据
    ParseError,
    InvalidFormat,
    DownloadFailed,
    ChecksumMismatch,
    JsonError,
    DataCorrupted,

    // JVM 与进程
    JvmNotFound,
    JvmIncompatible,
    ProcessCreateFailed,
    ProcessLaunchFailed,
    ProcessCrashed,
    OutOfMemory,

    // 版本与依赖
    VersionNotFound,
    VersionResolveFailed,
    AssetMissing,
    DependencyMissing,
    DependencyConflict,

    // 认证与安全
    AuthFailed,
    AuthExpired,
    TokenInvalid,

    // 插件与脚本
    PluginLoadFailed,
    PluginError,
    PluginDisabled,
    ScriptError,
    ScriptTimeout,

    // 配置与热重载
    ConfigLoadFailed,
    ConfigWatchFailed,
    ConfigParseError,
    HotReloadFailed
};

export enum class ErrorCategory {
    None,
    System,
    Parse,
    IO,
    Network,
    Security,
    Config,
    Runtime,
    Minecraft,
    Auth,
    Plugin,
    Script
};

export class Error {
  public:
    Error(ErrorCategory category, ErrorCode code, String message,
          std::source_location location = std::source_location::current());

    template <typename... Args>
    [[nodiscard]]
    static Error
    Format(ErrorCategory category, ErrorCode code,
           fmt::format_string<Args...> fmt, Args &&...args,
           std::source_location location = std::source_location::current());


    Error(ErrorCategory category, ErrorCode code, String message,
          SharedPtr<const Error> cause,
          std::source_location location = std::source_location::current());

    [[nodiscard]] ErrorCode Code() const noexcept;
    [[nodiscard]]
    StringView Message() const noexcept;
    [[nodiscard]]
    const std::source_location &Location() const noexcept;

    [[nodiscard]]
    static constexpr StringView ToString(ErrorCode) noexcept;

    [[nodiscard]]
    static constexpr StringView ToString(ErrorCategory) noexcept;

    [[nodiscard]]
    String ToString() const;

    [[nodiscard]]
    ErrorCategory Category() const noexcept;

    auto WithCause(Error cause) -> Error;

    // 注意，仅在你不会使用原对象时使用此函数
    auto rWithCause(Error &&cause) -> Error;

    [[nodiscard]]
    auto HasCause() const -> bool;

    [[nodiscard]]
    auto Cause() const -> const Error *;

    [[nodiscard]]
    auto ChainDepth() const -> usize;

  private:
    ErrorCode code_;
    ErrorCategory category_;
    String message_;
    std::source_location location_;
    SharedPtr<const Error> cause_;
};

/*


// Ok - 成功值
export template <typename T = void, typename E = Error>
[[nodiscard]]
constexpr Result<T, E> Ok() {
    if constexpr (std::is_void_v<T>) {
        return Result<T, E>{};
    } else {
        return Result<T, E>{InPlaceValueTag{}};
    }
}

export template <typename T, typename E = Error>
[[nodiscard]]
constexpr Result<std::decay_t<T>, E> Ok(T &&value) {
    return Result<std::decay_t<T>, E>{InPlaceValueTag{},
std::forward<T>(value)};
}

export template <typename T, typename E = Error, typename... Args>
[[nodiscard]]
constexpr Result<T, E> OkEmplace(Args &&...args) {
    return Result<T, E>{InPlaceValueTag{}, std::forward<Args>(args)...};
}

// Err - 错误值
export template <typename T = void, typename E = Error>
[[nodiscard]]
constexpr Result<T, E> Err(E &&error) {
    return Result<T, E>{InPlaceErrorTag{}, std::forward<E>(error)};
}

export template <typename T = void, typename E = Error>
[[nodiscard]]
constexpr Result<T, E> Err(const E &error) {
    return Result<T, E>{InPlaceErrorTag{}, error};
}

export template <typename T = void, typename E = Error, typename... Args>
[[nodiscard]]
constexpr Result<T, E> ErrEmplace(Args &&...args) {
    return Result<T, E>{InPlaceErrorTag{}, std::forward<Args>(args)...};
}
*/

}  // namespace launcher
