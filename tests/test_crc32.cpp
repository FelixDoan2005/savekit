#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>
#include <vector>

#include "savekit/crc32.hpp"

namespace {

std::uint32_t crc_of(std::string_view text) {
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    return savekit::crc32(bytes);
}

}  // namespace

// Reference values from zlib's crc32().

TEST(Crc32, EmptyInputIsZero) {
    EXPECT_EQ(crc_of(""), 0x00000000u);
}

TEST(Crc32, StandardCheckValue) {
    EXPECT_EQ(crc_of("123456789"), 0xCBF43926u);
}

TEST(Crc32, SingleLetter) {
    EXPECT_EQ(crc_of("a"), 0xE8B7BE43u);
}

TEST(Crc32, LongerSentence) {
    EXPECT_EQ(crc_of("The quick brown fox jumps over the lazy dog"), 0x414FA339u);
}

TEST(Crc32, RawBytesNotJustText) {
    const std::vector<std::uint8_t> bytes{0x01, 0x02, 0x03};
    EXPECT_EQ(savekit::crc32(bytes), 0x55BC801Du);
}

TEST(Crc32, ChangingOneByteChangesTheResult) {
    EXPECT_NE(crc_of("hello world"), crc_of("hello worle"));
}

TEST(Crc32, OrderMatters) {
    EXPECT_NE(crc_of("ab"), crc_of("ba"));
}

TEST(Crc32, SameInputGivesSameResult) {
    EXPECT_EQ(crc_of("savekit"), crc_of("savekit"));
}
