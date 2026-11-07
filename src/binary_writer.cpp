#include "savekit/binary_writer.hpp"
#include <limits>
#include <stdexcept>
#include <bit>

namespace savekit {

// adds one byte to the end of the buffer. every other write ends up calling this
void BinaryWriter::write_u8(std::uint8_t value) {
    buffer_.push_back(value);
}

// splits the number into 4 bytes and writes them lowest byte first (little-endian)
void BinaryWriter::write_u32(std::uint32_t value) {
    for (int i=0; i<4; i++) {
        write_u8(static_cast<std::uint8_t>((value>>(8*i)) & 0xFF));   // shift the byte we want to the bottom, keep only that byte
    }
}

// writes the length (4 bytes) first, then each letter as a byte
void BinaryWriter::write_string(std::string_view text) {
    const std::uint32_t max_len = std::numeric_limits<std::uint32_t>::max();
    if (text.size() > max_len) {   // length has to fit in 4 bytes
        throw std::length_error("BinaryWriter: string too long");
    }
    write_u32(static_cast<std::uint32_t>(text.size()));

    for (std::size_t i=0; i<text.size(); i++) {
        write_u8(static_cast<std::uint8_t>(text[i]));
    }
}

// same as write_u32 but 2 bytes
void BinaryWriter::write_u16(std::uint16_t value) {
    for (int i=0; i<2; i++) {
        write_u8(static_cast<std::uint8_t>((value>>(8*i)) & 0xFF));
    }
}

// same as write_u32 but 8 bytes
void BinaryWriter::write_u64(std::uint64_t value) {
    for (int i=0; i<8; i++) {
        write_u8(static_cast<std::uint8_t>((value>>(8*i)) & 0xFF));
    }
}

// bit_cast keeps the exact bits of the float, then we write them as a u32
void BinaryWriter::write_f32(float value) {
    write_u32(std::bit_cast<std::uint32_t>(value));
}

// same as write_f32 but 64 bits
void BinaryWriter::write_f64(double value) {
    write_u64(std::bit_cast<std::uint64_t>(value));
}

// writes the count first, then each number as a u16
void BinaryWriter::write_u16_vector(const std::vector<std::uint16_t>& values) {
    const std::uint32_t max_len = std::numeric_limits<std::uint32_t>::max();
    if (values.size() > max_len) {   // count has to fit in 4 bytes
        throw std::length_error("BinaryWriter: vector too long");
    }
    write_u32(static_cast<std::uint32_t>(values.size()));

    for (std::size_t i=0; i<values.size(); i++) {
        write_u16(values[i]);
    }
}

}
