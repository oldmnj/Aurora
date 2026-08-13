module;

#include <memory>
#include <new>
#include <source_location>
#include <type_traits>
#include <utility>

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

/*
export template <typename T>
using Result = std::expected<T, Error>;
*/

namespace details {
template <typename T, typename E>
union ResultStorage {
    struct Dummy {
        char _;
    } dummy;  ///< 这里必须要有一个非value和error的变量，否则默认构造是UB
    T value;
    E error;

    ResultStorage() : dummy{} {}
    ~ResultStorage() {}

    constexpr void destroy(bool has_value) noexcept(
            std::is_nothrow_destructible_v<T> &&
            std::is_nothrow_destructible_v<E>
    ) {
        if (has_value) {
            std::destroy_at(std::addressof(value));
        } else {
            std::destroy_at(std::addressof(error));
        }
    }
};

template <typename E>
union ResultStorage<void, E> {
    E error;

    ResultStorage() {}
    ~ResultStorage() {}

    constexpr void
    destroy(bool has_value) noexcept(std::is_nothrow_destructible_v<E>) {
        if (!has_value) {
            std::destroy_at(std::addressof(error));
        }
    }
};

}  // namespace details

export struct InPlaceValueTag {};
export struct InPlaceErrorTag {};

export template <typename T, typename E = Error>
class Result {
  private:
    [[no_unique_address]] bool has_value_ = false;
    details::ResultStorage<T, E> storage_;

    void destroy() { this->storage_.destroy(this->has_value_); }

  public:
    using ValueType = T;
    using ErrorType = E;

    template <typename... Args>
    constexpr Result(InPlaceValueTag, Args &&...args) : has_value_(true) {
        std::construct_at(
                std::addressof(storage_.value), std::forward<Args>(args)...
        );
    }

    template <typename... Args>
    constexpr Result(InPlaceErrorTag, Args &&...args) : has_value_(false) {
        std::construct_at(
                std::addressof(storage_.error), std::forward<Args>(args)...
        );
    }

    constexpr Result(T &&value) : Result{InPlaceValueTag{}, std::move(value)} {}
    constexpr Result(const T &value) : Result{InPlaceValueTag{}, value} {}

    constexpr Result(E &&error) : Result{InPlaceErrorTag{}, std::move(error)} {}
    constexpr Result(const E &error) : Result{InPlaceErrorTag{}, error} {}

    constexpr Result(const Result &other) : has_value_(other.has_value_) {
        if (has_value_) {
            std::construct_at(
                    std::addressof(storage_.value), other.storage_.value
            );
        } else {
            std::construct_at(
                    std::addressof(storage_.error), other.storage_.error
            );
        }
    }

    constexpr Result(Result &&other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_assignable_v<E>
    )
        : has_value_(other.has_value_) {
        if (has_value_) {
            std::construct_at(
                    std::addressof(storage_.value),
                    std::move(other.storage_.value)
            );
        } else {
            std::construct_at(
                    std::addressof(storage_.error),
                    std::move(other.storage_.error)
            );
        }
    }

    constexpr Result &operator=(const Result &other) {
        if (this != &other) {
            this->destroy();  // 不析构直接覆盖是未定义行为
            this->has_value_ = other.has_value_;
            if (this->has_value_) {
                std::construct_at(
                        std::addressof(storage_.value), other.storage_.value
                );
            } else {
                std::construct_at(
                        std::addressof(storage_.error), other.storage_.error
                );
            }
        }
        return *this;
    }

    constexpr Result &operator=(Result &&other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<E>
    ) {
        if (this != &other) {
            this->destroy();
            this->has_value_ = other.has_value_;
            if (this->has_value_) {
                std::construct_at(
                        std::addressof(storage_.value),
                        std::move(other.storage_.value)
                );
            } else {
                std::construct_at(
                        std::addressof(storage_.error),
                        std::move(other.storage_.error)
                );
            }
        }
        return *this;
    }

