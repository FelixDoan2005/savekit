#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;
using Moves = std::vector<std::uint16_t>;

// ---- writer ----

TEST(WriteU16Vector, EmptyIsJustACountOfZero) {
    savekit::BinaryWriter w;
    w.write_u16_vector({});
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x00, 0x00}));
}

TEST(WriteU16Vector, CountThenEachElement) {
    savekit::BinaryWriter w;
    w.write_u16_vector({1, 2, 3});
    EXPECT_EQ(w.bytes(), (Bytes{0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00}));
}

TEST(WriteU16Vector, ElementsAreLittleEndian) {
    savekit::BinaryWriter w;
    w.write_u16_vector({0x1234});
    EXPECT_EQ(w.bytes(), (Bytes{0x01, 0x00, 0x00, 0x00, 0x34, 0x12}));
}

// ---- reader ----

TEST(ReadU16Vector, ReadsCountedElements) {
    Bytes data{0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u16_vector(), (Moves{1, 2, 3}));
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(ReadU16Vector, Empty) {
    Bytes data{0x00, 0x00, 0x00, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_TRUE(r.read_u16_vector().empty());
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(ReadU16Vector, StopsAfterItsOwnElements) {
    Bytes data{0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x2A};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u16_vector(), (Moves{7}));
    EXPECT_EQ(r.read_u8(), 0x2A);
}

// ---- hostile / broken input ----

TEST(ReadU16Vector, MissingCountThrows) {
    Bytes data{0x02, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16_vector(), std::out_of_range);
}

TEST(ReadU16Vector, TruncatedElementsThrow) {
    Bytes data{0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00};  // says 3, has 2
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16_vector(), std::out_of_range);
}

TEST(ReadU16Vector, HalfAnElementThrows) {
    Bytes data{0x01, 0x00, 0x00, 0x00, 0x01};  // says 1 (2 bytes), has 1 byte
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16_vector(), std::out_of_range);
}

TEST(ReadU16Vector, HugeCountThrowsInsteadOfAllocating) {
    // Claims 4 billion elements (8 GB), but only 2 bytes follow.
    Bytes data{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16_vector(), std::out_of_range);
}

TEST(ReadU16Vector, FailedReadLeavesReaderWhereItStarted) {
    Bytes data{0x64, 0x00, 0x00, 0x00, 0x01, 0x00};  // says 100, has 1
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16_vector(), std::out_of_range);
    EXPECT_EQ(r.remaining(), 6u);
}

// ---- round trips ----

TEST(VectorRoundTrip, ChessMoves) {
    const Moves moves{0x0C1C, 0x3C2C, 0x0000, 0xFFFF};
    savekit::BinaryWriter w;
    w.write_u16_vector(moves);
    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_u16_vector(), moves);
}

TEST(VectorRoundTrip, MixedWithOtherFields) {
    savekit::BinaryWriter w;
    w.write_string("Felix");
    w.write_u16_vector({10, 20, 30});
    w.write_f32(2.5f);

    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_string(), "Felix");
    EXPECT_EQ(r.read_u16_vector(), (Moves{10, 20, 30}));
    EXPECT_EQ(r.read_f32(), 2.5f);
    EXPECT_EQ(r.remaining(), 0u);
}
