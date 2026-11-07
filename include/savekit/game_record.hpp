#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace savekit {

// a saved chess game. the example data that changes between versions
//   version 1: white, black, moves
//   version 2: adds move_times
struct GameRecord {
    std::string white;
    std::string black;
    std::vector<std::uint16_t> moves;
    std::vector<std::uint16_t> move_times;   // new in version 2. empty = no times saved

    bool operator==(const GameRecord&) const = default;
};

// field ids. never change or reuse an id once files have been saved with it
inline constexpr std::uint16_t kFieldWhite = 1;
inline constexpr std::uint16_t kFieldBlack = 2;
inline constexpr std::uint16_t kFieldMoves = 3;
inline constexpr std::uint16_t kFieldMoveTimes = 4;   // added in version 2

// current program (version 2)
// save: writes every field. returns a payload (use wrap_payload to make a file)
std::vector<std::uint8_t> save_game(const GameRecord& game);

// load: reads a payload from any version. missing field = default, unknown id = skipped
GameRecord load_game(std::span<const std::uint8_t> payload);

// old program (version 1), kept so the tests can play both sides
std::vector<std::uint8_t> save_game_v1(const GameRecord& game);    // no move_times
GameRecord load_game_v1(std::span<const std::uint8_t> payload);    // skips move_times

}
