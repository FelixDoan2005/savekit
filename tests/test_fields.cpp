#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "savekit/fields.hpp"

using Bytes = std::vector<std::uint8_t>;
using savekit::Field;
using savekit::FieldReader;
using savekit::FieldType;
using savekit::FieldWriter;
using savekit::FormatError;

// ---- FieldWriter: the exact bytes ----

TEST(FieldWriter, U32Field) {
    FieldWriter w;
    w.write_u32(1, 7);
    //                id        type  length            data
    EXPECT_EQ(w.bytes(), (Bytes{0x01, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00}));
}

TEST(FieldWriter, F32Field) {
    FieldWriter w;
    w.write_f32(5, 1.0f);
    // 1.0f is the bits 0x3F800000, little-endian: 00 00 80 3F. Type 4 = F32.
    EXPECT_EQ(w.bytes(), (Bytes{0x05, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F}));
}

TEST(FieldDecode, F32RoundTrip) {
    FieldWriter w;
    w.write_f32(5, -2.5f);
    FieldReader r(w.bytes());
    Field f;
    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.type, FieldType::F32);
    EXPECT_EQ(savekit::as_f32(f), -2.5f);
}

TEST(FieldDecode, F32WrongTypeThrows) {
    const Field f{5, FieldType::U32, Bytes{0x00, 0x00, 0x80, 0x3F}};
    EXPECT_THROW(savekit::as_f32(f), FormatError);
}

TEST(FieldDecode, F32WithTooFewBytesThrows) {
    const Field f{5, FieldType::F32, Bytes{0x00, 0x00}};
    EXPECT_THROW(savekit::as_f32(f), FormatError);
}

TEST(FieldWriter, StringField) {
    FieldWriter w;
    w.write_string(2, "hi");
    // data is the string's own u32 length (2) plus the letters = 6 bytes
    EXPECT_EQ(w.bytes(), (Bytes{0x02, 0x00, 0x02, 0x06, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00, 'h', 'i'}));
}

TEST(FieldWriter, U16VectorField) {
    FieldWriter w;
    w.write_u16_vector(3, {1, 2});
    // data is the vector's u32 count (2) plus two u16s = 8 bytes
    EXPECT_EQ(w.bytes(), (Bytes{0x03, 0x00, 0x03, 0x08, 0x00, 0x00, 0x00,
                                0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00}));
}

TEST(FieldWriter, FieldsAppendInOrder) {
    FieldWriter w;
    w.write_u32(1, 7);
    w.write_u32(2, 9);
    EXPECT_EQ(w.bytes().size(), 22u);
    EXPECT_EQ(w.bytes()[0], 0x01);    // first field's id
    EXPECT_EQ(w.bytes()[11], 0x02);   // second field's id
}

// ---- FieldReader: good payloads ----

TEST(FieldReader, EmptyPayloadHasNoFields) {
    const Bytes empty;
    FieldReader r(empty);
    Field f;
    EXPECT_FALSE(r.next(f));
}

TEST(FieldReader, ReadsFieldsBackInOrder) {
    FieldWriter w;
    w.write_u32(1, 42);
    w.write_string(2, "Felix");
    w.write_u16_vector(3, {10, 20, 30});

    FieldReader r(w.bytes());
    Field f;

    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.id, 1);
    EXPECT_EQ(f.type, FieldType::U32);
    EXPECT_EQ(savekit::as_u32(f), 42u);

    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.id, 2);
    EXPECT_EQ(f.type, FieldType::String);
    EXPECT_EQ(savekit::as_string(f), "Felix");

    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.id, 3);
    EXPECT_EQ(f.type, FieldType::U16Vector);
    EXPECT_EQ(savekit::as_u16_vector(f), (std::vector<std::uint16_t>{10, 20, 30}));

    EXPECT_FALSE(r.next(f));
}

TEST(FieldReader, UnknownTypeByteIsKeptNotRejected) {
    // id 9, type 0x63 (nobody knows it), 2 data bytes. Old code must be able to walk past this.
    const Bytes data{0x09, 0x00, 0x63, 0x02, 0x00, 0x00, 0x00, 0xAA, 0xBB};
    FieldReader r(data);
    Field f;
    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.id, 9);
    EXPECT_EQ(f.type, static_cast<FieldType>(0x63));
    EXPECT_EQ(f.data, (Bytes{0xAA, 0xBB}));
    EXPECT_FALSE(r.next(f));
}

TEST(FieldReader, ZeroLengthFieldIsFine) {
    const Bytes data{0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    FieldReader r(data);
    Field f;
    ASSERT_TRUE(r.next(f));
    EXPECT_EQ(f.id, 5);
    EXPECT_TRUE(f.data.empty());
    EXPECT_FALSE(r.next(f));
}

// ---- FieldReader: damaged payloads ----

TEST(FieldReader, TruncatedFieldHeaderThrows) {
    const Bytes data{0x01, 0x00, 0x01};   // header needs 7 bytes
    FieldReader r(data);
    Field f;
    EXPECT_THROW(r.next(f), FormatError);
}

TEST(FieldReader, TruncatedDataThrows) {
    const Bytes data{0x01, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x07, 0x00};  // says 4, has 2
    FieldReader r(data);
    Field f;
    EXPECT_THROW(r.next(f), FormatError);
}

TEST(FieldReader, LyingLengthThrowsInsteadOfAllocating) {
    const Bytes data{0x01, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};  // claims 4 GB
    FieldReader r(data);
    Field f;
    EXPECT_THROW(r.next(f), FormatError);
}

// ---- as_u32 / as_string / as_u16_vector ----

TEST(FieldDecode, WrongTypeThrows) {
    const Field f{1, FieldType::String, Bytes{0x01, 0x00, 0x00, 0x00}};
    EXPECT_THROW(savekit::as_u32(f), FormatError);
}

TEST(FieldDecode, U32WithTooFewBytesThrows) {
    const Field f{1, FieldType::U32, Bytes{0x01, 0x00}};
    EXPECT_THROW(savekit::as_u32(f), FormatError);
}

TEST(FieldDecode, U32WithLeftoverBytesThrows) {
    const Field f{1, FieldType::U32, Bytes{0x01, 0x00, 0x00, 0x00, 0xFF}};
    EXPECT_THROW(savekit::as_u32(f), FormatError);
}

TEST(FieldDecode, StringWithTruncatedTextThrows) {
    const Field f{2, FieldType::String, Bytes{0x05, 0x00, 0x00, 0x00, 'a'}};  // says 5, has 1
    EXPECT_THROW(savekit::as_string(f), FormatError);
}

TEST(FieldDecode, StringWithLeftoverBytesThrows) {
    const Field f{2, FieldType::String, Bytes{0x01, 0x00, 0x00, 0x00, 'a', 0xFF}};
    EXPECT_THROW(savekit::as_string(f), FormatError);
}

TEST(FieldDecode, VectorWithTruncatedElementsThrows) {
    const Field f{3, FieldType::U16Vector, Bytes{0x03, 0x00, 0x00, 0x00, 0x01, 0x00}};  // says 3, has 1
    EXPECT_THROW(savekit::as_u16_vector(f), FormatError);
}

TEST(FieldDecode, VectorWithHugeCountThrows) {
    const Field f{3, FieldType::U16Vector, Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00}};
    EXPECT_THROW(savekit::as_u16_vector(f), FormatError);
}
