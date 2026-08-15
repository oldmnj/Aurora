module;

#include <array>
#include <chrono>
#include <cstring>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>
#include <utility>

module aurora.service;

namespace launcher {

auto UUIDGenerator::V4() -> Result<UUID> {
    auto bytes_result = Random::Bytes(16);
    if (bytes_result.HasError()) {
        return Err<UUID>(std::move(bytes_result.Error()));
    }

    auto bytes = bytes_result.Value();
    std::array<u8, 16> data;
    for (usize i = 0; i < 16; i++) {
        data[i] = static_cast<u8>(bytes[i]);
    }

    data[6] = (data[6] & 0x0F) | (static_cast<u8>(UUIDVersion::Random) << 4);
    data[8] = (data[8] & 0x3F) | 0x80;

    return Ok(UUID{data});
}

auto UUIDGenerator::V7() -> Result<UUID> {
    auto bytes_result = Random::Bytes(16);
    if (bytes_result.HasError()) {
        return Err<UUID>(std::move(bytes_result.Error()));
    }

    auto bytes = bytes_result.Value();

    std::array<u8, 16> data;
    for (usize i = 0; i < 16; i++) {
        data[i] = static_cast<u8>(bytes[i]);
    }

    auto now      = std::chrono::system_clock::now();
    u64 timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()
    )
                            .count();

    data[0] = (timestamp >> 40) & 0xFF;
    data[1] = (timestamp >> 32) & 0xFF;
    data[2] = (timestamp >> 24) & 0xFF;
    data[3] = (timestamp >> 16) & 0xFF;
    data[4] = (timestamp >> 8) & 0xFF;
    data[5] = timestamp & 0xFF;

    data[6] = (data[6] & 0x0F) | (static_cast<u8>(UUIDVersion::UnixTime) << 4);
    data[8] = (data[8] & 0x3F) | 0x80;
    return Ok(UUID{data});
}

Result<UUID> UUID::Parse(StringView uuid_str) {
    if (uuid_str.length() != 36) {
        return Err<UUID>(
                {ErrorCategory::Parse, ErrorCode::InvalidArgument,
                 "invalid UUID length"}
        );
    }
    std::array<u8, 16> data{};
    usize pos = 0;

    for (int i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            if (pos >= uuid_str.size() || uuid_str[pos] != '-') {
                // clang-format off
                return Err<UUID>(
                        {ErrorCategory::Parse, ErrorCode::InvalidArgument,
                         fmt::format(
                                 "invalid UUID format: missing hyphen at position {}",
                                 pos
                         )}
                );
                //clang-format on
            }
            ++pos;
        }
        
        if (pos + 1 >= uuid_str.size()) {
            return Err<UUID>({
                ErrorCategory::Parse,
                ErrorCode::InvalidArgument,
                "invalid UUID format: unexpected end of string"
            });
        }

        u8 byte = 0;
        for (int j = 0; j < 2; j++) {
            char uuid_char = uuid_str[pos++];
            if (uuid_char >= '0' && uuid_char <= '9') {
                byte = (byte << 4) | static_cast<u8>(uuid_char - '0');
            } else if (uuid_char >= 'a' && uuid_char <= 'f') {
                byte = (byte << 4) | static_cast<u8>(uuid_char - 'a' + 10);
            } else if (uuid_char >= 'A' && uuid_char <= 'F') {
                byte = (byte << 4) | static_cast<u8>(uuid_char - 'A' + 10);
            } else {
                // clang-format off
                return Err<UUID>({
                    ErrorCategory::Parse,
                    ErrorCode::InvalidArgument,
                    fmt::format("invalid hex character: '{}' at position {}", uuid_char, pos - 1)
                });
                //clang-format on
            }
        }
        data[i] = byte;
    }
    u8 version_byte = data[6] >> 4;
    if (version_byte == 0 || version_byte > 7) {
        // clang-format off
        return Err<UUID>(
                {ErrorCategory::Parse, ErrorCode::InvalidArgument,
                 fmt::format("invalid UUID version: {}", version_byte)}
        );
        // clang-format on
    }

    return Ok(UUID{data});
}

String UUID::ToString() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (usize i = 0; i < data_.size(); i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            oss << '-';
        }
        oss << std::setw(2) << static_cast<int>(data_[i]);
    }

    return oss.str();
}

ConstSpan<u8> UUID::Bytes() const noexcept {
    return ConstSpan<u8>{data_.data(), data_.size()};
}

UUIDVersion UUID::Version() const noexcept {
    u8 version_byte = data_[6] >> 4;
    switch (version_byte) {
    case 0:
        return UUIDVersion::Nil;
    case 4:
        return UUIDVersion::Random;
    case 7:
        return UUIDVersion::UnixTime;
    default:
        return UUIDVersion::Nil;
    }
}

UUIDVariant UUID::Variant() const noexcept {
    u8 variant_byte = data_[8] >> 6;
    switch (variant_byte) {
    case 0:
        return UUIDVariant::NCS;
    case 1:
        return UUIDVariant::RFC4122;
    case 2:
        return UUIDVariant::RFC4122;
    case 3:
        return UUIDVariant::Microsoft;
    default:
        return UUIDVariant::Future;
    }
}

bool UUID::operator==(const UUID &other) const noexcept {
    return data_ == other.data_;
}

UUID::UUID(const std::array<u8, 16> &data) : data_(data) {}

}  // namespace launcher
