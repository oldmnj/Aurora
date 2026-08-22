/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oldmnj <oldmnj@163.com>
 *
 * This is the core kernel module of the launcher.
 * For license details, see the LICENSE file in the root directory.
 */
module;

export module aurora.service:crypto;
import launcher.base;

namespace launcher {
export class Crypto {
  public:
    static auto Sha1(ConstByteSpan) -> Result<Bytes>;

    static auto Sha256(ConstByteSpan) -> Result<Bytes>;

    static auto Sha512(ConstByteSpan) -> Result<Bytes>;

    static auto Md5(ConstByteSpan) -> Result<Bytes>;  // 仅兼容旧协议

    static auto Sha1File(Path) -> Result<Bytes>;  // 流式

    static auto Sha256File(Path) -> Result<Bytes>;

    static auto HexEncode(ConstByteSpan) -> Result<String>;

    static auto Base64Encode(ConstByteSpan) -> Result<String>;

    static auto Base64Decode(StringView) -> Result<Bytes>;

    static auto HmacSha256(ConstByteSpan key, ConstByteSpan data)
            -> Result<Bytes>;

    static auto AesGcmEncrypt(
            ConstByteSpan key, ConstByteSpan iv, ConstByteSpan plain,
            ConstByteSpan aad
    ) -> Result<Bytes>;

    static auto AesGcmDecrypt(
            ConstByteSpan key, ConstByteSpan iv, ConstByteSpan cipher,
            ConstByteSpan aad
    ) -> Result<Bytes>;
};
}  // namespace launcher
