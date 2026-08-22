module;

#define OPENSSL_SUPPRESS_DEPRECA_TED
#define OPENSSL_API_COMPAT 0x30000000L
#include <openssl/sha.h>

module aurora.service;

namespace launcher {
auto Crypto::Sha1(ConstByteSpan data) -> Result<Bytes> {
    Bytes hash(20);
    SHA1(reinterpret_cast<const unsigned char *>(data.data()), data.size(),
         reinterpret_cast<unsigned char *>(hash.data()));
    return Ok(hash);
}

auto Crypto::Sha256(ConstByteSpan data) -> Result<Bytes> {
    Bytes hash(32);
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(),
           reinterpret_cast<unsigned char *>(hash.data()));
    return Ok(hash);
}

auto Crypto::Sha512(ConstByteSpan data) -> Result<Bytes> {
    Bytes hash(64);
    SHA512(reinterpret_cast<const unsigned char *>(data.data()), data.size(),
           reinterpret_cast<unsigned char *>(hash.data()));
    return Ok(hash);
}
}  // namespace launcher
