/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

#include <array>

export module launcher.base:uuid;
import :types;
import :error;


namespace launcher {
enum class UUIDVersion { Nil = 0, Random = 4, TimeOrdered = 7 };

// 支持UUID4，RFC 4122
export class UUID {
  private:
    std::array<u8, 16> data_{};

    explicit constexpr UUID(std::array<u8, 16>) noexcept;

  public:
    constexpr UUID() noexcept = default;

    static Result<UUID> Parse(StringView value);

    static UUID Random();

    String ToString() const;

    constexpr auto Bytes() const noexcept -> ConstSpan<u8>;

    constexpr bool isNil() const noexcept;

    constexpr auto operator<=>(const UUID &) const = default;
};
}  // namespace launcher
