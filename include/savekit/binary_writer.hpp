#pragma once

#include <cstdint>
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

    // The bytes written so far.
    const std::vector<std::uint8_t>& bytes() const noexcept { 
        return buffer_; 
    }

private:
    std::vector<std::uint8_t> buffer_;
};

} // namespace savekit
