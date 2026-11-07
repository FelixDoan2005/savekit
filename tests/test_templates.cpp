#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "savekit/serialize.hpp"

using Bytes = std::vector<std::uint8_t>;
using savekit::BinaryReader;
using savekit::BinaryWriter;
using savekit::read_value;
using savekit::write_value;

namespace {

template <typename T>
Bytes bytes_of(const T& value) {
    BinaryWriter w;
    write_value(w, value);
    return w.bytes();
}

template <typename T>
T round_trip(const T& value) {
    BinaryWriter w;
    write_value(w, value);
    BinaryReader r(w.bytes());
    T out = read_value<T>(r);
    EXPECT_EQ(r.remaining(), 0u);   // read exactly what was written
    return out;
}

}  // namespace

// ---- the type trait (already written for you) ----

static_assert(savekit::is_vector_v<std::vector<std::uint16_t>>);
static_assert(savekit::is_vector_v<std::vector<std::string>>);
static_assert(!savekit::is_vector_v<std::string>);
static_assert(!savekit::is_vector_v<std::uint32_t>);

// ---- write_value: the exact bytes ----

TEST(WriteValue, U8) {
    EXPECT_EQ(bytes_of(std::uint8_t{0xAB}), (Bytes{0xAB}));
}

TEST(WriteValue, U16IsLittleEndian) {
    EXPECT_EQ(bytes_of(std::uint16_t{0x1234}), (Bytes{0x34, 0x12}));
}

TEST(WriteValue, U32IsLittleEndian) {
    EXPECT_EQ(bytes_of(std::uint32_t{1000}), (Bytes{0xE8, 0x03, 0x00, 0x00}));
}

TEST(WriteValue, U64IsLittleEndian) {
    EXPECT_EQ(bytes_of(std::uint64_t{0x0123456789ABCDEFull}),
              (Bytes{0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01}));
}

TEST(WriteValue, Float) {
    EXPECT_EQ(bytes_of(1.0f), (Bytes{0x00, 0x00, 0x80, 0x3F}));
}

TEST(WriteValue, Double) {
    EXPECT_EQ(bytes_of(1.0), (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}));
}

TEST(WriteValue, StringIsLengthThenLetters) {
    EXPECT_EQ(bytes_of(std::string("Felix")),
              (Bytes{0x05, 0x00, 0x00, 0x00, 'F', 'e', 'l', 'i', 'x'}));
}

TEST(WriteValue, VectorOfU16MatchesTheOldFunction) {
    const std::vector<std::uint16_t> v{1, 2, 3};
    BinaryWriter old;
    old.write_u16_vector(v);
    EXPECT_EQ(bytes_of(v), old.bytes());   // same file format as before
}

TEST(WriteValue, VectorOfU32) {
    EXPECT_EQ(bytes_of(std::vector<std::uint32_t>{1, 2}),
              (Bytes{0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}));
}

TEST(WriteValue, VectorOfStrings) {
    EXPECT_EQ(bytes_of(std::vector<std::string>{"a", "bc"}),
              (Bytes{0x02, 0x00, 0x00, 0x00,
                     0x01, 0x00, 0x00, 0x00, 'a',
                     0x02, 0x00, 0x00, 0x00, 'b', 'c'}));
}

TEST(WriteValue, EmptyVectorIsJustACount) {
    EXPECT_EQ(bytes_of(std::vector<std::uint32_t>{}), (Bytes{0x00, 0x00, 0x00, 0x00}));
}

TEST(WriteValue, VectorOfVectors) {
    const std::vector<std::vector<std::uint16_t>> v{{1}, {2, 3}};
    EXPECT_EQ(bytes_of(v), (Bytes{0x02, 0x00, 0x00, 0x00,
                                  0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
                                  0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x00}));
}

// ---- read_value: from fixed bytes ----

TEST(ReadValue, U16FromBytes) {
    const Bytes data{0x34, 0x12};
    BinaryReader r(data);
    EXPECT_EQ(read_value<std::uint16_t>(r), 0x1234);
}

