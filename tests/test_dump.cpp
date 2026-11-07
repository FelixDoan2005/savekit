#include <gtest/gtest.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "savekit/crc32.hpp"
#include "savekit/dump.hpp"
#include "savekit/fields.hpp"
#include "savekit/file_format.hpp"
#include "savekit/game_record.hpp"

using Bytes = std::vector<std::uint8_t>;
using savekit::Field;
using savekit::FieldType;
using savekit::FieldWriter;
using savekit::FormatError;

namespace {

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

// "0x55BC801D"
std::string crc_text(const Bytes& payload) {
    std::ostringstream out;
    out << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << savekit::crc32(payload);
    return out.str();
}

}  // namespace

// ---- field_type_name ----

TEST(FieldTypeName, KnownTypes) {
    EXPECT_EQ(savekit::field_type_name(FieldType::U32), "U32");
    EXPECT_EQ(savekit::field_type_name(FieldType::String), "String");
    EXPECT_EQ(savekit::field_type_name(FieldType::U16Vector), "U16Vector");
    EXPECT_EQ(savekit::field_type_name(FieldType::F32), "F32");
}

TEST(FieldTypeName, UnknownTypeShowsItsNumber) {
    EXPECT_EQ(savekit::field_type_name(static_cast<FieldType>(99)), "Unknown(99)");
}

// ---- hex_bytes ----

TEST(HexBytes, NoBytesIsEmpty) {
    EXPECT_EQ(savekit::hex_bytes(Bytes{}), "");
}

TEST(HexBytes, UppercaseWithSpaces) {
    EXPECT_EQ(savekit::hex_bytes(Bytes{0x01, 0xAB, 0xFF}), "01 AB FF");
}

TEST(HexBytes, ZeroByteKeepsItsTwoDigits) {
    EXPECT_EQ(savekit::hex_bytes(Bytes{0x00, 0x0A}), "00 0A");
}

// ---- quote_string ----

TEST(QuoteString, PlainText) {
    EXPECT_EQ(savekit::quote_string("Al"), "\"Al\"");
}

TEST(QuoteString, EmptyString) {
    EXPECT_EQ(savekit::quote_string(""), "\"\"");
}

TEST(QuoteString, EscapesQuoteAndBackslash) {
    EXPECT_EQ(savekit::quote_string("a\"b"), "\"a\\\"b\"");
    EXPECT_EQ(savekit::quote_string("a\\b"), "\"a\\\\b\"");
}

TEST(QuoteString, NonPrintableBecomesHex) {
    EXPECT_EQ(savekit::quote_string("a\nb"), "\"a\\x0Ab\"");
    EXPECT_EQ(savekit::quote_string(std::string("\0", 1)), "\"\\x00\"");
}

// ---- format_field ----

TEST(FormatField, U32) {
    const Field f{1, FieldType::U32, Bytes{0x07, 0x00, 0x00, 0x00}};
    EXPECT_EQ(savekit::format_field(f), "id 1  U32  7");
}

TEST(FormatField, String) {
    const Field f{2, FieldType::String, Bytes{0x02, 0x00, 0x00, 0x00, 'A', 'l'}};
    EXPECT_EQ(savekit::format_field(f), "id 2  String  \"Al\"");
}

TEST(FormatField, VectorOfNumbers) {
    const Field f{3, FieldType::U16Vector, Bytes{0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00}};
    EXPECT_EQ(savekit::format_field(f), "id 3  U16Vector  [1, 2]");
}

TEST(FormatField, EmptyVector) {
    const Field f{3, FieldType::U16Vector, Bytes{0x00, 0x00, 0x00, 0x00}};
    EXPECT_EQ(savekit::format_field(f), "id 3  U16Vector  []");
}

TEST(FormatField, Float) {
    const Field f{5, FieldType::F32, Bytes{0x00, 0x00, 0x20, 0x40}};   // 2.5f
    EXPECT_EQ(savekit::format_field(f), "id 5  F32  2.5");
}

TEST(FormatField, UnknownTypeShowsRawBytes) {
    const Field f{9, static_cast<FieldType>(99), Bytes{0xAA, 0xBB}};
    EXPECT_EQ(savekit::format_field(f), "id 9  Unknown(99)  2 bytes: AA BB");
}

TEST(FormatField, UnknownTypeWithNoBytes) {
    const Field f{9, static_cast<FieldType>(99), Bytes{}};
    EXPECT_EQ(savekit::format_field(f), "id 9  Unknown(99)  0 bytes");
}

TEST(FormatField, DamagedDataIsShownNotThrown) {
    const Field f{1, FieldType::U32, Bytes{0x01, 0x00}};   // a u32 needs 4 bytes
    EXPECT_EQ(savekit::format_field(f), "id 1  U32  <damaged bytes: 01 00>");
}

