#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;

// ---- u16 ----

TEST(WriteU16, IsLittleEndian) {
    savekit::BinaryWriter w;
    w.write_u16(0x1234);
    EXPECT_EQ(w.bytes(), (Bytes{0x34, 0x12}));
}

TEST(WriteU16, ZeroAndMax) {
    savekit::BinaryWriter w;
    w.write_u16(0);
    w.write_u16(0xFFFF);
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0xFF, 0xFF}));
}

TEST(ReadU16, IsLittleEndian) {
    Bytes data{0x34, 0x12};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u16(), 0x1234);
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(ReadU16, WithOneByteLeftThrowsAndConsumesNothing) {
    Bytes data{0x34};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u16(), std::out_of_range);
    EXPECT_EQ(r.remaining(), 1u);
}

// ---- u64 ----

TEST(WriteU64, IsLittleEndian) {
    savekit::BinaryWriter w;
    w.write_u64(0x0123456789ABCDEFull);
    EXPECT_EQ(w.bytes(), (Bytes{0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01}));
}

TEST(WriteU64, SmallValueHasZeroUpperBytes) {
    savekit::BinaryWriter w;
    w.write_u64(1);
    EXPECT_EQ(w.bytes(), (Bytes{0x01, 0, 0, 0, 0, 0, 0, 0}));
}

TEST(ReadU64, IsLittleEndian) {
    Bytes data{0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u64(), 0x0123456789ABCDEFull);
}

TEST(ReadU64, KeepsTheHighBytes) {
    // If you shift a 32-bit number by 56 you lose these. This catches that.
    Bytes data{0, 0, 0, 0, 0, 0, 0, 0x80};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_u64(), 0x8000000000000000ull);
}

TEST(ReadU64, WithSevenBytesLeftThrowsAndConsumesNothing) {
    Bytes data{1, 2, 3, 4, 5, 6, 7};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_u64(), std::out_of_range);
    EXPECT_EQ(r.remaining(), 7u);
}

// ---- round trips ----

TEST(IntegerRoundTrip, AllSizesInOrder) {
    savekit::BinaryWriter w;
    w.write_u8(0xAB);
    w.write_u16(0xBEEF);
    w.write_u32(0xDEADBEEF);
    w.write_u64(0xFEEDFACECAFEBABEull);

    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_u8(), 0xAB);
    EXPECT_EQ(r.read_u16(), 0xBEEF);
    EXPECT_EQ(r.read_u32(), 0xDEADBEEFu);
    EXPECT_EQ(r.read_u64(), 0xFEEDFACECAFEBABEull);
    EXPECT_EQ(r.remaining(), 0u);
}