TEST(ReadValue, U64FromBytes) {
    const Bytes data{0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
    BinaryReader r(data);
    EXPECT_EQ(read_value<std::uint64_t>(r), 0x0123456789ABCDEFull);
}

TEST(ReadValue, VectorOfStringsFromBytes) {
    const Bytes data{0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 'a',
                     0x02, 0x00, 0x00, 0x00, 'b', 'c'};
    BinaryReader r(data);
    EXPECT_EQ(read_value<std::vector<std::string>>(r), (std::vector<std::string>{"a", "bc"}));
}

// ---- round trips: every supported type ----

TEST(RoundTrip, Integers) {
    EXPECT_EQ(round_trip(std::uint8_t{255}), 255);
    EXPECT_EQ(round_trip(std::uint16_t{65535}), 65535);
    EXPECT_EQ(round_trip(std::uint32_t{0xDEADBEEF}), 0xDEADBEEFu);
    EXPECT_EQ(round_trip(std::uint64_t{0xFEEDFACECAFEBABEull}), 0xFEEDFACECAFEBABEull);
}

TEST(RoundTrip, Floats) {
    EXPECT_EQ(round_trip(-3.5f), -3.5f);
    EXPECT_EQ(round_trip(1e300), 1e300);
    EXPECT_EQ(round_trip(std::numeric_limits<float>::infinity()),
              std::numeric_limits<float>::infinity());
}

TEST(RoundTrip, Strings) {
    EXPECT_EQ(round_trip(std::string("")), "");
    EXPECT_EQ(round_trip(std::string("Stockfish")), "Stockfish");
    const std::string with_nul("a\0b", 3);
    EXPECT_EQ(round_trip(with_nul), with_nul);
}

TEST(RoundTrip, VectorsOfEveryKind) {
    EXPECT_EQ(round_trip(std::vector<std::uint8_t>{1, 2, 3}), (std::vector<std::uint8_t>{1, 2, 3}));
    EXPECT_EQ(round_trip(std::vector<std::uint16_t>{10, 20}), (std::vector<std::uint16_t>{10, 20}));
    EXPECT_EQ(round_trip(std::vector<std::uint64_t>{1ull << 40}), (std::vector<std::uint64_t>{1ull << 40}));
    EXPECT_EQ(round_trip(std::vector<float>{0.5f, -1.5f}), (std::vector<float>{0.5f, -1.5f}));
    EXPECT_EQ(round_trip(std::vector<std::string>{"x", "", "yz"}),
              (std::vector<std::string>{"x", "", "yz"}));
}

TEST(RoundTrip, EmptyVector) {
    EXPECT_TRUE(round_trip(std::vector<std::uint32_t>{}).empty());
}

TEST(RoundTrip, NestedVectors) {
    const std::vector<std::vector<std::uint16_t>> v{{1}, {}, {2, 3, 4}};
    EXPECT_EQ(round_trip(v), v);
}

TEST(RoundTrip, BigVector) {
    std::vector<std::uint32_t> v(5000);
    for (std::size_t i = 0; i < v.size(); i++) {
        v[i] = static_cast<std::uint32_t>(i * 7919);
    }
    EXPECT_EQ(round_trip(v), v);
}

TEST(RoundTrip, SeveralValuesInARow) {
    BinaryWriter w;
    write_value(w, std::string("Felix"));
    write_value(w, std::uint32_t{1500});
    write_value(w, std::vector<std::uint16_t>{1, 2, 3});
    write_value(w, 2.5f);

    BinaryReader r(w.bytes());
    EXPECT_EQ(read_value<std::string>(r), "Felix");
    EXPECT_EQ(read_value<std::uint32_t>(r), 1500u);
    EXPECT_EQ(read_value<std::vector<std::uint16_t>>(r), (std::vector<std::uint16_t>{1, 2, 3}));
    EXPECT_EQ(read_value<float>(r), 2.5f);
    EXPECT_EQ(r.remaining(), 0u);
}

// ---- hostile / broken input ----

TEST(ReadValue, TooFewBytesThrows) {
    const Bytes data{0x01, 0x02, 0x03};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::uint32_t>(r), std::out_of_range);
}

TEST(ReadValue, VectorWithHugeCountThrowsInsteadOfAllocating) {
    // Claims 4 billion u32s (16 GB) but only 4 bytes follow.
    const Bytes data{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::uint32_t>>(r), std::out_of_range);
}

TEST(ReadValue, VectorOfStringsWithHugeCountThrows) {
    const Bytes data{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 'a'};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::string>>(r), std::out_of_range);
}

TEST(ReadValue, NestedVectorWithLyingInnerCountThrows) {
    // outer count 1, inner count 4 billion
    const Bytes data{0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::vector<std::uint16_t>>>(r), std::out_of_range);
}

TEST(ReadValue, VectorWithTruncatedElementsThrows) {
    const Bytes data{0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};  // says 3, has 2
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::uint32_t>>(r), std::out_of_range);
}

TEST(ReadValue, FailedVectorReadLeavesReaderWhereItStarted) {
    const Bytes data{0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::uint32_t>>(r), std::out_of_range);
    EXPECT_EQ(r.position(), 0u);
    EXPECT_EQ(r.remaining(), data.size());
}

TEST(ReadValue, FailedNestedReadLeavesReaderWhereItStarted) {
    // second inner vector is cut short
    const Bytes data{0x02, 0x00, 0x00, 0x00,
                     0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
                     0x05, 0x00, 0x00, 0x00, 0x02, 0x00};
    BinaryReader r(data);
    EXPECT_THROW(read_value<std::vector<std::vector<std::uint16_t>>>(r), std::out_of_range);
    EXPECT_EQ(r.position(), 0u);
}
