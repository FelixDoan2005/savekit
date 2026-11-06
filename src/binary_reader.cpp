#include "savekit/binary_reader.hpp"

#include <stdexcept>
#include <bit>

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
        std::uint32_t byte = read_u8();
        result += byte << (8 * i);
    }
    return result;
}

std::string BinaryReader::read_string() {

    std::size_t prev_pos_ = pos_;
    std::uint32_t length = read_u32();

    try {
        require(length);
    } catch (...) {
        pos_ = prev_pos_;
        throw;
    }

    std::string result;
    for (std::size_t i=0; i<length; i++) {
        result.push_back(static_cast<char>(read_u8()));
    }
    return result;
}

std::uint16_t BinaryReader::read_u16() {
    require(2);
    std::uint32_t result = 0;
    for (int i=0; i<2; i++) {
        std::uint32_t byte = read_u8();
        result += byte << (8 * i);
    }
    return static_cast<std::uint16_t>(result);
}

std::uint64_t BinaryReader::read_u64() {
    require(8);
    std::uint64_t result = 0;
    for (int i=0; i<8; i++) {
        std::uint64_t byte = read_u8();
        result += byte << (8 * i);
    }
    return result;
}

float BinaryReader::read_f32() {
    std::uint32_t bits = read_u32();
    return std::bit_cast<float>(bits);
}

double BinaryReader::read_f64() {
    std::uint64_t bits = read_u64();
    return std::bit_cast<double>(bits);
}

std::vector<std::uint16_t> BinaryReader::read_u16_vector() {
    std::size_t prev_pos_ = pos_;
    std::uint32_t length = read_u32();
    if (length > remaining() / 2) {
        pos_= prev_pos_;
        throw std::out_of_range("BinaryReader: vector longer than data");
    }
    std::vector<std::uint16_t> result;
    for (std::uint32_t i = 0; i<length; i++) {
        result.push_back(read_u16());
    }
    return result;
}

}
