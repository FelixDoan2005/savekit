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

// gives the FieldType label for a C++ type, e.g. float -> F32
// used by write<T> to label the field and by as<T> to check the label
template <typename T>
constexpr FieldType field_type_of() {
    if constexpr (std::is_same_v<T, std::uint32_t>) {
        return FieldType::U32;
    } else if constexpr (std::is_same_v<T, float>) {
        return FieldType::F32;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return FieldType::String;
    } else if constexpr (std::is_same_v<T, std::vector<std::uint16_t>>) {
        return FieldType::U16Vector;
    } else {
        // any other type: compile error. add a branch above to support it
        static_assert(always_false_v<T>, "savekit: no FieldType for this type");
    }
}

// builds a payload out of fields
class FieldWriter {
public:
    void write_u32(std::uint16_t id, std::uint32_t value);
    void write_f32(std::uint16_t id, float value);
    void write_string(std::uint16_t id, std::string_view text);
    void write_u16_vector(std::uint16_t id, const std::vector<std::uint16_t>& values);

    // one function for any type, replaces the four above
    // pass the exact type: std::uint32_t{7}, 1.5f, std::string("hi") (not a string literal "hi")
    template <typename T>
    void write(std::uint16_t id, const T& value) {
        BinaryWriter temp;                                   //turn the value into bytes (write_value picks the right writer)
        write_value(temp, value);
        write_field(id, field_type_of<T>(), temp.bytes());   //write_field adds the id, type and length around them
    }

    //everything written so far. pass this to wrap_payload to make a file
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

// one function for any type, replaces the four above. turns a fields data back into a T
//     std::uint32_t n = as<std::uint32_t>(field);
template <typename T>
T as(const Field& field) {
    if (field.type != field_type_of<T>()) {   // is the field the kind we asked for?
        throw FormatError("field is not the requested type");
    }

    try {
        BinaryReader br(field.data);          // reader over the fields data
        T value = read_value<T>(br);          // decode it into a T
        if (br.remaining() != 0) {            // bytes left over = something is wrong
            throw FormatError("field has extra bytes");
        }
        return value;
    } catch (const std::out_of_range&) {      // reader ran out of bytes, so the data was too short
        throw FormatError("field is too short");
    }
}

}
