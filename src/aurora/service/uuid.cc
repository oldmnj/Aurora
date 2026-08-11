module;

#include <array>
#include <chrono>
#include <cstring>
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

}  // namespace launcher
