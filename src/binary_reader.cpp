#include "savekit/binary_reader.hpp"

#include <stdexcept>
#include <bit>

namespace savekit {

// throws if there arent `count` bytes left to read. every read checks this first
void BinaryReader::require(std::size_t count) const {
    if (remaining() < count) {
        throw std::out_of_range("BinaryReader: not enough bytes remaining");
    }
}

// reads one byte and moves the position forward by 1. every other read ends up calling this
std::uint8_t BinaryReader::read_u8() {
    require(1);
    std::uint8_t byte = data_[pos_];
    pos_++;
    return byte;
}

// reads 4 bytes (lowest first) and rebuilds the number
std::uint32_t BinaryReader::read_u32() {
    require(4);
    std::uint32_t result = 0;
    for (int i=0; i<4; i++) {
        std::uint32_t byte = read_u8();   // 32 bits so the shift doesnt lose anything
        result += byte << (8 * i);        // move the byte back to its place
    }
    return result;
}

// reads the length first, then that many letters
std::string BinaryReader::read_string() {

    std::size_t prev_pos_ = pos_;   // remember where we started so we can go back if it fails
    std::uint32_t length = read_u32();

    try {
        require(length);   // is the length a lie? check before reading anything
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

// same as read_u32 but 2 bytes. calculate in 32 bits, cast down at the end
std::uint16_t BinaryReader::read_u16() {
    require(2);
    std::uint32_t result = 0;
    for (int i=0; i<2; i++) {
        std::uint32_t byte = read_u8();
        result += byte << (8 * i);
    }
    return static_cast<std::uint16_t>(result);
}

// same as read_u32 but 8 bytes
std::uint64_t BinaryReader::read_u64() {
    require(8);
    std::uint64_t result = 0;
    for (int i=0; i<8; i++) {
        std::uint64_t byte = read_u8();
        result += byte << (8 * i);
    }
    return result;
}

// read the 32 bits, then bit_cast them back into a float (same bits, new type)
float BinaryReader::read_f32() {
    std::uint32_t bits = read_u32();
    return std::bit_cast<float>(bits);
}

// same as read_f32 but 64 bits
double BinaryReader::read_f64() {
    std::uint64_t bits = read_u64();
    return std::bit_cast<double>(bits);
}

// reads the count first, then that many u16s
std::vector<std::uint16_t> BinaryReader::read_u16_vector() {
    std::size_t prev_pos_ = pos_;
    std::uint32_t length = read_u32();
    if (length > remaining() / 2) {   // we knoiw each number is 2 bytes. if length>bytes/2 then it wont fit
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
