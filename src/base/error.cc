module;

#include <source_location>

module launcher.base;

namespace launcher {
Error::Error(
        ErrorCategory category, ErrorCode code, StringView message, std::source_location location)
    : code_(code), category_(category), message_(message), location_(location) {}

[[nodiscard]] ErrorCode Error::Code() const noexcept { return this->code_; }

[[nodiscard]] StringView Error::Message() const noexcept { return this->message_; }

[[nodiscard]] const std::source_location &Error::Location() const noexcept {
    return this->location_;
}

[[nodiscard]]
ErrorCategory Error::Category() const noexcept {
    return this->category_;
}

[[nodiscard]]
constexpr StringView Error::ToString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::Ok:
        return "Ok";

    case ErrorCode::InternalError:
        return "InternalError";

    case ErrorCode::InvalidArgument:
        return "InvalidArgument";

    case ErrorCode::InvalidFormat:
        return "InvalidFormat";

    case ErrorCode::InvalidState:
        return "InvalidState";

    case ErrorCode::ChecksumMismatch:
        return "ChecksumMismatch";

    case ErrorCode::ConnectionFailed:
        return "ConnectionFailed";

    case ErrorCode::DownloadFailed:
        return "DownloadFailed";

    case ErrorCode::FileNotFound:
        return "FileNotFound";

    case ErrorCode::IOError:
        return "IOError";

    case ErrorCode::NetworkError:
        return "NetworkError";

    case ErrorCode::ParseError:
        return "ParseError";

    case ErrorCode::PermissionDenied:
        return "PermissionDenied";

    case ErrorCode::Timeout:
        return "Timeout";

    case ErrorCode::Unknown:
        return "Unknown";

    case ErrorCode::Unsupported:
        return "Unsupported";

    default:
        return "Unknown";
        break;
    }
}
}  // namespace launcher
