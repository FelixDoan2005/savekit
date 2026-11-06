#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace savekit {

// Turns values into bytes. Every multi-byte number is written little-endian
// (least significant byte first), so files are identical on every machine.
class BinaryWriter {
public:
    // Append one byte.
    void write_u8(std::uint8_t value);

    // Append 4 bytes, little-endian. 1000 (0x000003E8) -> E8 03 00 00
    void write_u32(std::uint32_t value);

    // Append a string: a u32 length, then that many bytes (no terminator).
    // "Felix" -> 05 00 00 00 46 65 6C 69 78
    // Throws std::length_error if the string is longer than a u32 can count.
    void write_string(std::string_view text);

    // The bytes written so far.
    const std::vector<std::uint8_t>& bytes() const noexcept { 
        return buffer_; 
    }

private:
    std::vector<std::uint8_t> buffer_;
};

} // namespace savekit
