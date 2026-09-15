module;

#include <concepts>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

export module launcher.base:result;
import :error;

namespace launcher {

export struct InPlaceValueTag {};
export struct InPlaceErrorTag {};
export struct InPlaceOkTag {};
export struct InPlaceErrTag {};

export template <typename T, typename E>
class Result;

namespace detail {
template <typename T>
using RemoveCvRef = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename>
struct IsResultImpl : std::false_type {};

template <typename T, typename E>
struct IsResultImpl<Result<T, E>> : std::true_type {};

template <typename T>
concept IsResult = IsResultImpl<RemoveCvRef<T>>::value;

template <typename R>
struct ResultTraits;

template <typename T, typename E>
struct ResultTraits<Result<T, E>> {
    using ValueType = T;
    using ErrorType = E;
};

template <typename R>
using ResultValueOf = typename ResultTraits<RemoveCvRef<R>>::ValueType;

template <typename R>
using ResultErrorOf = typename ResultTraits<RemoveCvRef<R>>::ErrorType;

template <typename T>
concept NothrowMove = std::is_nothrow_move_constructible_v<T> &&
                      std::is_nothrow_destructible_v<T>;
}  // namespace detail

export template <typename T, typename E = Error>
class Result {
  private:
    static_assert(!std::is_reference_v<T>, "Result: T cannot be reference");
    static_assert(!std::is_reference_v<E>, "Result: E cannot be reference");
    static_assert(
            !std::is_void_v<T>, "Result: use Result<void, E> for void value"
    );
    static_assert(!std::is_void_v<E>, "Result: E cannot be void");
    static_assert(
            detail::NothrowMove<T>,
            "Result: T must be nothrow-move/destructible"
    );
    static_assert(
            detail::NothrowMove<E>,
            "Result: E must be nothrow-move/destructible"
    );

    union ResultStorage {
        T value;
        E error;
        constexpr ResultStorage() noexcept {}
        constexpr ~ResultStorage() noexcept {}
    };

    ResultStorage storage_;
    bool has_value_;

