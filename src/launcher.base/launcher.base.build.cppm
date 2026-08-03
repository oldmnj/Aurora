module;

export module launcher.base:build;
import :types;
import :platform;


namespace launcher {
export struct BuildInfo {
    Version version;  // kernelVersion

    StringView name;  // projectName(KernelName)

    StringView compiler;  // 记录compiler，来源__clang_version__宏，__VERSION__等等

    StringView build_type;  // Debug, Release等等，建议使用xmake注入宏

    StringView build_date;  // 编译时间，来源: 宏: __DATE__, __TIME__

    Platform platform;  // 直接复制Platform

    Architecture architecture;  // 同上
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
// 返回引用类型，原因：BuildIofo全局唯一

}  // namespace launcher
