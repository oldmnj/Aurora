#include <fmt/base.h>
module;

#include <algorithm>
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <source_location>
#include <string>
#include <utility>

module launcher.base;

namespace launcher {
Error::Error(
        ErrorCategory category, ErrorCode code, String message,
        std::source_location location
)
    : code_(code),
      category_(category),
      message_(message),
      location_(location) {}

Error::Error(
        ErrorCategory category, ErrorCode code, String message,
        SharedPtr<const Error> cause, std::source_location location
)
    : code_(code),
      category_(category),
      message_(message),
      location_(location),
      cause_(cause) {}


[[nodiscard]] ErrorCode Error::Code() const noexcept { return this->code_; }

[[nodiscard]] StringView Error::Message() const noexcept {
    return this->message_;
}

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
    }
}

template <typename... Args>
[[nodiscard]]
Error Error::Format(
        ErrorCategory category, ErrorCode code, fmt::format_string<Args...> fmt,
        Args &&...args, std::source_location location
) {
    return Error{
            category, code,
            std::move(fmt::format(fmt, std::forward<Args>(args)...)), location
    };
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
    }
}

[[nodiscard]]
String Error::ToString() const {
    String out_str;
    out_str.reserve(
            (128 * (ChainDepth() > 8 ? 9 : (ChainDepth() + 1))) +
            ((ChainDepth() > 8) ? 72 : 0)
    );

    auto it = std::back_inserter(out_str);

    fmt::format_to(
            it, "Error[{}:{}] {}", ToString(category_), ToString(code_),
            message_
    );
    if (location_.file_name() != nullptr && location_.line() != 0) {
        fmt::format_to(
                it, "\n  at {}:{}", location_.file_name(), location_.line()
        );
    }

    if (this->HasCause()) {
        constexpr usize kMaxChainDepth = 8;
        usize depth                    = 0;
        const Error *cur               = cause_.get();

        while (cur != nullptr) {
            if (depth >= kMaxChainDepth) {
                fmt::format_to(
                        it,
                        "\n  ...(Chain is too long(ChainDepth[{}] > 8), has "
                        "been truncated)",
                        ChainDepth()
                );
                break;
            }

            fmt::format_to(
                    it, "\n  caused by: Error[{}:{}] {}",
                    ToString(cur->category_), ToString(cur->code_),
                    cur->message_
            );
            if (cur->location_.file_name() != nullptr &&
                cur->location_.line() != 0) {
                fmt::format_to(
                        it, "\n    at {}:{}", cur->location_.file_name(),
                        cur->location_.line()
                );
            }
            cur = cur->cause_.get();
            ++depth;
        }
    }
}

auto Error::WithCause(Error cause) -> Error {
    return Error{
            this->category_, this->code_, this->message_,
            std::make_shared<const Error>(std::move(cause)), this->location_
    };
}

auto Error::rWithCause(Error cause) && -> Error {
    Error out_err{std::move(*this)};
    out_err.cause_ = std::make_shared<const Error>(std::move(cause));
    return out_err;
}

auto Error::HasCause() const -> bool { return this->cause_ != nullptr; }

auto Error::Cause() const -> const Error * { return cause_.get(); }

auto Error::ChainDepth() const -> usize {
    usize depth      = 0;
    const Error *cur = cause_.get();
    while (cur != nullptr) {
        ++depth;
        cur = cur->cause_.get();
    }
    return depth;
}

}  // namespace launcher
