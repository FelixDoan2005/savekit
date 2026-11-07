#include "savekit/dump.hpp"

#include <format>

#include "savekit/crc32.hpp"
#include "savekit/file_format.hpp"

namespace savekit {

std::string field_type_name(FieldType type) {
    if (type == FieldType::U32) {
        return "U32";
    } else if (type == FieldType::String) {
        return "String";
    } else if (type == FieldType::U16Vector) {
        return "U16Vector";
    } else if (type == FieldType::F32) {
        return "F32";
    } 
    // none matched therefore an unknown type byte. static_cast<int> prints as a number, not a character
    return "Unknown(" + std::to_string(static_cast<int>(type)) + ")";
}

std::string hex_bytes(std::span<const std::uint8_t> data) {
    if (data.empty()) {
        return "";
    }
    const std::string hex_digits = "0123456789ABCDEF";  
    std::string result;

    std::size_t b = data[0];
    result += hex_digits[b/16];
    result += hex_digits[b%16];

    for (std::size_t i=1; i<data.size(); i++) {
        b = data[i];
        result += ' ';   
        result += hex_digits[b/16];
        result += hex_digits[b%16];
    }
    return result;
}

std::string quote_string(const std::string& text) {
    const std::string hex_digits = "0123456789ABCDEF";
    std::string result = "\"";   // start with a quote

    for (std::size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (c == '"') {
            result += "\\\"";             // a quote becomes backslash + quote 
        } else if (c == '\\') {
            result += "\\\\";             // a backslash becomes two backslashes
        } else if (c >= 32 && c <= 126) {
            result += c;                  // printable character: as it is
        } else {
            std::size_t b = static_cast<std::uint8_t>(c);   // anything else: backslash x + two hex digits
            result += "\\x";
            result += hex_digits[b / 16];
            result += hex_digits[b % 16];
        }
    }
    result += '"';
    return result;
}

std::string format_field(const Field& field) {
    std::string line = "id " + std::to_string(field.id) + "  " + field_type_name(field.type) + "  ";

    try {
        // then the value, decided by the type. as<T> throws FormatError if the data doesnt match
        if (field.type == FieldType::U32) {
            line += std::to_string(as<std::uint32_t>(field));
        } else if (field.type == FieldType::F32) {
            std::string number = std::to_string(as<float>(field));   // 2.5 gives "2.500000"
            for (std::size_t i=number.size()-1; i>0; i--) {
                if (number[i] != '0') {
                    break;
                }
                number.pop_back();
            }
            if (number.back() == '.') {
                number.pop_back();   // a whole number like 2.00 cuts the dot too: "2"
            }
            line += number;
        } else if (field.type == FieldType::String) {
            line += quote_string(as<std::string>(field));
        } else if (field.type == FieldType::U16Vector) {
            std::vector<std::uint16_t> values = as<std::vector<std::uint16_t>>(field);
            line += "[";
            if (!values.empty()) {   // an empty list stays as []
                line += std::to_string(values[0]);   // first number on its own, no comma before it
                for (std::size_t i = 1; i < values.size(); i++) {
                    line += ", ";                    // the rest get a comma in front
                    line += std::to_string(values[i]);
                }
            }
            line += "]";
        } else {
            // a type we dont know and cant decode it, so show how many bytes and the raw bytes
            line += std::to_string(field.data.size()) + " bytes";
            if (!field.data.empty()) {
                line += ": " + hex_bytes(field.data);
            }
        }
    } catch (const FormatError&) {
        // known type but the data is wrong: show the raw bytes instead of crashing
        line += "<damaged bytes: " + hex_bytes(field.data) + ">";
    }

    return line;
}

std::string dump_file(std::span<const std::uint8_t> file) {
    // check the file (magic, version, size, crc). if its bad this throws FormatError and we let it through
    ParsedFile parsed = parse_file(file);

    // first line: the version
    std::string text = "savekit file, version " + std::to_string(parsed.version) + "\n";

    // second line: payload size and crc as 8 hex digits, e.g. 0x55BC801D
    std::uint32_t crc = crc32(parsed.payload); // get fingerprint
    std::string crc_text = std::format("0x{:08X}", crc);   // X = hex in capitals, 08 = 8 digits wide padded with zeros
    text +="payload: " + std::to_string(parsed.payload.size()) + " bytes, crc32 " + crc_text + "\n";

    // one line per field. collect them in `lines` first so we know if there were any
    FieldReader r(parsed.payload);
    Field f;
    std::string lines;

    // next() throws FormatError if the field structure is damaged: show it and stop - dont crash
    try {
        while (r.next(f)) {
            lines += "  " + format_field(f) + "\n";
        }
    } catch (const FormatError& e) {
        lines += std::string("  <damaged field: ") + e.what() + ">\n";
    }

    if (lines.empty()) {
        text += "fields: none\n";
    } else {
        text += "fields:\n" + lines;
    }

    return text;
}

}
