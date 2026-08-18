/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
/**
 * @file launcher.base.types.cppm
 * @brief 提供统一类型
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

export module launcher.base:types;

export namespace launcher {
// ---------------------------------------
// 整数类型
// 有符号整数
using i8            = std::int8_t;
using i16           = std::int16_t;
using i32           = std::int32_t;
using i64           = std::int64_t;
// 无符号整数
using u8            = std::uint8_t;
using u16           = std::uint16_t;
using u32           = std::uint32_t;
using u64           = std::uint64_t;
// 特殊整数类型
using isize         = std::ptrdiff_t;
using usize         = std::size_t;

// ========== 浮点型 ==========
using f32           = float;
using f64           = double;
// ========== 字节 ===========
using Byte          = std::byte;
using ByteSpan      = std::span<std::byte>;
using Bytes         = std::vector<std::byte>;
// ========== 字符串 ==========
using String        = std::string;
using StringView    = std::string_view;
using WString       = std::wstring;
using WStringView   = std::wstring_view;
using U8String      = std::u8string;
using U8StringView  = std::u8string_view;
using Path          = std::filesystem::path;
using StringLiteral = const char *;
using CString       = const char *;
// ========== Span 特化 ==========
using U8Span        = std::span<std::uint8_t>;
using CharSpan      = std::span<char>;
using ConstByteSpan = std::span<const std::byte>;

// ========== 智能指针 ==========
template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

// ========== 容器视图 ==========
template <typename T>
using Span = std::span<T>;

template <typename T>
using ConstSpan = std::span<const T>;

template <typename T>
using Optional = std::optional<T>;

template <typename T>
using Vector = std::vector<T>;
}  // namespace launcher
