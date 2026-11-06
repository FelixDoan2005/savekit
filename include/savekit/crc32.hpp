#pragma once

#include <cstdint>
#include <span>

namespace savekit {

// CRC-32 checksum (the same one used by zip, PNG and Ethernet).
// Change any bit of the input and the result changes, so it detects corruption.
//
// Known answers:   crc32("")           == 0x00000000
//                  crc32("123456789")  == 0xCBF43926
std::uint32_t crc32(std::span<const std::uint8_t> data) noexcept;

}
