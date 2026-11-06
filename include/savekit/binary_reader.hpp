#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace savekit {

// turns bytes back into values. the opposite of BinaryWriter
// it only looks at the bytes (doesnt own them) so they must stay alive while reading
// reading past the end throws std::out_of_range
class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> data) noexcept
        : data_(data) {}

    std::uint8_t read_u8();        // 1 byte
    std::uint16_t read_u16();      // 2 bytes
    std::uint32_t read_u32();      // 4 bytes
    std::uint64_t read_u64();      // 8 bytes

    float read_f32();              // 4 bytes back into a float
    double read_f64();             // 8 bytes back into a double

    std::string read_string();                   // length then the letters
    std::vector<std::uint16_t> read_u16_vector(); // count then the numbers

    // how many bytes are left to read
    std::size_t remaining() const {
        std::size_t n = data_.size();
        return n - pos_;
    }

    // where we are now, and a way to go back. used when a read fails halfway
    std::size_t position() const noexcept { return pos_; }
    void set_position(std::size_t pos) {
        if (pos > data_.size()) {
            throw std::out_of_range("BinaryReader: position past the end");
        }
        pos_ = pos;
    }

private:
    // throws if fewer than `count` bytes are left
    void require(std::size_t count) const;

    std::span<const std::uint8_t> data_;   // the bytes we read from
    std::size_t pos_ = 0;                  // index of the next byte to read
};

}
