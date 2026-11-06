#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace savekit {

// turns values into bytes. numbers are written lowest byte first (little-endian)
class BinaryWriter {
public:
    void write_u8(std::uint8_t value);            // 1 byte
    void write_u16(std::uint16_t value);          // 2 bytes
    void write_u32(std::uint32_t value);          // 4 bytes
    void write_u64(std::uint64_t value);          // 8 bytes

    void write_f32(float value);                  // float: its raw bits as 4 bytes
    void write_f64(double value);                 // double: its raw bits as 8 bytes

    void write_string(std::string_view text);     // length (4 bytes) then the letters

    void write_u16_vector(const std::vector<std::uint16_t>& values);   // count (4 bytes) then each number as 2 bytes

    // all the bytes written so far
    const std::vector<std::uint8_t>& bytes() const noexcept {
        return buffer_;
    }

private:
    std::vector<std::uint8_t> buffer_;   // the bytes. write_u8 adds to this
};

}
