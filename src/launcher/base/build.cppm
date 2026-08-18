/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
/**
 * @file launcher.base.build.cppm
 * @brief 编译时的信息
 * @anthor oldmnj
 * @date 2026-08-15
 */
export module launcher.base:build;
import :types;
import :platform;


namespace launcher {
/**
 * @brief 构建信息
 */
export struct BuildInfo {
    Version version;  ///< 启动器内核版本

    StringView name;  ///< 项目名称

    StringView compiler;  ///< 记录编译器及其版本，建议编译时使用宏注入

    StringView build_type;  ///< 编译类型，建议编译时使用宏注入

    StringView build_date;  ///< 编译时间，来源: 宏: __DATE__, __TIME__

    Platform platform;  ///< 系统名称，直接复制Platform

    Architecture architecture;  ///< 架构名称
};
/*
constexpr BuildInfo info{
        .version      = {0, 1, 0},
        .name         = "AuroraLauncherCore",
        .compiler     = "clang",
        .build_type   = "release",
        .build_date   = __DATE__,
        .platform     = Platform{},
        .architecture = Architecture{},
};
*/
// export const BuildInfo &CurrentBuildInfo() noexcept;
// 返回引用类型，原因：BuildInfo全局唯一

}  // namespace launcher
