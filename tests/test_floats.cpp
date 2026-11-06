#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;

TEST(WriteF32, OneIsTheIeeeBits) {
    savekit::BinaryWriter w;
    w.write_f32(1.0f);  // 0x3F800000
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x80, 0x3F}));
}

TEST(WriteF32, NegativeNumber) {
    savekit::BinaryWriter w;
    w.write_f32(-2.5f);  // 0xC0200000
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x20, 0xC0}));
}

TEST(WriteF64, OneIsTheIeeeBits) {
    savekit::BinaryWriter w;
    w.write_f64(1.0);  // 0x3FF0000000000000
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}));
}

TEST(ReadF32, OneFromBytes) {
    Bytes data{0x00, 0x00, 0x80, 0x3F};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_f32(), 1.0f);
}

TEST(ReadF32, TooFewBytesThrows) {
    Bytes data{0x00, 0x00, 0x80};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_f32(), std::out_of_range);
}

TEST(FloatRoundTrip, F32Values) {
    for (float v : {0.0f, 1.5f, -3.14159f, 1e-30f, 1e30f,
                    std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}) {
        savekit::BinaryWriter w;
        w.write_f32(v);
        savekit::BinaryReader r(w.bytes());
        EXPECT_EQ(r.read_f32(), v);
    }
}

TEST(FloatRoundTrip, F64Values) {
    for (double v : {0.0, 1.5, -3.14159265358979, 1e-300, 1e300,
                     std::numeric_limits<double>::infinity()}) {
        savekit::BinaryWriter w;
        w.write_f64(v);
        savekit::BinaryReader r(w.bytes());
        EXPECT_EQ(r.read_f64(), v);
    }
}

TEST(FloatRoundTrip, NegativeZeroKeepsItsSign) {
    savekit::BinaryWriter w;
    w.write_f32(-0.0f);
    savekit::BinaryReader r(w.bytes());
    EXPECT_TRUE(std::signbit(r.read_f32()));
}

TEST(FloatRoundTrip, NanStaysNan) {
    savekit::BinaryWriter w;
    w.write_f32(std::numeric_limits<float>::quiet_NaN());
    savekit::BinaryReader r(w.bytes());
    EXPECT_TRUE(std::isnan(r.read_f32()));
}

TEST(FloatRoundTrip, BitsAreIdenticalNotJustClose) {
    const float v = 0.1f;  // can't be stored exactly, so this checks nothing got "fixed"
    savekit::BinaryWriter w;
    w.write_f32(v);
    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(std::bit_cast<std::uint32_t>(r.read_f32()), std::bit_cast<std::uint32_t>(v));
}
