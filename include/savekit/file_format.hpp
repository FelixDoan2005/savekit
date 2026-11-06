#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <vector>

namespace savekit {

// Every savekit file starts with a 15-byte header, then the payload:
//
//   offset  size  field
//   0       5     magic          'G' 'R' 'A' 'C' 'E'
//   5       2     version        u16, little-endian (the container format version)
//   7       4     payload_size   u32, number of payload bytes that follow
//   11      4     crc32          u32, CRC-32 of the payload
//   15      n     payload
//
inline constexpr std::array<std::uint8_t, 5> kMagic{'G', 'R', 'A', 'C', 'E'};
inline constexpr std::uint16_t kFormatVersion = 1;
inline constexpr std::size_t kHeaderSize = 15;

// Thrown when a file is not a valid savekit file: wrong magic, unsupported version,
// wrong size, or a checksum that doesn't match.
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct ParsedFile {
    std::uint16_t version = 0;
    std::vector<std::uint8_t> payload;
};

// Put a header in front of `payload`. Returns the whole file as bytes.
// `version` defaults to the current format version (tests pass others on purpose).
std::vector<std::uint8_t> wrap_payload(std::span<const std::uint8_t> payload,
                                       std::uint16_t version = kFormatVersion);

// Check a whole file and return its payload. Throws FormatError if ANYTHING is wrong:
//   - fewer than kHeaderSize bytes
//   - magic isn't "GRACE"
//   - version is 0 or newer than kFormatVersion
//   - payload_size doesn't match the number of bytes actually left (truncated OR extra bytes)
//   - the CRC-32 of the payload doesn't match the one in the header
// Never return a payload that failed a check.
ParsedFile parse_file(std::span<const std::uint8_t> file);

// Write bytes to disk. Throws std::runtime_error if the file can't be opened or written.
void write_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);

// Read a whole file into memory. Throws std::runtime_error if it can't be opened.
std::vector<std::uint8_t> read_file(const std::filesystem::path& path);

}
