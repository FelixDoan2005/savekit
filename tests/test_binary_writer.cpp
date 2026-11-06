#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;

TEST(BinaryWriter, StartsEmpty) {
    savekit::BinaryWriter w;
    EXPECT_TRUE(w.bytes().empty());
}

TEST(BinaryWriter, WriteU8AppendsOneByte) {
    savekit::BinaryWriter w;
    w.write_u8(0xAB);
    EXPECT_EQ(w.bytes(), (Bytes{0xAB}));
}

TEST(BinaryWriter, WriteU32IsLittleEndian) {
    savekit::BinaryWriter w;
    w.write_u32(1000);  // 0x000003E8
    EXPECT_EQ(w.bytes(), (Bytes{0xE8, 0x03, 0x00, 0x00}));
}

TEST(BinaryWriter, WriteU32UsesAllFourBytes) {
    savekit::BinaryWriter w;
    w.write_u32(0x12345678);
    EXPECT_EQ(w.bytes(), (Bytes{0x78, 0x56, 0x34, 0x12}));
}

TEST(BinaryWriter, WritesAppendInOrder) {
    savekit::BinaryWriter w;
    w.write_u8(0x01);
    w.write_u32(2);
    EXPECT_EQ(w.bytes(), (Bytes{0x01, 0x02, 0x00, 0x00, 0x00}));
}

TEST(BinaryWriter, Writes0) {
    savekit::BinaryWriter w;
    w.write_u32(0);
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x00, 0x00}));
}