TEST(FormatField, DamagedStringIsShownNotThrown) {
    const Field f{2, FieldType::String, Bytes{0x05, 0x00, 0x00, 0x00, 'a'}};   // says 5 letters, has 1
    EXPECT_EQ(savekit::format_field(f), "id 2  String  <damaged bytes: 05 00 00 00 61>");
}

// ---- dump_file ----

TEST(DumpFile, ShowsHeaderAndEveryField) {
    FieldWriter w;
    w.write(1, std::string("Al"));
    w.write(3, std::vector<std::uint16_t>{1});
    w.write(5, 2.5f);
    const Bytes payload = w.bytes();
    const std::string text = savekit::dump_file(savekit::wrap_payload(payload));

    EXPECT_TRUE(contains(text, "savekit file, version 1\n"));
    EXPECT_TRUE(contains(text, "payload: " + std::to_string(payload.size()) + " bytes, crc32 " + crc_text(payload) + "\n"));
    EXPECT_TRUE(contains(text, "fields:\n"));
    EXPECT_TRUE(contains(text, "  id 1  String  \"Al\"\n"));
    EXPECT_TRUE(contains(text, "  id 3  U16Vector  [1]\n"));
    EXPECT_TRUE(contains(text, "  id 5  F32  2.5\n"));
}

TEST(DumpFile, FieldsAppearInFileOrder) {
    FieldWriter w;
    w.write(7, std::uint32_t{1});
    w.write(2, std::uint32_t{2});
    const std::string text = savekit::dump_file(savekit::wrap_payload(w.bytes()));
    EXPECT_LT(text.find("id 7"), text.find("id 2"));
}

TEST(DumpFile, EmptyPayloadSaysNoFields) {
    const std::string text = savekit::dump_file(savekit::wrap_payload(Bytes{}));
    EXPECT_TRUE(contains(text, "fields: none\n"));
}

TEST(DumpFile, UnknownFieldIsListedNotSkipped) {
    FieldWriter w;
    w.write(1, std::string("x"));
    Bytes payload = w.bytes();
    const Bytes unknown{0x09, 0x00, 0x63, 0x02, 0x00, 0x00, 0x00, 0xAA, 0xBB};   // id 9, type 99, 2 bytes
    payload.insert(payload.end(), unknown.begin(), unknown.end());

    const std::string text = savekit::dump_file(savekit::wrap_payload(payload));
    EXPECT_TRUE(contains(text, "  id 9  Unknown(99)  2 bytes: AA BB\n"));
}

TEST(DumpFile, WorksOnARealGameFile) {
    savekit::GameRecord g;
    g.white = "Felix";
    g.black = "Stockfish";
    g.moves = {3100, 15404};
    g.move_times = {120, 340};
    const std::string text = savekit::dump_file(savekit::wrap_payload(savekit::save_game(g)));

    EXPECT_TRUE(contains(text, "id 1  String  \"Felix\""));
    EXPECT_TRUE(contains(text, "id 2  String  \"Stockfish\""));
    EXPECT_TRUE(contains(text, "id 3  U16Vector  [3100, 15404]"));
    EXPECT_TRUE(contains(text, "id 4  U16Vector  [120, 340]"));
}

TEST(DumpFile, DamagedFieldStructureIsReportedNotThrown) {
    // the file itself is fine (good header, good crc), but the payload isnt a list of fields
    const Bytes garbage{0x01, 0x00, 0x01};
    const std::string text = savekit::dump_file(savekit::wrap_payload(garbage));
    EXPECT_TRUE(contains(text, "<damaged field:"));
}

TEST(DumpFile, FieldsBeforeTheDamageAreStillShown) {
    FieldWriter w;
    w.write(1, std::string("ok"));
    Bytes payload = w.bytes();
    payload.push_back(0x01);   // a stray byte: not enough for a field header
    const std::string text = savekit::dump_file(savekit::wrap_payload(payload));
    EXPECT_TRUE(contains(text, "id 1  String  \"ok\""));
    EXPECT_TRUE(contains(text, "<damaged field:"));
}

// ---- a bad file is an error ----

TEST(DumpFile, EmptyInputThrows) {
    EXPECT_THROW(savekit::dump_file(Bytes{}), FormatError);
}

TEST(DumpFile, WrongMagicThrows) {
    Bytes file = savekit::wrap_payload(Bytes{1, 2, 3});
    file[0] = 'X';
    EXPECT_THROW(savekit::dump_file(file), FormatError);
}

TEST(DumpFile, CorruptedPayloadThrows) {
    Bytes file = savekit::wrap_payload(Bytes{1, 2, 3});
    file.back() ^= 0xFF;
    EXPECT_THROW(savekit::dump_file(file), FormatError);
}

TEST(DumpFile, TruncatedFileThrows) {
    Bytes file = savekit::wrap_payload(Bytes{1, 2, 3});
    file.pop_back();
    EXPECT_THROW(savekit::dump_file(file), FormatError);
}
