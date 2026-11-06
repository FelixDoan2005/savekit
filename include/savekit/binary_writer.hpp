#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace savekit {

// Turns values into bytes. Every multi-byte number is written little-endian
// (least significant byte first), so files are identical on every machine.
class BinaryWriter {
public:
    void write_u8(std::uint8_t value);

    // Append 2 bytes, little-endian. 0x1234 -> 34 12
    void write_u16(std::uint16_t value);

    void write_u32(std::uint32_t value);

    // Append 8 bytes, little-endian. 0x0123456789ABCDEF -> EF CD AB 89 67 45 23 01
    void write_u64(std::uint64_t value);

    // Floats are written as their raw IEEE-754 bits, so the file layout is
    // the same everywhere. 1.0f has the bits 0x3F800000 -> 00 00 80 3F
    void write_f32(float value);
    // 1.0 has the bits 0x3FF0000000000000 -> 00 00 00 00 00 00 F0 3F
    void write_f64(double value);

    void write_string(std::string_view text);

    // A u32 count, then each element as a u16 (little-endian).
    // {1, 2, 3} -> 03 00 00 00  01 00  02 00  03 00
    // (Chess moves fit in a u16. Milestone 4 makes this work for any T.)
    // Throws std::length_error if there are more elements than a u32 can count.
    void write_u16_vector(const std::vector<std::uint16_t>& values);

    // The bytes written so far.
    const std::vector<std::uint8_t>& bytes() const noexcept { 
        return buffer_; 
    }

private:
    std::vector<std::uint8_t> buffer_;
};

}
