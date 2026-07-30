/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

export module launcher.base:platform;
import :types;

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
