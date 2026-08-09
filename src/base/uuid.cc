module;

#include <array>
#include <random>

module launcher.base;

namespace launcher {
UUID UUID::Random() {
    std::array<u8, 16> bytes;

    std::random_device rd;

    for (auto &b : bytes) {
        b = static_cast<u8>(rd());
    }

    bytes[6] = (bytes[6] & 0x0F) | 0x40;

    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    return UUID{bytes};
}

static constexpr char hex[] = "0123456789abcdef";

String UUID::ToString() const {
    String result;
    result.reserve(36);

    for (usize i = 0; i < 16; ++i) {
        result.push_back(hex[data_[i] >> 4]);

        result.push_back(hex[data_[i] & 0x0F]);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            result.push_back('-');
        }
    }

    return result;
}

Result<UUID> UUID::Parse(StringView value) {}

constexpr bool UUID::isNil() const noexcept {
    for (auto b : data_) {
        if (b != 0) {
            return false;
        }
    }

    return true;
}

}  // namespace launcher
