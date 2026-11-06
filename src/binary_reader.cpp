#include "savekit/binary_reader.hpp"

#include <stdexcept>

namespace savekit {

void BinaryReader::require(std::size_t count) const {
    if (remaining() < count) {
        throw std::out_of_range("BinaryReader: not enough bytes remaining");
    }
}

std::uint8_t BinaryReader::read_u8() {
    require(1);
    std::uint8_t byte = data_[pos_];
    pos_++;
    return byte;
}

std::uint32_t BinaryReader::read_u32() {
    require(4);
    std::uint32_t result = 0;
    for (int i=0; i<4; i++) {
        std::uint8_t byte = read_u8();
        result += byte << (8 * i);
    }
    return result;
}

} // namespace savekit
