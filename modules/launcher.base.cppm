module;

#include <expected>
#include <source_location>

export module launcher.base;
export import :types;


namespace launcher {
export enum class ErrorCode {
    Ok,
    InvalidArgument,
    InvalidState,
    Unsupported,
    IOError,
    NetworkError,
    DownloadFailed,
    Timeout,
    NotFound,
    AlreadyExists,
    PermissionDenied,
    ParseError,
    InternalError
};
export class Error {
  public:
  private:
    ErrorCode code_;
    String message_;
    String module_;
    std::source_location location_;
};

export template <typename T>
using Result = std::expected<T, Error>;
}  // namespace launcher
/*
export class Logger {
    static void Initialize();
    static void Shutdown();
    static void Trace();
    static void Debug();
    static void Info();
    static void Warn();
    static void Error();
    static void Critical();
};
*/

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
