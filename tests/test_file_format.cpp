#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "savekit/crc32.hpp"
#include "savekit/file_format.hpp"

using Bytes = std::vector<std::uint8_t>;

namespace {

const Bytes kPayload{0x01, 0x02, 0x03};

}  // namespace

// ---- wrap_payload: the layout ----

TEST(WrapPayload, HeaderLayout) {
    const Bytes file = savekit::wrap_payload(kPayload);

    ASSERT_EQ(file.size(), savekit::kHeaderSize + kPayload.size());
    // magic
    EXPECT_EQ((Bytes(file.begin(), file.begin() + 5)), (Bytes{'G', 'R', 'A', 'C', 'E'}));
    // version 1, little-endian u16
    EXPECT_EQ((Bytes(file.begin() + 5, file.begin() + 7)), (Bytes{0x01, 0x00}));
    // payload size 3, little-endian u32
    EXPECT_EQ((Bytes(file.begin() + 7, file.begin() + 11)), (Bytes{0x03, 0x00, 0x00, 0x00}));
    // crc32 of {1,2,3} = 0x55BC801D, little-endian u32
    EXPECT_EQ((Bytes(file.begin() + 11, file.begin() + 15)), (Bytes{0x1D, 0x80, 0xBC, 0x55}));
    // then the payload itself
    EXPECT_EQ((Bytes(file.begin() + 15, file.end())), kPayload);
}

TEST(WrapPayload, EmptyPayloadIsJustTheHeader) {
    const Bytes file = savekit::wrap_payload({});
    EXPECT_EQ(file.size(), savekit::kHeaderSize);
}

// ---- parse_file: good files ----

TEST(ParseFile, RoundTrip) {
    const auto parsed = savekit::parse_file(savekit::wrap_payload(kPayload));
    EXPECT_EQ(parsed.payload, kPayload);
    EXPECT_EQ(parsed.version, savekit::kFormatVersion);
}

TEST(ParseFile, EmptyPayloadRoundTrip) {
    const auto parsed = savekit::parse_file(savekit::wrap_payload({}));
    EXPECT_TRUE(parsed.payload.empty());
}

TEST(ParseFile, LargerPayloadRoundTrip) {
    Bytes big(100000);
    for (std::size_t i = 0; i < big.size(); i++) {
        big[i] = static_cast<std::uint8_t>(i * 31 + 7);
    }
    EXPECT_EQ(savekit::parse_file(savekit::wrap_payload(big)).payload, big);
}

// ---- parse_file: bad files ----

TEST(ParseFile, EmptyInputIsRejected) {
    EXPECT_THROW(savekit::parse_file({}), savekit::FormatError);
}

TEST(ParseFile, TooShortForAHeaderIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file.resize(savekit::kHeaderSize - 1);
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, WrongMagicIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file[0] = 'X';
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, VersionZeroIsRejected) {
    EXPECT_THROW(savekit::parse_file(savekit::wrap_payload(kPayload, 0)), savekit::FormatError);
}

TEST(ParseFile, VersionFromTheFutureIsRejected) {
    EXPECT_THROW(savekit::parse_file(savekit::wrap_payload(kPayload, 999)), savekit::FormatError);
}

TEST(ParseFile, TruncatedPayloadIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file.pop_back();
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, TrailingGarbageIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file.push_back(0x00);
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, CorruptedPayloadByteIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file.back() ^= 0xFF;
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, CorruptedChecksumIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file[11] ^= 0x01;
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

TEST(ParseFile, LyingPayloadSizeIsRejected) {
    Bytes file = savekit::wrap_payload(kPayload);
    file[7] = 0xFF;
    file[8] = 0xFF;
    file[9] = 0xFF;
    file[10] = 0xFF;  // claims a 4 GB payload
    EXPECT_THROW(savekit::parse_file(file), savekit::FormatError);
}

// Flip every single bit of a valid file, one at a time. Every one must be caught.
TEST(ParseFile, AnySingleBitFlipIsRejected) {
    const Bytes good = savekit::wrap_payload(Bytes{'F', 'e', 'l', 'i', 'x', 1, 2, 3, 4, 5, 6, 7, 8});
    for (std::size_t byte = 0; byte < good.size(); byte++) {
        for (int bit = 0; bit < 8; bit++) {
            Bytes bad = good;
            bad[byte] = static_cast<std::uint8_t>(bad[byte] ^ (1u << bit));
            EXPECT_THROW(savekit::parse_file(bad), savekit::FormatError)
                << "flipping bit " << bit << " of byte " << byte << " was not caught";
        }
    }
}

// ---- files on disk ----

TEST(FileIo, WriteThenReadGivesTheSameBytes) {
    const auto path = std::filesystem::temp_directory_path() / "savekit_test_roundtrip.bin";
    const Bytes file = savekit::wrap_payload(kPayload);

    savekit::write_file(path, file);
    const Bytes loaded = savekit::read_file(path);
    std::filesystem::remove(path);

    EXPECT_EQ(loaded, file);
    EXPECT_EQ(savekit::parse_file(loaded).payload, kPayload);
}

TEST(FileIo, BinaryBytesSurviveNewlinesAndZeros) {
    // 0x0A and 0x0D get mangled if the file is opened in text mode on Windows.
    const auto path = std::filesystem::temp_directory_path() / "savekit_test_binary.bin";
    const Bytes tricky{0x00, 0x0A, 0x0D, 0x0A, 0x1A, 0xFF, 0x00};

    savekit::write_file(path, tricky);
    const Bytes loaded = savekit::read_file(path);
    std::filesystem::remove(path);

    EXPECT_EQ(loaded, tricky);
}

TEST(FileIo, ReadingAMissingFileThrows) {
    const auto path = std::filesystem::temp_directory_path() / "savekit_does_not_exist.bin";
    EXPECT_THROW(savekit::read_file(path), std::runtime_error);
}
