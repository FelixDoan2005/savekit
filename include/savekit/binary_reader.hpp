#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace savekit {

// Turns bytes back into values. The mirror image of BinaryWriter:
// numbers are read little-endian, in the same order they were written.
//
// The reader does NOT own the bytes. It only looks at them, so the
// vector you pass in must stay alive while you read.
//
// Reading past the end of the data throws std::out_of_range. The bytes
// may come from a corrupted or hostile file, so never trust them.
class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> data) noexcept
        : data_(data) {}

    // Read one byte and move forward by 1.
    std::uint8_t read_u8();

    // Read 4 bytes (little-endian) and move forward by 4.
    // E8 03 00 00 -> 1000
    std::uint32_t read_u32();

    // How many bytes are left to read.
    std::size_t remaining() const noexcept { return data_.size() - pos_; }

private:
    // Throws std::out_of_range unless at least `count` bytes remain.
    void require(std::size_t count) const;

    std::span<const std::uint8_t> data_;
    std::size_t pos_ = 0;  // index of the next byte to read
};

} // namespace savekit
