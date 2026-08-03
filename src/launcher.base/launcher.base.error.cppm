module;

#include <expected>
#include <new>
#include <source_location>
#include <type_traits>
#include <utility>

export module launcher.base:error;
import :types;

namespace launcher {
export enum class ErrorCode {
    // 通用错误
    Ok,

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
          SharedPtr<Error> cause,
          std::source_location location = std::source_location::current());

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
    Optional<SharedPtr<Error>> cause_;
};

/*
export template <typename T>
using Result = std::expected<T, Error>;
*/

namespace details {
template <typename T, typename E>
union ResultStorage {
    T value;
    E error;

    ResultStorage() {}
    ~ResultStorage() {}

    void destroy(bool has_value) {
        if (has_value) {
            this->value.~T();
        } else {
            this->error.~E();
        }
    }
};

template <typename E>
union ResultStorage<void, E> {
    E error;

    ResultStorage() {}
    ~ResultStorage() {}

    void destroy(bool has_value) {
        if (!has_value) {
            this->error.~E();
        }
    }
};

}  // namespace details

export struct InPlaceValueTag {};
export struct InPlaceErrorTag {};

export template <typename T, typename E = Error>
class Result {
  private:
    bool has_value_;
    details::ResultStorage<T, E> storage_;

    void destroy() { this->storage_.destroy(this->has_value_); }

  public:
    using ValueType = T;
    using ErrorType = E;

    template <typename... Args>
    constexpr Result(InPlaceValueTag, Args &&...args) : has_value_(true) {
        new (&storage_.value) T{std::forward<Args>(args)...};
    }

    template <typename... Args>
    constexpr Result(InPlaceErrorTag, Args &&...args) : has_value_(false) {
        new (&storage_.error) E{std::forward<Args>(args)...};
    }

    Result(const Result &other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(other.storage_.value);
        } else {
            new (&storage_.error) E(other.storage_.error);
        }
    }

    Result(Result &&other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_move_assignable_v<E>
    )
        : has_value_(other.has_value_) {
        if (has_value_) {
            new (&storage_.value) T(std::move(other.storage_.value));
        } else {
            new (&storage_.error) E(std::move(other.storage_.error));
        }
    }

    Result &operator=(const Result &other) {
        if (this != &other) {
            this->destroy();  // 不析构直接覆盖是未定义行为
            this->has_value_ = other.has_value_;
            if (this->has_value_) {
                new (&storage_.value) T(other.storage_.value);
            } else {
                new (&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
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

    [[nodiscard]]
    constexpr T &Value() & {
        return storage_.value;
    }

    [[nodiscard]]
    constexpr T &&Value() && {
        return std::move(storage_.value);
    }

    template <typename U>
    [[nodiscard]]
    constexpr T ValueOr(U &&default_value) const & {
        return has_value_ ? storage_.value
                          : static_cast<T>(std::forward<U>(default_value));
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
    Result IfValue(F &&f) & {
        if (has_value_) {
            std::forward<F>(f)(storage_.value);
        }
        return *this;
    }

    template <typename F>
    Result IfError(F &&f) & {
        if (!has_value_) {
            std::forward<F>(f)(storage_.error);
        }
        return *this;
    }
};

}  // namespace launcher
