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

export class Error {
  public:
    Error(ErrorCode code, StringView message,
            std::source_location location = std::source_location::current());
    [[nodiscard]]
    ErrorCode Code() const noexcept;
    [[nodiscard]]
    StringView Message() const noexcept;
    [[nodiscard]]
    const std::source_location &Location() const noexcept;

    [[nodiscard]]
    constexpr StringView ToString(ErrorCode) noexcept;

  private:
    ErrorCode code_;
    String message_;
    std::source_location location_;
};

export template <typename T>
using Result = std::expected<T, Error>;

}  // namespace launcher
