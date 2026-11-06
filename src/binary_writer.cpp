#include "savekit/binary_writer.hpp"
#include <limits>
#include <stdexcept>

namespace savekit {

void BinaryWriter::write_u8(std::uint8_t value) {
    buffer_.push_back(value);
}

void BinaryWriter::write_u32(std::uint32_t value) {
    for (int i=0; i<4; i++) {
        write_u8(static_cast<std::uint8_t>((value>>(8*i)) & 0xFF));
    }
}

void BinaryWriter::write_string(std::string_view text) {
    const std::uint32_t max_len = std::numeric_limits<std::uint32_t>::max();
    if (text.size() > max_len) {
        throw std::length_error("BinaryWriter: string too long");
    }
    write_u32(static_cast<std::uint32_t>(text.size()));
    
    for (std::size_t i=0; i<text.size(); i++) {
        write_u8(static_cast<std::uint8_t>(text[i]));
    }
}

} 
