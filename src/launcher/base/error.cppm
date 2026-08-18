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
    // 通用错误
    InvalidArgument,
    InvalidState,
    Unsupported,

    // IO
    IOError,
    FileNotFound,
    FileTooLarge,
    PermissionDenied,

    // 网络
    NetworkError,
    Timeout,
    ConnectionFailed,

    // 数据
    ParseError,
    InvalidFormat,

    // 下载
    DownloadFailed,
    ChecksumMismatch,

    // 运行时
    InternalError,
    Unknown
};

export enum class ErrorCategory {  // 错误类型
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
    Error(ErrorCategory category, ErrorCode code, StringView message,
          std::source_location location = std::source_location::current());
    [[nodiscard]]
    ErrorCode Code() const noexcept;
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
    // UniquePtr<Error> cause_;
};

export struct InPlaceValueTag {};
export struct InPlaceErrorTag {};

export template <typename T, typename E = Error>
class Result {
  private:
    [[no_unique_address]] bool has_value_ = false;
    std::variant<T, E> storage_;

  public:
    using ValueType = T;
    using ErrorType = E;

    template <typename... Args>
    constexpr Result(InPlaceValueTag, Args &&...args) : has_value_(true) {
        storage_.template emplace<0>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    constexpr Result(InPlaceErrorTag, Args &&...args) : has_value_(false) {
        storage_.template emplace<1>(std::forward<Args>(args)...);
    }

    constexpr Result(T &&value) : Result{InPlaceValueTag{}, std::move(value)} {}
    constexpr Result(const T &value) : Result{InPlaceValueTag{}, value} {}

    constexpr Result(E &&error) : Result{InPlaceErrorTag{}, std::move(error)} {}
    constexpr Result(const E &error) : Result{InPlaceErrorTag{}, error} {}

    constexpr Result(const Result &other)
        : has_value_(other.has_value_), storage_(other.storage_) {}

    constexpr Result(Result &&other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_assignable_v<E>
    )
        : has_value_(other.has_value_), storage_(std::move(other.storage_)) {}

    constexpr Result &operator=(const Result &other) {
        if (this != &other) {
            storage_   = other.storage_;
            has_value_ = other.has_value_;
        }
        return *this;
    }

    constexpr Result &operator=(Result &&other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<E>
    ) {
        if (this != &other) {
            this->storage_   = std::move(other.storage_);
            this->has_value_ = other.has_value_;
        }
        return *this;
    }

    constexpr ~Result() noexcept(
            std::is_nothrow_destructible_v<T> &&
            std::is_nothrow_destructible_v<E>
    ) = default;

    [[nodiscard]] constexpr bool HasValue() const noexcept {
        return this->has_value_;
    }

    [[nodiscard]]
    constexpr bool HasError() const noexcept {
        return !this->has_value_;
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept {
        return this->has_value_;
    }

    [[nodiscard]]
    constexpr T &Value() & {
        return std::get<0>(storage_);
    }

    [[nodiscard]]
    constexpr const T &Value() const & {
        return std::get<0>(storage_);
    }

    [[nodiscard]]
    constexpr T &&Value() && {
        return std::get<0>(std::move(storage_));
    }

    [[nodiscard]]
    constexpr const T &&Value() const && {
        return std::get<0>(std::move(storage_));
    }


    template <typename U>
    [[nodiscard]]
    constexpr T ValueOr(U &&default_value) const & {
        return has_value_ ? std::get<0>(storage_)
                          : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    [[nodiscard]]
    constexpr T ValueOr(U &&default_value) && {
        return has_value_ ? std::get<0>(std::move(storage_))
                          : static_cast<T>(std::forward<U>(default_value));
    }

    [[nodiscard]]
    constexpr E &Error() & {
        return std::get<1>(storage_);
    }

    [[nodiscard]]
    constexpr const E &Error() const & {
        return std::get<1>(storage_);
    }

    [[nodiscard]]
    constexpr E &&Error() && {
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]]
    constexpr const E &&Error() const && {
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]]
    constexpr T *operator->() {
        return std::addressof(std::get<0>(storage_));
    }

    [[nodiscard]]
    constexpr const T *operator->() const {
        return std::addressof(std::get<0>(storage_));
    }

    [[nodiscard]]
    constexpr T &operator*() & {
        return std::get<0>(storage_);
    }

    [[nodiscard]]
    constexpr const T &operator*() const & {
        return std::get<0>(storage_);
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) & {
        using U = std::invoke_result_t<F, T &>;
        if (has_value_) {
            return Result<U, E>{
                    InPlaceValueTag{}, std::forward<F>(f)(std::get<0>(storage_))
            };
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) const & {
        using U = std::invoke_result_t<F, const T &>;
        if (has_value_) {
            return Result<U, E>{
                    InPlaceValueTag{}, std::forward<F>(f)(std::get<0>(storage_))
            };
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) && {
        using U = std::invoke_result_t<F, T &&>;
        if (has_value_) {
            return Result<U, E>{
                    InPlaceValueTag{},
                    std::forward<F>(f)(std::get<0>(std::move(storage_)))
            };
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(std::move(storage_))};
    }

    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) & {
        using U = std::invoke_result_t<F, T &>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)(std::get<0>(storage_));
        }
        return U{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) const & {
        using U = std::invoke_result_t<F, const T &>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)(std::get<0>(storage_));
        }
        return U{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) && {
        using U = std::invoke_result_t<F, T &&>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)(std::get<0>(std::move(storage_)));
        }
        return U{InPlaceErrorTag{}, std::get<1>(std::move(storage_))};
    }

    template <typename F>
    [[nodiscard]]
    constexpr auto OrElse(F &&f) & {
        using U = std::invoke_result_t<F, E &>;
        static_assert(
                std::is_same_v<typename U::ValueType, T>,
                "OrElse must preserve value type"
        );

        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(storage_));
        }
        return U{InPlaceValueTag{}, std::get<0>(storage_)};
    }


    template <typename F>
    [[nodiscard]]
    constexpr auto OrElse(F &&f) const & {
        using U = std::invoke_result_t<F, const E &>;
        static_assert(
                std::is_same_v<typename U::ValueType, T>,
                "OrElse must preserve value type"
        );

        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(storage_));
        }
        return U{InPlaceValueTag{}, std::get<0>(storage_)};
    }


    template <typename F>
    [[nodiscard]]
    constexpr auto OrElse(F &&f) && {
        using U = std::invoke_result_t<F, E &&>;
        static_assert(
                std::is_same_v<typename U::ValueType, T>,
                "OrElse must preserve value type"
        );

        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(std::move(storage_)));
        }
        return U{InPlaceValueTag{}, std::get<0>(std::move(storage_))};
    }

    template <typename F>
    constexpr Result &IfValue(F &&f) & {
        if (has_value_) {
            std::forward<F>(f)(std::get<0>(storage_));
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfValue(F &&f) const & {
        if (has_value_) {
            std::forward<F>(f)(std::get<0>(storage_));
        }
        return *this;
    }

    template <typename F>
    constexpr Result &IfError(F &&f) & {
        if (!has_value_) {
            std::forward<F>(f)(std::get<1>(storage_));
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfError(F &&f) const & {
        if (!has_value_) {
            std::forward<F>(f)(std::get<1>(storage_));
        }
        return *this;
    }
};


export template <typename E>
class Result<void, E> {
  private:
    [[no_unique_address]] bool has_value_ = false;
    std::variant<std::monostate, E> storage_;

  public:
    using ValueType = void;
    using ErrorType = E;

    // 默认构造 = 成功
    constexpr Result() noexcept : has_value_(true) {
        storage_.template emplace<0>(std::monostate{});
    }

    // 显式成功构造
    constexpr Result(InPlaceValueTag) noexcept : has_value_(true) {
        storage_.template emplace<0>(std::monostate{});
    }

    // 错误构造
    template <typename... Args>
    constexpr Result(InPlaceErrorTag, Args &&...args) : has_value_(false) {
        storage_.template emplace<1>(std::forward<Args>(args)...);
    }

    // 从错误值隐式构造
    constexpr Result(E &&error) : Result{InPlaceErrorTag{}, std::move(error)} {}
    constexpr Result(const E &error) : Result{InPlaceErrorTag{}, error} {}

    // 拷贝构造
    constexpr Result(const Result &other)
        : has_value_(other.has_value_), storage_(other.storage_) {}

    // 移动构造
    constexpr Result(
            Result &&other
    ) noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(other.has_value_), storage_(std::move(other.storage_)) {}

    // 拷贝赋值
    constexpr Result &operator=(const Result &other) {
        if (this != &other) {
            this->storage_   = other.storage_;
            this->has_value_ = other.has_value_;
        }
        return *this;
    }

    // 移动赋值
    constexpr Result &
    operator=(Result &&other) noexcept(std::is_nothrow_move_assignable_v<E>) {
        if (this != &other) {
            this->storage_   = std::move(other.storage_);
            this->has_value_ = other.has_value_;
        }
        return *this;
    }

    // 析构
    constexpr ~Result() noexcept(std::is_nothrow_destructible_v<E>) = default;

    [[nodiscard]]
    constexpr bool HasValue() const noexcept {
        return this->has_value_;
    }

    [[nodiscard]]
    constexpr bool HasError() const noexcept {
        return !this->has_value_;
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept {
        return this->has_value_;
    }

    // ========== 错误访问 ==========
    [[nodiscard]]
    constexpr E &Error() & {
        return std::get<1>(storage_);
    }

    [[nodiscard]]
    constexpr const E &Error() const & {
        return std::get<1>(storage_);
    }

    [[nodiscard]]
    constexpr E &&Error() && {
        return std::get<1>(std::move(storage_));
    }

    [[nodiscard]]
    constexpr const E &&Error() const && {
        return std::get<1>(std::move(storage_));
    }

    // ========== 函数式操作 ==========

    // Map: void -> U（F 是无参函数，返回 U）
    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) & {
        using U = std::invoke_result_t<F>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) const & {
        using U = std::invoke_result_t<F>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) && {
        using U = std::invoke_result_t<F>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, std::get<1>(std::move(storage_))};
    }

    // AndThen: void -> Result<U, E>
    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) & {
        using U = std::invoke_result_t<F>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)();
        }
        return U{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) const & {
        using U = std::invoke_result_t<F>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)();
        }
        return U{InPlaceErrorTag{}, std::get<1>(storage_)};
    }

    template <typename F>
    [[nodiscard]]
    auto AndThen(F &&f) && {
        using U = std::invoke_result_t<F>;
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "Chained Result must use same Error type"
        );
        if (has_value_) {
            return std::forward<F>(f)();
        }
        return U{InPlaceErrorTag{}, std::get<1>(std::move(storage_))};
    }

    // OrElse: 失败时处理错误
    template <typename F>
    [[nodiscard]]
    auto OrElse(F &&f) & {
        using U = std::invoke_result_t<F, E &>;
        static_assert(
                std::is_same_v<typename U::ValueType, void>,
                "OrElse must preserve void value type"
        );
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "OrElse must preserve error type"
        );
        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(storage_));
        }
        return U{InPlaceValueTag{}};
    }

    template <typename F>
    [[nodiscard]]
    auto OrElse(F &&f) const & {
        using U = std::invoke_result_t<F, const E &>;
        static_assert(
                std::is_same_v<typename U::ValueType, void>,
                "OrElse must preserve void value type"
        );
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "OrElse must preserve error type"
        );
        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(storage_));
        }
        return U{InPlaceValueTag{}};
    }

    template <typename F>
    [[nodiscard]]
    auto OrElse(F &&f) && {
        using U = std::invoke_result_t<F, E &&>;
        static_assert(
                std::is_same_v<typename U::ValueType, void>,
                "OrElse must preserve void value type"
        );
        static_assert(
                std::is_same_v<typename U::ErrorType, E>,
                "OrElse must preserve error type"
        );
        if (!has_value_) {
            return std::forward<F>(f)(std::get<1>(std::move(storage_)));
        }
        return U{InPlaceValueTag{}};
    }

    // 副作用
    template <typename F>
    constexpr Result &IfValue(F &&f) & {
        if (has_value_) {
            std::forward<F>(f)();  /// 这里没有value值可传，所以为无参函数调用
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfValue(F &&f) const & {
        if (has_value_) {
            std::forward<F>(f)();
        }
        return *this;
    }

    template <typename F>
    constexpr Result &IfError(F &&f) & {
        if (!has_value_) {
            std::forward<F>(f)(std::get<1>(storage_));
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfError(F &&f) const & {
        if (!has_value_) {
            std::forward<F>(f)(std::get<1>(storage_.error));
        }
        return *this;
    }
};

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
    return Result<std::decay_t<T>, E>{InPlaceValueTag{}, std::forward<T>(value)};
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


}  // namespace launcher
