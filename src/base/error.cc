module;

#include <source_location>
#include <string>

module launcher.base;

namespace launcher {
Error::Error(
        ErrorCategory category, ErrorCode code, StringView message, std::source_location location)
    : code_(code), category_(category), message_(message), location_(location) {}

Error::Error(ErrorCategory category, ErrorCode code, StringView message, SharedPtr<Error> cause,
        std::source_location location)
    : code_(code), category_(category), message_(message), location_(location), cause_(cause) {}

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

[[nodiscard]]
constexpr StringView Error::ToString(ErrorCategory category) noexcept {
    switch (category) {
    case ErrorCategory::None:
        return "None";
    case ErrorCategory::Config:
        return "Config";
    case ErrorCategory::IO:
        return "IO";
    case ErrorCategory::Minecraft:
        return "Minecraft";
    case ErrorCategory::Network:
        return "Network";
    case ErrorCategory::Runtime:
        return "Runtime";
    case ErrorCategory::Security:
        return "Security";
    case ErrorCategory::System:
        return "System";
    default:
        return "None";
        break;
    }
}

[[nodiscard]]
String Error::ToString() const {
    StringView filename = location_.file_name();
    if (auto pos = filename.rfind('/'); pos != StringView::npos) {
        filename.remove_prefix(pos + 1);
    }
#ifdef _WIN32
    if (auto pos = filename.rfind('\\'); pos != std::string_view::npos) {
        filename.remove_prefix(pos + 1);
    }
#endif

    String result;
    result.reserve(128);
    result.append("[")
            .append(ToString(category_))
            .append("] ")
            .append(ToString(code_))
            .append(" message: ")
            .append(message_)
            .append(", at ")
            .append(filename)
            .append(":")
            .append(std::to_string(location_.line()));

    return String(result);
}

}  // namespace launcher
