/**
 * @file system.cc
 * @brief 有关架构及版本信息都详细实现
 * @anthor oldmnj
 * @date 2026-08-15
 */
module;

#include <bit>

module launcher.base;

namespace launcher {
Platform CurrentPlatform() noexcept {
#if defined(_WIN32)
    return Platform::Windows;
#elif defined(__ANDROID__)
    return Platform::Android;
#elif defined(__APPLE__)
    return Platform::MacOS;
#elif defined(__linux__)
    return Platform::Linux;
#else
    return Platform::Unknown;
#endif
}

Architecture CurrentArchitecture() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    return Architecture::X64;
#elif defined(__i386__) || defined(_M_IX86)
    return Architecture::X86;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return Architecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
    return Architecture::ARM;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return Architecture::RISCV64;
#else
    return Architecture::Unknown;
#endif
}

bool Is64Bit() noexcept { return sizeof(void *) == 8; }

bool IsLittleEndian() noexcept {
    return std::endian::native == std::endian::little;
}

constexpr StringView ToString(Platform p) noexcept {
    switch (p) {
    case Platform::Windows:
        return "windows";
    case Platform::Linux:
        return "linux";
    case Platform::MacOS:
        return "macos";
    case Platform::Android:
        return "android";
    case Platform::IOS:
        return "ios";
    default:
        return "unknown";
    }
}

constexpr StringView ToString(Architecture arch) noexcept {
    switch (arch) {
    case Architecture::ARM:
        return "arm";
    case Architecture::ARM64:
        return "arm64";
    case Architecture::X64:
        return "x64";
    case Architecture::X86:
        return "x86";
    case Architecture::RISCV64:
        return "riscv64";
    default:
        return "unknown";
    }
}
}  // namespace launcher
