#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

using Bytes = std::vector<std::uint8_t>;

// ---- writer ----

TEST(WriteString, EmptyStringIsJustALengthOfZero) {
    savekit::BinaryWriter w;
    w.write_string("");
    EXPECT_EQ(w.bytes(), (Bytes{0x00, 0x00, 0x00, 0x00}));
}

TEST(WriteString, LengthThenCharacters) {
    savekit::BinaryWriter w;
    w.write_string("Felix");
    EXPECT_EQ(w.bytes(), (Bytes{0x05, 0x00, 0x00, 0x00, 'F', 'e', 'l', 'i', 'x'}));
}

TEST(WriteString, AppendsAfterEarlierWrites) {
    savekit::BinaryWriter w;
    w.write_u8(0xFF);
    w.write_string("hi");
    EXPECT_EQ(w.bytes(), (Bytes{0xFF, 0x02, 0x00, 0x00, 0x00, 'h', 'i'}));
}

// ---- reader ----

TEST(ReadString, ReadsLengthPrefixedText) {
    Bytes data{0x05, 0x00, 0x00, 0x00, 'F', 'e', 'l', 'i', 'x'};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_string(), "Felix");
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(ReadString, EmptyString) {
    Bytes data{0x00, 0x00, 0x00, 0x00};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_string(), "");
    EXPECT_EQ(r.remaining(), 0u);
}

TEST(ReadString, StopsAfterItsOwnBytes) {
    Bytes data{0x02, 0x00, 0x00, 0x00, 'o', 'k', 0x2A};
    savekit::BinaryReader r(data);
    EXPECT_EQ(r.read_string(), "ok");
    EXPECT_EQ(r.read_u8(), 0x2A);
}

// ---- hostile / broken input ----

TEST(ReadString, MissingLengthThrows) {
    Bytes data{0x05, 0x00};  // not even a full u32
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_string(), std::out_of_range);
}

TEST(ReadString, TruncatedTextThrows) {
    Bytes data{0x05, 0x00, 0x00, 0x00, 'F', 'e'};  // says 5, has 2
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_string(), std::out_of_range);
}

TEST(ReadString, HugeLengthThrowsInsteadOfAllocating) {
    // Claims a 4 GB string, but only 1 byte follows. Must throw, not try to allocate.
    Bytes data{0xFF, 0xFF, 0xFF, 0xFF, 'x'};
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_string(), std::out_of_range);
}

TEST(ReadString, FailedReadLeavesReaderWhereItStarted) {
    Bytes data{0x64, 0x00, 0x00, 0x00, 'a', 'b', 'c'};  // says 100, has 3
    savekit::BinaryReader r(data);
    EXPECT_THROW(r.read_string(), std::out_of_range);
    EXPECT_EQ(r.remaining(), 7u);  // nothing consumed, not even the length
}

// ---- round trips ----

TEST(StringRoundTrip, PlainText) {
    savekit::BinaryWriter w;
    w.write_string("Stockfish");
    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_string(), "Stockfish");
}

TEST(StringRoundTrip, EmbeddedNulBytesSurvive) {
    const std::string tricky("a\0b\0", 4);
    savekit::BinaryWriter w;
    w.write_string(tricky);
    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_string(), tricky);
}

TEST(StringRoundTrip, ManyFieldsInOrder) {
    savekit::BinaryWriter w;
    w.write_string("Felix");
    w.write_u32(1500);
    w.write_string("Stockfish");

    savekit::BinaryReader r(w.bytes());
    EXPECT_EQ(r.read_string(), "Felix");
    EXPECT_EQ(r.read_u32(), 1500u);
    EXPECT_EQ(r.read_string(), "Stockfish");
    EXPECT_EQ(r.remaining(), 0u);
}
