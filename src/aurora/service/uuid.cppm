/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

#include <array>

export module aurora.service:uuid;
import launcher.base;

namespace launcher {
export enum class UUIDVersion : u8 {
    Nil      = 0,
    // TimeBased     = 1,
    // NameBasedMD5  = 3,
    Random   = 4,
    // NameBasedSHA1 = 5,
    // ReorderedTime = 6,
    UnixTime = 7
};

export enum class UUIDVariant : u8 { NCS, RFC4122, Microsoft, Future };


export class UUID {
  private:
    std::array<u8, 16> data_;

  public:
    constexpr UUID() noexcept = default;

    explicit UUID(const std::array<u8, 16> &data);

    static Result<UUID> Parse(StringView);

    String ToString() const;

    ConstSpan<u8> Bytes() const noexcept;

    UUIDVersion Version() const noexcept;

    UUIDVariant Variant() const noexcept;

    bool operator==(const UUID &) const noexcept;
};


export class UUIDGenerator {
  public:
    // static auto V1() -> Result<UUID>;

    // static auto V3(UUID namespace_id, StringView name) -> Result<UUID>;

    static auto V4() -> Result<UUID>;

    // static auto V5(UUID namespace_id, StringView name) -> Result<UUID>;

    // static auto V6() -> Result<UUID>;

    static auto V7() -> Result<UUID>;
};
}  // namespace launcher
