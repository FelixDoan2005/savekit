#include "savekit/game_record.hpp"

#include "savekit/fields.hpp"

namespace savekit {

// old program (version 1) save: white, black and moves only
std::vector<std::uint8_t> save_game_v1(const GameRecord& game) {
    FieldWriter w;
    w.write_string(kFieldWhite, game.white);
    w.write_string(kFieldBlack, game.black);
    w.write_u16_vector(kFieldMoves, game.moves);
    return w.bytes();
}

// current program (version 2) save: same plus move times
std::vector<std::uint8_t> save_game(const GameRecord& game) {
    FieldWriter w;
    w.write_string(kFieldWhite, game.white);
    w.write_string(kFieldBlack, game.black);
    w.write_u16_vector(kFieldMoves, game.moves);
    w.write_u16_vector(kFieldMoveTimes, game.move_times);   // new in version 2
    return w.bytes();
}

// old program (version 1) load: only knows white, black and moves
GameRecord load_game_v1(std::span<const std::uint8_t> payload) {
    GameRecord game;
    FieldReader r(payload);
    Field f;

    while (r.next(f)) {   // go through every field, the id says where it goes
        if (f.id == kFieldWhite) {
            game.white = as_string(f);
        } else if (f.id == kFieldBlack) {
            game.black = as_string(f);
        } else if (f.id == kFieldMoves) {
            game.moves = as_u16_vector(f);
        }
        // any other id: do nothing. next() already moved past its data using the length, so its skipped
    }

    return game;
}

// current program (version 2) load: reads any version
GameRecord load_game(std::span<const std::uint8_t> payload) {
    GameRecord game;
    FieldReader r(payload);
    Field f;

    while (r.next(f)) {
        if (f.id == kFieldWhite) {
            game.white = as_string(f);
        } else if (f.id == kFieldBlack) {
            game.black = as_string(f);
        } else if (f.id == kFieldMoves) {
            game.moves = as_u16_vector(f);
        } else if (f.id == kFieldMoveTimes) {
            game.move_times = as_u16_vector(f);   // missing in old files: stays empty
        }
    }

    return game;
}

}
