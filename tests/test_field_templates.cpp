#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "savekit/fields.hpp"

using Bytes = std::vector<std::uint8_t>;
using savekit::Field;
using savekit::FieldReader;
using savekit::FieldType;
using savekit::FieldWriter;
using savekit::FormatError;

namespace {

// Writes one field with the template, reads it back as a Field.
template <typename T>
Field field_from(const T& value) {
    FieldWriter w;
    w.write(7, value);
    FieldReader r(w.bytes());
    Field f;
    EXPECT_TRUE(r.next(f));
    return f;
}

}  // namespace

// ---- field_type_of ----

TEST(FieldTypeOf, MapsEachSupportedType) {
    EXPECT_EQ(savekit::field_type_of<std::uint32_t>(), FieldType::U32);
    EXPECT_EQ(savekit::field_type_of<float>(), FieldType::F32);
    EXPECT_EQ(savekit::field_type_of<std::string>(), FieldType::String);
    EXPECT_EQ(savekit::field_type_of<std::vector<std::uint16_t>>(), FieldType::U16Vector);
}

// ---- FieldWriter::write must give the SAME bytes as the four old functions ----

TEST(FieldWriterTemplate, U32SameAsOldFunction) {
    FieldWriter a, b;
    a.write(1, std::uint32_t{7});
    b.write_u32(1, 7);
    EXPECT_EQ(a.bytes(), b.bytes());
}

TEST(FieldWriterTemplate, F32SameAsOldFunction) {
    FieldWriter a, b;
    a.write(5, 1.0f);
    b.write_f32(5, 1.0f);
    EXPECT_EQ(a.bytes(), b.bytes());
}

TEST(FieldWriterTemplate, StringSameAsOldFunction) {
    FieldWriter a, b;
    a.write(2, std::string("hi"));
    b.write_string(2, "hi");
    EXPECT_EQ(a.bytes(), b.bytes());
}

TEST(FieldWriterTemplate, U16VectorSameAsOldFunction) {
    FieldWriter a, b;
    a.write(3, std::vector<std::uint16_t>{1, 2});
    b.write_u16_vector(3, {1, 2});
    EXPECT_EQ(a.bytes(), b.bytes());
}

TEST(FieldWriterTemplate, ExactBytesForAU32) {
    FieldWriter w;
    w.write(1, std::uint32_t{7});
    EXPECT_EQ(w.bytes(), (Bytes{0x01, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00}));
}

// ---- as<T> ----

TEST(AsTemplate, U32) {
    EXPECT_EQ(savekit::as<std::uint32_t>(field_from(std::uint32_t{42})), 42u);
}

TEST(AsTemplate, F32) {
    EXPECT_EQ(savekit::as<float>(field_from(-2.5f)), -2.5f);
}

TEST(AsTemplate, String) {
    EXPECT_EQ(savekit::as<std::string>(field_from(std::string("Felix"))), "Felix");
}

TEST(AsTemplate, U16Vector) {
    const std::vector<std::uint16_t> v{10, 20, 30};
    EXPECT_EQ(savekit::as<std::vector<std::uint16_t>>(field_from(v)), v);
}

TEST(AsTemplate, AgreesWithTheOldFunctions) {
    const Field f = field_from(std::string("same"));
    EXPECT_EQ(savekit::as<std::string>(f), savekit::as_string(f));
}

TEST(AsTemplate, WrongTypeThrows) {
    const Field f = field_from(std::string("text"));
    EXPECT_THROW(savekit::as<std::uint32_t>(f), FormatError);
}

TEST(AsTemplate, ShortDataThrows) {
    const Field f{1, FieldType::U32, Bytes{0x01, 0x00}};
    EXPECT_THROW(savekit::as<std::uint32_t>(f), FormatError);
}

TEST(AsTemplate, LeftoverBytesThrow) {
    const Field f{1, FieldType::U32, Bytes{0x01, 0x00, 0x00, 0x00, 0xFF}};
    EXPECT_THROW(savekit::as<std::uint32_t>(f), FormatError);
}

TEST(AsTemplate, TruncatedStringThrows) {
    const Field f{2, FieldType::String, Bytes{0x05, 0x00, 0x00, 0x00, 'a'}};
    EXPECT_THROW(savekit::as<std::string>(f), FormatError);
}

TEST(AsTemplate, VectorWithHugeCountThrows) {
    const Field f{3, FieldType::U16Vector, Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00}};
    EXPECT_THROW(savekit::as<std::vector<std::uint16_t>>(f), FormatError);
}
