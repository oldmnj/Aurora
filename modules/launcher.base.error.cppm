module;

#include <expected>
#include <source_location>

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
    Error(ErrorCategory category, ErrorCode code, StringView message, SharedPtr<Error> cause,
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
    String ToString() const noexcept;

    [[nodiscard]]
    ErrorCategory Category() const noexcept;

  private:
    ErrorCode code_;
    ErrorCategory category_;
    String message_;
    std::source_location location_;
    Optional<SharedPtr<Error>> cause_;
};


export template <typename T>
using Result = std::expected<T, Error>;

}  // namespace launcher
