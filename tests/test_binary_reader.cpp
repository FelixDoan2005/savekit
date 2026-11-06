#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;

TEST(BinaryReader, ReadU8ReturnsTheByte) {
    Bytes data{0xAB};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u8(), 0xAB);
}

TEST(BinaryReader, ReadU32IsLittleEndian) {
    Bytes data{0xE8, 0x03, 0x00, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u32(), 1000u);
}

TEST(BinaryReader, ReadU32UsesAllFourBytes) {
    Bytes data{0x78, 0x56, 0x34, 0x12};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u32(), 0x12345678u);
}

TEST(BinaryReader, ReadsAdvanceInOrder) {
    Bytes data{0x01, 0x02, 0x00, 0x00, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u8(), 0x01);
    EXPECT_EQ(r.read_u32(), 2u);
}

TEST(BinaryReader, RemainingShrinksAsYouRead) {
    Bytes data{0x01, 0x02, 0x03, 0x04, 0x05};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.remaining(), 5u);
    r.read_u8();
    EXPECT_EQ(r.remaining(), 4u);
    r.read_u32();
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(BinaryReader, ReadU8PastEndThrows) {
    Bytes data;
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u8(), std::out_of_range);
}

TEST(BinaryReader, ReadU32WithTooFewBytesThrows) {
    Bytes data{0x01, 0x02, 0x03};  // only 3 bytes, needs 4
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u32(), std::out_of_range);
}

TEST(BinaryReader, FailedReadDoesNotConsumeBytes) {
    Bytes data{0x01, 0x02, 0x03};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u32(), std::out_of_range);
    EXPECT_EQ(r.remaining(), 3u);  // nothing was eaten by the failed read
}

TEST(RoundTrip, WriterThenReaderGivesBackTheSameValues) {
    savekit::BinaryWriter w;
    w.write_u8(7);
    w.write_u32(0xDEADBEEF);
    w.write_u32(0);

    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_u8(), 7);
    EXPECT_EQ(r.read_u32(), 0xDEADBEEFu);
    EXPECT_EQ(r.read_u32(), 0u);
    EXPECT_EQ(r.remaining(), 0u);
}
