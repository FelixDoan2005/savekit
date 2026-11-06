#include "savekit/fields.hpp"

#include <limits>
#include <stdexcept>

namespace savekit {

// writes one field: id, type, length, then the data. the 3 write functions below all use this
void FieldWriter::write_field(std::uint16_t id, FieldType type,const std::vector<std::uint8_t>& data) {
    const std::uint32_t max_len = std::numeric_limits<std::uint32_t>::max();
    if (data.size() > max_len) {   // length has to fit in 4 bytes
        throw std::length_error("length error: field too large");
    }
    w_.write_u16(id);                                          // id
    w_.write_u8(static_cast<std::uint8_t>(type));              // type
    w_.write_u32(static_cast<std::uint32_t>(data.size()));     // length

    for (std::size_t i=0; i<data.size(); i++) {                // data
        w_.write_u8(data[i]);
    }
}

// the 4 write functions all do the same: turn the value into bytes in a temp writer, then write_field adds the label
void FieldWriter::write_u32(std::uint16_t id, std::uint32_t value) {
    BinaryWriter temp;
    temp.write_u32(value);
    write_field(id, FieldType::U32, temp.bytes());
}

void FieldWriter::write_f32(std::uint16_t id, float value) {
    BinaryWriter temp;
    temp.write_f32(value);
    write_field(id, FieldType::F32, temp.bytes());
}

void FieldWriter::write_string(std::uint16_t id, std::string_view text) {
    BinaryWriter temp;
    temp.write_string(text);
    write_field(id, FieldType::String, temp.bytes());
}

void FieldWriter::write_u16_vector(std::uint16_t id, const std::vector<std::uint16_t>& values) {
    BinaryWriter temp;
    temp.write_u16_vector(values);
    write_field(id, FieldType::U16Vector, temp.bytes());
}

// reads the next field into `out`. true = got one, false = no more fields
bool FieldReader::next(Field& out) {
    if (r_.remaining() == 0) {   // nothing left, normal end
        return false;
    }
    if (r_.remaining() < kFieldHeaderSize) {   // not enough for a header, damaged
        throw FormatError("field header is cut off");
    }
    out.id = r_.read_u16();
    out.type = static_cast<FieldType>(r_.read_u8());
    std::uint32_t length = r_.read_u32();

    if (length > r_.remaining()) {   // length is a lie, check before copying
        throw FormatError("field length is longer than the data left");
    }

    out.data.clear();
    for (std::uint32_t i = 0; i < length; i++) {
        out.data.push_back(r_.read_u8());
    }
    return true;
}

// the 4 as_ functions turn a fields data back into a value. all check: right type, enough bytes, no extra bytes
std::uint32_t as_u32(const Field& field) {
    if (field.type != FieldType::U32) {
        throw FormatError("field is not a u32");
    }

    try {
        BinaryReader br(field.data);
        std::uint32_t value = br.read_u32();
        if (br.remaining() != 0) {   // leftover bytes
            throw FormatError("u32 field has extra bytes");
        }
        return value;
    } catch (const std::out_of_range&) {   // reader ran out of bytes
        throw FormatError("u32 field is too short");
    }
}

float as_f32(const Field& field) {
    if (field.type != FieldType::F32) {
        throw FormatError("field is not an f32");
    }

    try {
        BinaryReader br(field.data);
        float value = br.read_f32();
        if (br.remaining() != 0) {
            throw FormatError("f32 field has extra bytes");
        }
        return value;
    } catch (const std::out_of_range&) {
        throw FormatError("f32 field is too short");
    }
}

std::string as_string(const Field& field) {
    if (field.type != FieldType::String) {
        throw FormatError("field is not a string");
    }

    try {
        BinaryReader br(field.data);
        std::string value = br.read_string();
        if (br.remaining() != 0) {
            throw FormatError("string field has extra bytes");
        }
        return value;
    } catch (const std::out_of_range&) {
        throw FormatError("string field is too short");
    }
}

std::vector<std::uint16_t> as_u16_vector(const Field& field) {
    if (field.type != FieldType::U16Vector) {
        throw FormatError("field is not a u16 vector");
    }

    try {
        BinaryReader br(field.data);
        std::vector<std::uint16_t> value = br.read_u16_vector();
        if (br.remaining() != 0) {
            throw FormatError("vector field has extra bytes");
        }
        return value;
    } catch (const std::out_of_range&) {
        throw FormatError("vector field is too short");
    }
}

}
