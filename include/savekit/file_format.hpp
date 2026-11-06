#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <vector>

namespace savekit {

// the file = 15 byte header, then the payload
//start-byte size  field
//   0       5     magic          'G' 'R' 'A' 'C' 'E'
//   5       2     version        u16, little-endian (the container format version)
//   7       4     payload_size   u32, number of payload bytes that follow
//   11      4     crc32          u32, CRC-32 of the payload
//   15      n     payload

inline constexpr std::array<std::uint8_t, 5> kMagic{'G', 'R', 'A', 'C', 'E'};   // says its our kind of file
inline constexpr std::uint16_t kFormatVersion = 1;                               // header layout version
inline constexpr std::size_t kHeaderSize = 15;                                   // 5 + 2 + 4 + 4

// thrown when a file is bad: wrong magic, wrong version, wrong size or crc doesnt match
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// what parse_file gives back
struct ParsedFile {
    std::uint16_t version = 0;
    std::vector<std::uint8_t> payload;
};

// puts the header in front of the payload. returns the whole file as bytes
std::vector<std::uint8_t> wrap_payload(std::span<const std::uint8_t> payload,
                                       std::uint16_t version = kFormatVersion);

// checks the header and crc, gives back the payload. throws FormatError if anything is wrong
ParsedFile parse_file(std::span<const std::uint8_t> file);

// saves bytes to a file on disk
void write_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);

// loads a whole file from disk into bytes
std::vector<std::uint8_t> read_file(const std::filesystem::path& path);

}
