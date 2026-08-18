/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
/**
 * @file launcher.base.platform.cppm
 * @brief 判断目编译时的版本号及目标平台的架构与名称
 * @author oldmnj
 * @date 2026-08-15
 */
module;

export module launcher.base:platform;
import :types;

namespace launcher {
/**
 * @brief 项目版本都存储
 * @details 此结构体用来确认编译后项目的版本号，也可为CLI的--version子命令服务
 */
export struct Version {
    u32 major{};
    u32 minor{};
    u32 patch{};

    constexpr auto operator<=>(const Version &) const = default;

    /**
     * @brief 版本号转为字符串
     * @return 格式为: "major.minor.patch" 的字符串
     */
    [[nodiscard]]
    String ToString() const;
};

/**
 * @brief 目标平台名称的详细列举
 */
export enum class Platform {
    Windows,  //
    Linux,
    MacOS,
    Android,
    IOS,
    Unknown
};

export enum class Architecture { X86, X64, ARM, ARM64, RISCV64, Unknown };

export [[nodiscard]] Platform CurrentPlatform() noexcept;

export [[nodiscard]] Architecture CurrentArchitecture() noexcept;

export [[nodiscard]] bool Is64Bit() noexcept;

export [[nodiscard]] bool IsLittleEndian() noexcept;

export constexpr StringView ToString(Platform) noexcept;
export constexpr StringView ToString(Architecture) noexcept;
}  // namespace launcher
