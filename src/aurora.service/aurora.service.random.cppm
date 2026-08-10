/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

export module aurora.service:random;
import launcher.base;

namespace launcher {
export class Random {
  public:
    static auto Bytes(usize size) -> Result<launcher::Bytes>;

    static auto UInt32() -> Result<u32>;

    static auto UInt64() -> Result<u64>;

    static auto Fill(ByteSpan buffer) -> Result<void>;
};
}  // namespace launcher
