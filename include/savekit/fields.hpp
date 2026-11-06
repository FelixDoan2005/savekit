#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"
#include "savekit/file_format.hpp"   // for FormatError
#include "savekit/serialize.hpp"

namespace savekit {

// every value in the payload is a field: id (2 bytes) | type (1 byte) | length (4 bytes) | data
// the length lets old code skip fields it doesnt know
inline constexpr std::size_t kFieldHeaderSize = 7;   // 2 + 1 + 4

// what kind of data a field holds
enum class FieldType : std::uint8_t {
    U32 = 1,        // a number (4 bytes)
    String = 2,     // text (length + letters)
    U16Vector = 3,  // list of numbers (count + u16s)
    F32 = 4,        // a float (4 bytes)
};

// one field read back out of a payload
struct Field {
    std::uint16_t id = 0;
    FieldType type = FieldType::U32;
    std::vector<std::uint8_t> data;
};

// which FieldType a C++ type is stored as
//     std::uint32_t -> U32      float -> F32      std::string -> String
//     std::vector<std::uint16_t> -> U16Vector
// any other type is a compile error (static_assert)
template <typename T>
constexpr FieldType field_type_of() {
    // TODO: an if constexpr chain on T, returning the FieldType for each supported type.
    //       Last branch: static_assert(always_false_v<T>, "savekit: no FieldType for this type");
    return FieldType::U32;   // TODO: delete once every branch returns
}

// builds a payload out of fields
class FieldWriter {
public:
    void write_u32(std::uint16_t id, std::uint32_t value);
    void write_f32(std::uint16_t id, float value);
    void write_string(std::uint16_t id, std::string_view text);
    void write_u16_vector(std::uint16_t id, const std::vector<std::uint16_t>& values);

    // one function for every type, replaces the four above
    // pass the exact type: std::uint32_t{7}, 1.5f, std::string("hi") (not a string literal "hi")
    template <typename T>
    void write([[maybe_unused]] std::uint16_t id, [[maybe_unused]] const T& value) {
        // TODO: BinaryWriter temp;
        //       write_value(temp, value);
        //       write_field(id, field_type_of<T>(), temp.bytes());
    }

    // everything written so far. pass this to wrap_payload to make a file
    const std::vector<std::uint8_t>& bytes() const noexcept { return w_.bytes(); }

private:
    // writes id, type, length, then the data
    void write_field(std::uint16_t id, FieldType type, const std::vector<std::uint8_t>& data);

    BinaryWriter w_;
};

// goes through a payload one field at a time
class FieldReader {
public:
    explicit FieldReader(std::span<const std::uint8_t> payload) noexcept : r_(payload) {}

    // reads the next field into `out`. true = got one, false = no more fields
    // throws FormatError if the payload is damaged
    bool next(Field& out);

private:
    BinaryReader r_;
};

// turn a fields data back into a value. throws FormatError if wrong type, too short or extra bytes
std::uint32_t as_u32(const Field& field);
float as_f32(const Field& field);
std::string as_string(const Field& field);
std::vector<std::uint16_t> as_u16_vector(const Field& field);

// one function for every type, replaces the four above
//     std::uint32_t n = as<std::uint32_t>(field);
template <typename T>
T as([[maybe_unused]] const Field& field) {
    // TODO: 1. if field.type != field_type_of<T>() -> throw FormatError
    //       2. try { BinaryReader br(field.data); T value = read_value<T>(br);
    //                if (br.remaining() != 0) throw FormatError(...); return value; }
    //          catch (const std::out_of_range&) { throw FormatError(...); }
    return T{};
}

}
