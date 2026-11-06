#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"
#include "savekit/file_format.hpp"   // for FormatError

namespace savekit {

// Every value in the payload is stored as a tagged field:
//
//   id      u16   which field this is (1 = white, 2 = black, ...)
//   type    u8    what kind of data it holds (see FieldType)
//   length  u32   how many data bytes follow
//   data    ...   exactly `length` bytes
//
// The length is what makes old code able to SKIP fields it has never heard of.
//
inline constexpr std::size_t kFieldHeaderSize = 7;   // 2 + 1 + 4

enum class FieldType : std::uint8_t {
    U32 = 1,        // data: 4 bytes, little-endian
    String = 2,     // data: what BinaryWriter::write_string produces (u32 length + letters)
    U16Vector = 3,  // data: what BinaryWriter::write_u16_vector produces (u32 count + u16s)
    F32 = 4,        // data: 4 bytes, the raw IEEE-754 bits of the float, little-endian
};

// One field, read back out of a payload.
struct Field {
    std::uint16_t id = 0;
    FieldType type = FieldType::U32;
    std::vector<std::uint8_t> data;
};

// Builds a payload out of tagged fields.
class FieldWriter {
public:
    void write_u32(std::uint16_t id, std::uint32_t value);
    void write_f32(std::uint16_t id, float value);
    void write_string(std::uint16_t id, std::string_view text);
    void write_u16_vector(std::uint16_t id, const std::vector<std::uint16_t>& values);

    // Everything written so far. Pass this to wrap_payload() to make a file.
    const std::vector<std::uint8_t>& bytes() const noexcept { return w_.bytes(); }

private:
    // Writes id, type, length, then the data bytes.
    void write_field(std::uint16_t id, FieldType type, const std::vector<std::uint8_t>& data);

    BinaryWriter w_;
};

// Walks through a payload one field at a time.
class FieldReader {
public:
    explicit FieldReader(std::span<const std::uint8_t> payload) noexcept : r_(payload) {}

    // Reads the next field into `out` and returns true.
    // Returns false when there are no more fields (clean end of the payload).
    // Throws FormatError if the payload is damaged:
    //   - not enough bytes left for a field header
    //   - the field claims more data than is left (check BEFORE copying/allocating!)
    // An unknown `type` byte is NOT an error: keep it as it is, the caller can skip the field.
    bool next(Field& out);

private:
    BinaryReader r_;
};

// Turn a field's data back into a value. Each one throws FormatError if
//   - the field has the wrong type, or
//   - the data is the wrong size / too short / has extra bytes left over.
// (BinaryReader throws std::out_of_range on short data: catch it and rethrow as FormatError.)
std::uint32_t as_u32(const Field& field);
float as_f32(const Field& field);
std::string as_string(const Field& field);
std::vector<std::uint16_t> as_u16_vector(const Field& field);

}
