#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "savekit/fields.hpp"
#include "savekit/file_format.hpp"
#include "savekit/game_record.hpp"

using Bytes = std::vector<std::uint8_t>;
using Moves = std::vector<std::uint16_t>;
using savekit::FieldWriter;
using savekit::FormatError;
using savekit::GameRecord;

namespace {

GameRecord sample_game() {
    GameRecord g;
    g.white = "Felix";
    g.black = "Stockfish";
    g.moves = {0x0C1C, 0x3C2C, 0x0D1D};
    g.move_times = {120, 340, 95};
    return g;
}

}  // namespace

// ---- same version on both sides ----

TEST(GameRecord, CurrentVersionRoundTrip) {
    const GameRecord g = sample_game();
    EXPECT_EQ(savekit::load_game(savekit::save_game(g)), g);
}

TEST(GameRecord, EmptyGameRoundTrip) {
    const GameRecord g;
    EXPECT_EQ(savekit::load_game(savekit::save_game(g)), g);
}

TEST(GameRecord, WorksInsideAFileWithHeaderAndChecksum) {
    const GameRecord g = sample_game();
    const Bytes file = savekit::wrap_payload(savekit::save_game(g));
    const auto parsed = savekit::parse_file(file);
    EXPECT_EQ(savekit::load_game(parsed.payload), g);
}

// ---- the point of the whole milestone ----

TEST(Evolution, NewCodeLoadsOldFile_MissingFieldGetsDefault) {
    const GameRecord g = sample_game();
    const Bytes old_file = savekit::save_game_v1(g);   // written before move_times existed

    const GameRecord loaded = savekit::load_game(old_file);

    EXPECT_EQ(loaded.white, "Felix");
    EXPECT_EQ(loaded.black, "Stockfish");
    EXPECT_EQ(loaded.moves, g.moves);
    EXPECT_TRUE(loaded.move_times.empty());            // default, not garbage
}

TEST(Evolution, OldCodeLoadsNewFile_UnknownFieldIsSkipped) {
    const GameRecord g = sample_game();
    const Bytes new_file = savekit::save_game(g);      // has move_times, which v1 has never heard of

    const GameRecord loaded = savekit::load_game_v1(new_file);

    EXPECT_EQ(loaded.white, "Felix");
    EXPECT_EQ(loaded.black, "Stockfish");
    EXPECT_EQ(loaded.moves, g.moves);
    EXPECT_TRUE(loaded.move_times.empty());            // v1 can't know about them
}

TEST(Evolution, OldFileRoundTripsThroughOldCode) {
    GameRecord g = sample_game();
    g.move_times.clear();
    EXPECT_EQ(savekit::load_game_v1(savekit::save_game_v1(g)), g);
}

TEST(Evolution, NewFileIsBiggerThanOldFile) {
    const GameRecord g = sample_game();
    EXPECT_GT(savekit::save_game(g).size(), savekit::save_game_v1(g).size());
}

// ---- how forgiving the loader is ----

TEST(Loader, UnknownFieldsAreSkippedWhereverTheyAre) {
    FieldWriter w;
    w.write_u32(999, 1234);                  // unknown, first
    w.write_string(savekit::kFieldWhite, "Felix");
    w.write_string(777, "from the future");  // unknown, in the middle
    w.write_string(savekit::kFieldBlack, "Stockfish");
    w.write_u16_vector(savekit::kFieldMoves, {1, 2});
    w.write_u32(888, 5);                     // unknown, last

    const GameRecord loaded = savekit::load_game(w.bytes());
    EXPECT_EQ(loaded.white, "Felix");
    EXPECT_EQ(loaded.black, "Stockfish");
    EXPECT_EQ(loaded.moves, (Moves{1, 2}));
}

TEST(Loader, FieldOrderDoesNotMatter) {
    FieldWriter w;
    w.write_u16_vector(savekit::kFieldMoves, {7, 8, 9});
    w.write_string(savekit::kFieldBlack, "B");
    w.write_string(savekit::kFieldWhite, "A");

    const GameRecord loaded = savekit::load_game(w.bytes());
    EXPECT_EQ(loaded.white, "A");
    EXPECT_EQ(loaded.black, "B");
    EXPECT_EQ(loaded.moves, (Moves{7, 8, 9}));
}

TEST(Loader, MissingFieldsGetDefaults) {
    FieldWriter w;
    w.write_string(savekit::kFieldWhite, "OnlyWhite");

    const GameRecord loaded = savekit::load_game(w.bytes());
    EXPECT_EQ(loaded.white, "OnlyWhite");
    EXPECT_TRUE(loaded.black.empty());
    EXPECT_TRUE(loaded.moves.empty());
    EXPECT_TRUE(loaded.move_times.empty());
}

TEST(Loader, EmptyPayloadGivesDefaultGame) {
    const Bytes empty;
    EXPECT_EQ(savekit::load_game(empty), GameRecord{});
}

TEST(Loader, DuplicateFieldLastOneWins) {
    FieldWriter w;
    w.write_string(savekit::kFieldWhite, "first");
    w.write_string(savekit::kFieldWhite, "second");
    EXPECT_EQ(savekit::load_game(w.bytes()).white, "second");
}

// ---- damaged or wrong data ----

TEST(Loader, KnownFieldWithWrongTypeThrows) {
    FieldWriter w;
    w.write_u32(savekit::kFieldWhite, 5);    // white must be a string
    EXPECT_THROW(savekit::load_game(w.bytes()), FormatError);
}

TEST(Loader, TruncatedPayloadThrows) {
    Bytes payload = savekit::save_game(sample_game());
    payload.resize(payload.size() - 3);
    EXPECT_THROW(savekit::load_game(payload), FormatError);
}

TEST(Loader, EveryTruncationEitherThrowsOrLoadsCleanly) {
    // Chop the payload at every possible length. Must never crash or hang.
    const Bytes payload = savekit::save_game(sample_game());
    for (std::size_t n = 0; n < payload.size(); n++) {
        const std::span<const std::uint8_t> cut(payload.data(), n);
        try {
            (void)savekit::load_game(cut);
        } catch (const FormatError&) {
        }
    }
    SUCCEED();
}
