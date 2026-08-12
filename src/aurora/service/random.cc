module;

#include <openssl/rand.h>
#include <span>
#include <utility>

module aurora.service;
import launcher.base;

namespace launcher {

auto Random::Fill(ByteSpan buffer) -> Result<void> {
    if (buffer.empty()) {
        return Ok();
    }
    if (RAND_bytes(
                reinterpret_cast<unsigned char *>(buffer.data()),
                static_cast<int>(buffer.size())
        ) != 1) {
        return Err(
                {ErrorCategory::Runtime, ErrorCode::InternalError,
                 "RAND_bytes生成失败"}
        );
    }
    return Ok();
}

auto Random::Bytes(usize size) -> Result<launcher::Bytes> {
    if (size == 0) {
        return Err<launcher::Bytes>(
                {ErrorCategory::Runtime, ErrorCode::InvalidArgument,
                 "size不能为0"}
        );
    }
    launcher::Bytes buffer{size};

    auto result = Fill(std::span<std::byte>{buffer});
    if (result.HasError()) {
        return Err<launcher::Bytes>(std::move(result.Error()));
    }
    return Ok<launcher::Bytes>(std::move(buffer));
}

auto Random::UInt32() -> Result<u32> {
    u32 value;
    if (RAND_bytes(reinterpret_cast<unsigned char *>(&value), sizeof(value)) !=
        1) {
        return Err<u32>(
                {ErrorCategory::Runtime, ErrorCode::InternalError,
                 "RAND_bytes生成u32失败"}
        );
    }
    return Ok<u32>(std::move(value));
}

auto Random::UInt64() -> Result<u64> {
    u64 value;
    if (RAND_bytes(reinterpret_cast<unsigned char *>(&value), sizeof(value)) !=
        1) {
        return Err<u64>(
                {ErrorCategory::Runtime, ErrorCode::InternalError,
                 "RAND_bytes生成u64失败"}
        );
    }
    return Ok<u64>(std::move(value));
}
}  // namespace launcher
