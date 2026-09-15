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
#include <memory>
#include <new>
#include <source_location>
#include <type_traits>
#include <utility>
#include <variant>

export module launcher.base:error;
import :types;

namespace launcher {

export enum class ErrorCode {
    Ok,
    InvalidArgument,
    InvalidState,
    Unsupported,
    IOError,
    FileNotFound,
    PermissionDenied,
    NetworkError,
    Timeout,
    ConnectionFailed,
    ParseError,
    InvalidFormat,
    DownloadFailed,
    ChecksumMismatch,
    InternalError,
    Unknown
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
    Minecraft
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

    /*
    Error(ErrorCategory category, ErrorCode code, String message,
          UniquePtr<Error> cause,
          std::source_location location = std::source_location::current());

    template <typename... Args>
    [[nodiscard]]
    static Error
    Format(ErrorCategory category, ErrorCode code,
           fmt::format_string<Args...> fmt, Args &&...args,
           UniquePtr<Error> cause,
           std::source_location location = std::source_location::current());

    */

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

  private:
    ErrorCode code_;
    ErrorCategory category_;
    String message_;
    std::source_location location_;
    SharedPtr<Error> cause_;
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