    constexpr ~Result() noexcept(
            std::is_nothrow_destructible_v<T> &&
            std::is_nothrow_destructible_v<E>
    ) {
        this->destroy();
    }

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
        return storage_.value;
    }

    [[nodiscard]]
    constexpr const T &Value() const & {
        return storage_.value;
    }

    [[nodiscard]]
    constexpr T &&Value() && {
        return std::move(storage_.value);
    }

    [[nodiscard]]
    constexpr const T &&Value() const && {
        return std::move(storage_.value);
    }


    template <typename U>
    [[nodiscard]]
    constexpr T ValueOr(U &&default_value) const & {
        return has_value_ ? storage_.value
                          : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    [[nodiscard]]
    constexpr T ValueOr(U &&default_value) && {
        return has_value_ ? std::move(storage_.value)
                          : static_cast<T>(std::forward<U>(default_value));
    }

    [[nodiscard]]
    constexpr E &Error() & {
        return storage_.error;
    }

    [[nodiscard]]
    constexpr const E &Error() const & {
        return storage_.error;
    }

    [[nodiscard]]
    constexpr E &&Error() && {
        return std::move(storage_.error);
    }

    [[nodiscard]]
    constexpr T *operator->() {
        return std::addressof(storage_.value);
    }

    [[nodiscard]]
    constexpr const T *operator->() const {
        return std::addressof(storage_.value);
    }

    [[nodiscard]]
    constexpr T &operator*() & {
        return storage_.value;
    }

    [[nodiscard]]
    constexpr const T &operator*() const & {
        return storage_.value;
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) & {
        using U = std::invoke_result_t<F, T &>;
        if (has_value_) {
            return Result<U, E>{
                    InPlaceValueTag{}, std::forward<F>(f)(storage_.value)
            };
        }
        return Result<U, E>{InPlaceErrorTag{}, storage_.error};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) const & {
        using U = std::invoke_result_t<F, const T &>;
        if (has_value_) {
            return Result<U, E>{
                    InPlaceValueTag{}, std::forward<F>(f)(storage_.value)
            };
        }
        return Result<U, E>{InPlaceErrorTag{}, storage_.error};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) && {
        using U = std::invoke_result_t<F, T &&>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, std::move(storage_.error)};
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
            return std::forward<F>(f)(storage_.value);
        }
        return U{InPlaceErrorTag{}, storage_.error};
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
            return std::forward<F>(f)(storage_.value);
        }
        return U{InPlaceErrorTag{}, std::move(storage_.error)};
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
            return std::forward<F>(f)(std::move(storage_.value));
        }
        return U{InPlaceErrorTag{}, std::move(storage_.error)};
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
            return std::forward<F>(f)(storage_.error);
        }
        return U{InPlaceValueTag{}, storage_.value};
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
            return std::forward<F>(f)(storage_.error);
        }
        return U{InPlaceValueTag{}, storage_.value};
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
            return std::forward<F>(f)(std::move(storage_.error));
        }
        return U{InPlaceValueTag{}, std::move(storage_.value)};
    }

    template <typename F>
    constexpr Result &IfValue(F &&f) & {
        if (has_value_) {
            std::forward<F>(f)(storage_.value);
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfValue(F &&f) const & {
        if (has_value_) {
            std::forward<F>(f)(storage_.value);
        }
        return *this;
    }

    template <typename F>
    constexpr Result &IfError(F &&f) & {
        if (!has_value_) {
            std::forward<F>(f)(storage_.error);
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfError(F &&f) const & {
        if (!has_value_) {
            std::forward<F>(f)(storage_.error);
        }
        return *this;
    }
};


export template <typename E>
class Result<void, E> {
  private:
    [[no_unique_address]] bool has_value_ = false;
    details::ResultStorage<void, E> storage_;

    void destroy() { this->storage_.destroy(this->has_value_); }

  public:
    using ValueType = void;
    using ErrorType = E;

    // 默认构造 = 成功
    constexpr Result() noexcept : has_value_(true) {}

    // 显式成功构造
    constexpr Result(InPlaceValueTag) noexcept : has_value_(true) {}

    // 错误构造
    template <typename... Args>
    constexpr Result(InPlaceErrorTag, Args &&...args) : has_value_(false) {
        std::construct_at(
                std::addressof(storage_.error), std::forward<Args>(args)...
        );
    }

    // 从错误值隐式构造
    constexpr Result(E &&error) : Result{InPlaceErrorTag{}, std::move(error)} {}
    constexpr Result(const E &error) : Result{InPlaceErrorTag{}, error} {}

    // 拷贝构造
    constexpr Result(const Result &other) : has_value_(other.has_value_) {
        if (!has_value_) {
            std::construct_at(
                    std::addressof(storage_.error), other.storage_.error
            );
        }
    }

    // 移动构造
    constexpr Result(
            Result &&other
    ) noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(other.has_value_) {
        if (!has_value_) {
            std::construct_at(
                    std::addressof(storage_.error),
                    std::move(other.storage_.error)
            );
        }
    }

    // 拷贝赋值
    constexpr Result &operator=(const Result &other) {
        if (this != &other) {
            this->destroy();
            this->has_value_ = other.has_value_;
            if (!this->has_value_) {
                std::construct_at(
                        std::addressof(storage_.error), other.storage_.error
                );
            }
        }
        return *this;
    }

    // 移动赋值
    constexpr Result &
    operator=(Result &&other) noexcept(std::is_nothrow_move_assignable_v<E>) {
        if (this != &other) {
            this->destroy();
            this->has_value_ = other.has_value_;
            if (!this->has_value_) {
                std::construct_at(
                        std::addressof(storage_.error),
                        std::move(other.storage_.error)
                );
            }
        }
        return *this;
    }

    // 析构
    constexpr ~Result() noexcept(std::is_nothrow_destructible_v<E>) {
        this->destroy();
    }

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
        return storage_.error;
    }

    [[nodiscard]]
    constexpr const E &Error() const & {
        return storage_.error;
    }

    [[nodiscard]]
    constexpr E &&Error() && {
        return std::move(storage_.error);
    }

    [[nodiscard]]
    constexpr const E &&Error() const && {
        return std::move(storage_.error);
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
        return Result<U, E>{InPlaceErrorTag{}, storage_.error};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) const & {
        using U = std::invoke_result_t<F>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, storage_.error};
    }

    template <typename F>
    [[nodiscard]]
    auto Map(F &&f) && {
        using U = std::invoke_result_t<F>;
        if (has_value_) {
            return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
        }
        return Result<U, E>{InPlaceErrorTag{}, std::move(storage_.error)};
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
        return U{InPlaceErrorTag{}, storage_.error};
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
        return U{InPlaceErrorTag{}, storage_.error};
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
        return U{InPlaceErrorTag{}, std::move(storage_.error)};
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
            return std::forward<F>(f)(storage_.error);
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
            return std::forward<F>(f)(storage_.error);
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
            return std::forward<F>(f)(std::move(storage_.error));
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
            std::forward<F>(f)(storage_.error);
        }
        return *this;
    }

    template <typename F>
    constexpr const Result &IfError(F &&f) const & {
        if (!has_value_) {
            std::forward<F>(f)(storage_.error);
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