    constexpr void destroy() noexcept {
        if (has_value_) {
            std::destroy_at(std::addressof(storage_.value));
        }
        std::destroy_at(std::addressof(storage_.error));
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr void construct_value(
            Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        std::construct_at(
                std::addressof(storage_.value), std::forward<Args>(args)...
        );
        has_value_ = true;
    }

    template <typename... Args>
        requires std::constructible_from<E, Args...>
    constexpr void construct_error(
            Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<E, Args...>) {
        std::construct_at(
                std::addressof(storage_.error), std::forward<Args>(args)...
        );
        has_value_ = false;
    }

  public:
    using ValueType = T;
    using ErrorType = E;

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr explicit Result(
            InPlaceValueTag, Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : has_value_(true) {
        std::construct_at(
                std::addressof(storage_.value), std::forward<Args>(args)...
        );
    }

    template <typename... Args>
        requires std::constructible_from<E, Args...>
    constexpr explicit Result(
            InPlaceErrorTag, Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : has_value_(false) {
        std::construct_at(
                std::addressof(storage_.error), std::forward<Args>(args)...
        );
    }

    constexpr Result(const Result &other) noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_copy_constructible_v<E>
    )
        : has_value_(other.has_value_) {
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

    // static_assert保证
    constexpr Result(Result &&other) noexcept : has_value_(other.has_value_) {
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

    constexpr ~Result() noexcept { destroy(); };

    constexpr Result &operator=(const Result &other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        if (has_value_ == other.has_value_) {
            if (has_value_) {
                storage_.value = other.storage_.value;
            } else {
                storage_.error = other.storage_.error;
            }
        } else {
            Result tmp{other};
            destroy();
            if (tmp.has_value_) {
                construct_value(std::move(tmp.storage_.value));
            } else {
                construct_error(std::move(tmp.storage_.error));
            }
        }
        return *this;
    }

    constexpr Result &operator=(Result &&other) noexcept {
        if (this == std::addressof(other)) {
            return *this;
        }

        if (has_value_ == other.has_value_) {
            if (has_value_) {
                storage_.value = std::move(other.storage_.value);
            } else {
                storage_.error = std::move(other.storage_.error);
            }
        } else {
            destroy();
            if (other.has_value_) {
                construct_value(std::move(other.storage_.value));
            } else {
                construct_error(std::move(other.storage_.error));
            }
        }
        return *this;
    }


    constexpr void Swap(Result &other) noexcept {
        if (this == std::addressof(other)) {
            return;
        }
        Result tmp{std::move(other)};
        other = std::move(*this);
        *this = std::move(tmp);
    }

    friend constexpr void swap(Result &a, Result &b) noexcept { a.Swap(b); }


    [[nodiscard]]
    constexpr bool HasValue() noexcept {
        return has_value_;
    }
    [[nodiscard]]
    constexpr bool HasError() noexcept {
        return !has_value_;
    }
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    [[nodiscard]]
    constexpr decltype(auto) Value(this auto &&self) noexcept {
        if (!self.has_value_) {
            std::unreachable();
        }
        return (std::forward<decltype(self)>(self).storage_.value);
    }

    [[nodiscard]]
    constexpr decltype(auto) Error(this auto &&self) noexcept {
        if (self.has_value_) {
            std::unreachable();
        }
        return (std::forward<decltype(self)>(self).storage_.error);
    }


    [[nodiscard]]
    constexpr auto ValueIf(this auto &&self) noexcept {
        using Ptr = std::conditional_t<
                std::is_const_v<std::remove_reference_t<decltype(self)>>,
                const T *, T *>;
        return self.has_value_
                       ? static_cast<Ptr>(std::addressof(self.storage_.value))
                       : static_cast<Ptr>(nullptr);
    }

    [[nodiscard]]
    constexpr auto ErrorIf(this auto &&self) noexcept {
        using Ptr = std::conditional_t<
                std::is_const_v<std::remove_reference_t<decltype(self)>>,
                const E *, E *>;
        return !self.has_value_
                       ? static_cast<Ptr>(std::addressof(self.storage_.error))
                       : static_cast<Ptr>(nullptr);
    }

    template <typename U>
        requires std::constructible_from<T, U>
    [[nodiscard]]
    constexpr T ValueOr(this auto &&self, U &&default_value) {
        if (self.has_value_) {
            return std::forward<decltype(self)>(self).storage_.value;
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    [[nodiscard]]
    constexpr decltype(auto) operator*(this auto &&self) noexcept {
        return std::forward<decltype(self)>(self).Value;
    }

    [[nodiscard]]
    constexpr auto operator->(this auto &&self) noexcept {
        return std::addressof(std::forward<decltype(self)>(self).Value());
    }


    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Value())>
    [[nodiscard]]
    constexpr auto Map(this auto &&self, F f) {
        using Self = decltype(self);
        using U    = std::invoke_result_t<
                   F, decltype(std::forward<Self>(self).Value())>;

        if (self.has_value_) {
            if constexpr (std::is_void_v<U>) {
                std::forward<F>(f)(std::forward<Self>(self).Value());
                return Result<void, E>{InPlaceValueTag{}};
            } else {
                return Result<U, E>{
                        InPlaceValueTag{},
                        std::forward<F>(f)(std::forward<Self>(self).Value())
                };
            }
        } else {
            return Result<U, E>{
                    InPlaceErrorTag{}, std::forward<Self>(self).Error()
            };
        }
    }

    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Error())>
    [[nodiscard]]
    constexpr auto MapErr(this auto &&self, F f) {
        using Self = decltype(self);
        using F_   = std::invoke_result_t<
                  F, decltype(std::forward<Self>(self).Error())>;

        if (self.has_value_) {
            return Result<T, F_>{
                    InPlaceValueTag{}, std::forward<Self>(self).Value()
            };
        } else {
            if constexpr (std::is_void_v<F_>) {
                std::forward<F>(f)(std::forward<Self>(self).Error());
                return Result<T, void>{InPlaceErrorTag{}};
            } else {
                return Result<T, F_>{
                        InPlaceErrorTag{},
                        std::forward<F>(f)(std::forward<Self>(self).Error())
                };
            }
        }
    }

    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Value())> &&
                 detail::IsResult<std::invoke_result_t<
                         F, decltype(std::declval<Result &>().Value())>>
    [[nodiscard]] constexpr auto AndThen(this auto &&self, F &&f) {
        using Self = decltype(self);
        using U    = std::invoke_result_t<
                   F, decltype(std::forward<Self>(self).Value())>;
        using Traits = detail::ResultTraits<detail::RemoveCvRef<U>>;
        static_assert(
                std::is_same_v<typename Traits::ErrorType, E>,
                "AndThen: chained Result must use same Error type"
        );

        if (self.has_value_)
            return std::forward<F>(f)(std::forward<Self>(self).Value());
        else
            return U{InPlaceErrorTag{}, std::forward<Self>(self).Error()};
    }

    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Error())> &&
                 detail::IsResult<std::invoke_result_t<
                         F, decltype(std::declval<Result &>().Error())>>
    [[nodiscard]] constexpr auto OrElse(this auto &&self, F &&f) {
        using Self = decltype(self);
        using U    = std::invoke_result_t<
                   F, decltype(std::forward<Self>(self).Error())>;
        using Traits = detail::ResultTraits<detail::RemoveCvRef<U>>;
        using UVal   = typename Traits::ValueType;

        if (!self.has_value_) {
            return std::forward<F>(f)(std::forward<Self>(self).Error());
        } else {
            if constexpr (std::is_void_v<UVal>)
                return U{InPlaceValueTag{}};
            else
                return U{InPlaceValueTag{}, std::forward<Self>(self).Value()};
        }
    }

    template <typename F>
        requires std::invocable<F, T &>
    constexpr Result &IfValue(this Result &self, F &&f) {
        if (self.has_value_)
            std::forward<F>(f)(self.storage_.value);
        return self;
    }

    template <typename F>
        requires std::invocable<F, const T &>
    constexpr const Result &IfValue(this const Result &self, F &&f) {
        if (self.has_value_)
            std::forward<F>(f)(self.storage_.value);
        return self;
    }

    template <typename F>
        requires std::invocable<F, E &>
    constexpr Result &IfError(this Result &self, F &&f) {
        if (!self.has_value_)
            std::forward<F>(f)(self.storage_.error);
        return self;
    }

    template <typename F>
        requires std::invocable<F, const E &>
    constexpr const Result &IfError(this const Result &self, F &&f) {
        if (!self.has_value_)
            std::forward<F>(f)(self.storage_.error);
        return self;
    }

    friend constexpr bool operator==(const Result &a, const Result &b)
        requires std::equality_comparable<T> && std::equality_comparable<E>
    {
        if (a.has_value_ != b.has_value_)
            return false;
        return a.has_value_ ? (a.storage_.value == b.storage_.value)
                            : (a.storage_.error == b.storage_.error);
    }
};

export template <typename E>
class Result<void, E> {
    static_assert(!std::is_void_v<E>, "Result: E cannot be void");
    static_assert(
            detail::NothrowMove<E>,
            "Result: E must be nothrow-move/destructible"
    );

    union ResultStorage {
        E error;
        constexpr ResultStorage() noexcept {}
        constexpr ~ResultStorage() noexcept {}
    };

    ResultStorage storage_;
    bool has_value_;

    constexpr void destroy() noexcept {
        if (!has_value_)
            std::destroy_at(std::addressof(storage_.error));
    }

  public:
    using ValueType = void;
    using ErrorType = E;

    constexpr Result() noexcept : has_value_(true) {}

    constexpr explicit Result(InPlaceValueTag) noexcept : has_value_(true) {}

    template <typename... Args>
        requires std::constructible_from<E, Args...>
    constexpr explicit Result(
            InPlaceErrorTag, Args &&...args
    ) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : has_value_(false) {
        std::construct_at(
                std::addressof(storage_.error), std::forward<Args>(args)...
        );
    }

    constexpr Result(
            const Result &other
    ) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : has_value_(other.has_value_) {
        if (!has_value_)
            std::construct_at(
                    std::addressof(storage_.error), other.storage_.error
            );
    }

    constexpr Result(Result &&other) noexcept : has_value_(other.has_value_) {
        if (!has_value_)
            std::construct_at(
                    std::addressof(storage_.error),
                    std::move(other.storage_.error)
            );
    }

    constexpr ~Result() noexcept { destroy(); }

    constexpr Result &operator=(const Result &other) {
        if (this == std::addressof(other))
            return *this;
        if (has_value_ == other.has_value_) {
            if (!has_value_)
                storage_.error = other.storage_.error;
        } else {
            Result tmp(other);
            destroy();
            has_value_ = tmp.has_value_;
            if (!has_value_)
                construct_error(std::move(tmp.storage_.error));
        }
        return *this;
    }

    constexpr Result &operator=(Result &&other) noexcept {
        if (this == std::addressof(other))
            return *this;
        if (has_value_ == other.has_value_) {
            if (!has_value_)
                storage_.error = std::move(other.storage_.error);
        } else {
            destroy();
            has_value_ = other.has_value_;
            if (!has_value_)
                construct_error(std::move(other.storage_.error));
        }
        return *this;
    }

    constexpr void Swap(Result &other) noexcept {
        Result tmp(std::move(other));
        other = std::move(*this);
        *this = std::move(tmp);
    }
    friend constexpr void swap(Result &a, Result &b) noexcept { a.Swap(b); }

    [[nodiscard]] constexpr bool HasValue() const noexcept {
        return has_value_;
    }
    [[nodiscard]] constexpr bool HasError() const noexcept {
        return !has_value_;
    }
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    [[nodiscard]] constexpr decltype(auto) Error(this auto &&self) noexcept {
        if (self.has_value_)
            std::unreachable();
        return (std::forward<decltype(self)>(self).storage_.error);
    }

    [[nodiscard]] constexpr auto ErrorIf(this auto &&self) noexcept {
        using Ptr = std::conditional_t<
                std::is_const_v<std::remove_reference_t<decltype(self)>>,
                const E *, E *>;
        return !self.has_value_
                       ? static_cast<Ptr>(std::addressof(self.storage_.error))
                       : static_cast<Ptr>(nullptr);
    }

    template <typename F>
        requires std::invocable<F>
    [[nodiscard]] constexpr auto Map(this auto &&self, F &&f) {
        using Self = decltype(self);
        using U    = std::invoke_result_t<F>;

        if (self.has_value_) {
            if constexpr (std::is_void_v<U>) {
                std::forward<F>(f)();
                return Result<void, E>{InPlaceValueTag{}};
            } else {
                return Result<U, E>{InPlaceValueTag{}, std::forward<F>(f)()};
            }
        } else {
            return Result<U, E>{
                    InPlaceErrorTag{}, std::forward<Self>(self).Error()
            };
        }
    }

    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Error())>
    [[nodiscard]] constexpr auto MapErr(this auto &&self, F &&f) {
        using Self = decltype(self);
        using F_   = std::invoke_result_t<
                  F, decltype(std::forward<Self>(self).Error())>;
        static_assert(!std::is_void_v<F_>, "MapErr: F must return non-void");

        if (self.has_value_)
            return Result<void, F_>{InPlaceValueTag{}};
        else
            return Result<void, F_>{
                    InPlaceErrorTag{},
                    std::forward<F>(f)(std::forward<Self>(self).Error())
            };
    }

    template <typename F>
        requires std::invocable<F> && detail::IsResult<std::invoke_result_t<F>>
    [[nodiscard]] constexpr auto AndThen(this auto &&self, F &&f) {
        using Self   = decltype(self);
        using U      = std::invoke_result_t<F>;
        using Traits = detail::ResultTraits<detail::RemoveCvRef<U>>;
        static_assert(
                std::is_same_v<typename Traits::ErrorType, E>,
                "AndThen: chained Result must use same Error type"
        );

        if (self.has_value_)
            return std::forward<F>(f)();
        else
            return U{InPlaceErrorTag{}, std::forward<Self>(self).Error()};
    }

    template <typename F>
        requires std::invocable<F, decltype(std::declval<Result &>().Error())> &&
                 detail::IsResult<std::invoke_result_t<
                         F, decltype(std::declval<Result &>().Error())>>
    [[nodiscard]] constexpr auto OrElse(this auto &&self, F &&f) {
        using Self = decltype(self);
        using U    = std::invoke_result_t<
                   F, decltype(std::forward<Self>(self).Error())>;
        using Traits = detail::ResultTraits<detail::RemoveCvRef<U>>;
        using UVal   = typename Traits::ValueType;

        if (!self.has_value_) {
            return std::forward<F>(f)(std::forward<Self>(self).Error());
        } else {
            if constexpr (std::is_void_v<UVal>)
                return U{InPlaceValueTag{}};
            else
                return U{InPlaceValueTag{}, /* void -> U 需要 f 返回值 */};
            // 注：void Result 的 OrElse 成功分支无法直接产生非 void U 的值，
            //     所以这里要求 UVal 必须为 void；如需转换请用 AndThen/Map。
        }
    }

    template <typename F>
        requires std::invocable<F>
    constexpr Result &IfValue(this Result &self, F &&f) {
        if (self.has_value_)
            std::forward<F>(f)();
        return self;
    }

    template <typename F>
        requires std::invocable<F>
    constexpr const Result &IfValue(this const Result &self, F &&f) {
        if (self.has_value_)
            std::forward<F>(f)();
        return self;
    }

    template <typename F>
        requires std::invocable<F, E &>
    constexpr Result &IfError(this Result &self, F &&f) {
        if (!self.has_value_)
            std::forward<F>(f)(self.storage_.error);
        return self;
    }

    template <typename F>
        requires std::invocable<F, const E &>
    constexpr const Result &IfError(this const Result &self, F &&f) {
        if (!self.has_value_)
            std::forward<F>(f)(self.storage_.error);
        return self;
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
