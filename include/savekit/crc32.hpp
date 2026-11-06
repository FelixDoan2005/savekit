#pragma once

#include <cstdint>
#include <span>

namespace savekit {

// gives a fingerprint of some bytes. if any byte changes the fingerprint changes
// used to check a file wasnt damaged. crc32("123456789") should give 0xCBF43926
std::uint32_t crc32(std::span<const std::uint8_t> data) noexcept;

}
