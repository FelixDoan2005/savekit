#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace savekit {

// A saved chess game. This is the thing that changes between versions:
//
//   version 1:  white, black, moves
//   version 2:  adds move_times   (centiseconds spent on each move)
//
struct GameRecord {
    std::string white;
    std::string black;
    std::vector<std::uint16_t> moves;
    std::vector<std::uint16_t> move_times;   // new in version 2. Empty = "no times recorded"

    bool operator==(const GameRecord&) const = default;
};

// Field IDs. NEVER change or reuse an ID once a file has been written with it.
inline constexpr std::uint16_t kFieldWhite = 1;
inline constexpr std::uint16_t kFieldBlack = 2;
inline constexpr std::uint16_t kFieldMoves = 3;
inline constexpr std::uint16_t kFieldMoveTimes = 4;   // added in version 2

// ---- the CURRENT program (version 2) ----

// Writes every field, including move_times. Returns a payload (use wrap_payload to make a file).
std::vector<std::uint8_t> save_game(const GameRecord& game);

// Reads a payload written by ANY version.
//   - a field that is missing        -> keeps its default (empty)
//   - a field with an unknown ID     -> skipped
//   - a known field with wrong type  -> FormatError
//   - a damaged payload              -> FormatError
//   - a field that appears twice     -> the last one wins
// Must not depend on the ORDER of the fields.
GameRecord load_game(std::span<const std::uint8_t> payload);

// ---- the OLD program (version 1), kept so tests can play both sides ----

// Writes only white, black and moves, like version 1 of the program did.
std::vector<std::uint8_t> save_game_v1(const GameRecord& game);

// Reads only white, black and moves, like version 1 did. Everything else is skipped,
// including move_times, which it has never heard of.
GameRecord load_game_v1(std::span<const std::uint8_t> payload);

}
