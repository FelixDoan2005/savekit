#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "savekit/fields.hpp"

namespace savekit {

// the text-making part of the dump tool. kept in the library (not in main.cpp) so the tests can check it

// the name of a field type: "U32", "String", "U16Vector", "F32". an unknown type byte gives "Unknown(99)"
std::string field_type_name(FieldType type);

// bytes as hex, uppercase, one space between: {0x01, 0xAB} -> "01 AB". no bytes -> ""
std::string hex_bytes(std::span<const std::uint8_t> data);

// a string in quotes, with escapes: Al -> "Al"   a"b -> "a\"b"   backslash -> \\   non printable byte -> \x0A
std::string quote_string(const std::string& text);

// one line describing one field. never throws. examples:
//   id 1  U32  7
//   id 2  String  "Al"
//   id 3  U16Vector  [1, 2]            (empty list: [])
//   id 5  F32  2.5
//   id 9  Unknown(99)  2 bytes: AA BB  (no bytes: "0 bytes")
//   id 1  U32  <damaged bytes: 01 00>  (data doesnt match the type: show the raw bytes)
std::string format_field(const Field& field);

// a whole file as text. throws FormatError if the file itself is bad (magic, version, size, crc)
//   savekit file, version 1
//   payload: 26 bytes, crc32 0x55BC801D
//   fields:
//     id 1  String  "Al"
//     id 3  U16Vector  [1]
// no fields at all: "fields: none". if the field structure is damaged: print the fields before it,
// then "  <damaged field: MESSAGE>" and stop (dont throw). every line ends with \n
std::string dump_file(std::span<const std::uint8_t> file);

}
