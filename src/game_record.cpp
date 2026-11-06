#include "savekit/game_record.hpp"

#include "savekit/fields.hpp"

namespace savekit {

std::vector<std::uint8_t> save_game_v1(const GameRecord& game) {
    FieldWriter w;
    w.write_string(kFieldWhite, game.white);
    w.write_string(kFieldBlack, game.black);
    w.write_u16_vector(kFieldMoves, game.moves);
    return w.bytes();
}

std::vector<std::uint8_t> save_game(const GameRecord& game) {
    FieldWriter w;
    w.write_string(kFieldWhite, game.white);
    w.write_string(kFieldBlack, game.black);
    w.write_u16_vector(kFieldMoves, game.moves);
    w.write_u16_vector(kFieldMoveTimes, game.move_times);   // new in version 2
    return w.bytes();
}

GameRecord load_game_v1(std::span<const std::uint8_t> payload) {
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
        }
        // Any other id: do nothing. The field is skipped for free, because
        // next() already moved past its data using the stored length.
    }

    return game;
}

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
