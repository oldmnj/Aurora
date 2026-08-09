/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

#include <coroutine>
#include <utility>

export module aurora.core:task;
import launcher.base;

namespace launcher {

export template <typename T, typename E>
class Task;


namespace details {
template <typename T, typename E = Error>
struct TaskPromise {
    Result<T, E> result;
    bool done = false;

    Task<T, E> get_return_object() {
        return Task<T, E>{
                std::coroutine_handle<TaskPromise>::from_promise(*this)
        };
    }

    std::suspend_never initial_suspend() noexcept { return {}; }

    std::suspend_always final_susupend() noexcept { return {}; }

    void return_value(T &&value) {
        result = Ok<T, E>(std::move(value));
        done   = true;
    }

    void return_value(const T &value) {
        result = Ok<T, E>(value);
        done   = true;
    }

    void unhandled_exception() noexcept {
        result = Err<T, E>(
                {ErrorCategory::Runtime, ErrorCode::InternalError,
                 "unhandle exception in coroutine"}
        );

        done = true;
    }
};

template <typename E>
struct TaskPromise<void, E> {
    Result<void, E> result;
    bool done = false;

    Task<void, E> get_return_object() {
        return Task<void, E>{
                std::coroutine_handle<TaskPromise>::from_promise(*this)
        };
    }

    std::suspend_never initial_suspend() { return {}; }

    std::suspend_always final_susupend() { return {}; }

    void return_void() {
        result = Ok<void, E>();
        done   = true;
    }

    void unhandled_exception() {
        result = Err<void, E>(
                {ErrorCategory::Runtime, ErrorCode::InternalError,
                 "unhandle exception in coroutine"}
        );
        done = true;
    }
};

}  // namespace details

template <typename T, typename E = Error>
class Task {
  public:
    using promise_type = details::TaskPromise<T, E>;
    using Handle       = std::coroutine_handle<promise_type>;

    explicit Task(Handle h) : handle_(h) {}

    Task(Task &&other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    Task &operator=(Task &&other) noexcept {
        if (this != &other) {
            if (handle_)
                handle_.destroy();
            handle_       = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }


  private:
    Handle handle_;
};

}  // namespace launcher
