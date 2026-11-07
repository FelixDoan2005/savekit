# savekit

[![CI](https://github.com/FelixDoan2005/savekit/actions/workflows/ci.yml/badge.svg)](https://github.com/FelixDoan2005/savekit/actions/workflows/ci.yml)

A small C++20 library that saves data to a **compact binary file format** and loads it back safely, even when the format changes over time.

- **Compact binary format**: fixed-size types, little-endian, the same bytes on every machine
- **Corruption detection**: a magic number, a size check and a CRC-32 checksum
- **Schema evolution**: old files load in new code, and new files load in old code
- **Hostile-input safe**: every length read from a file is checked before anything is allocated
- **Templates**: one `write_value` / `read_value` for numbers, strings and vectors of anything
- **`savekit-dump`**: a command-line tool that prints any savekit file as readable text
- **200+ unit tests**, run in CI with AddressSanitizer and UBSan

Chess games are used as the example data (white, black, moves, move times), but nothing in the library is chess-specific. In the examples, each move is one `u16`: the starting square in the high byte and the destination square in the low byte (a1 = 0 ... h8 = 63), so `3100` is e2-e4.

## See it work

The repo includes two sample files of the same game (1. e4 e5 2. Nf3 Nc6 3. Bb5): `game_v1.bin` was written by version 1 of the program, and `game_v2.bin` by version 2, which added a "move times" field.

```
$ savekit-dump examples/game_v2.bin
savekit file, version 1
payload: 78 bytes, crc32 0x027AC304
fields:
  id 1  String  "Felix"
  id 2  String  "Stockfish"
  id 3  U16Vector  [3100, 13348, 1557, 14634, 1313]
  id 4  U16Vector  [120, 340, 95, 410, 220]

$ savekit-dump examples/game_v1.bin
savekit file, version 1
payload: 57 bytes, crc32 0xEFEAD46B
fields:
  id 1  String  "Felix"
  id 2  String  "Stockfish"
  id 3  U16Vector  [3100, 13348, 1557, 14634, 1313]
```

The version shown is the file header's format version. Version 2 of the program added a field, not a new header, so both files are the same header version i.e. version 1.

The only difference between the two files is the extra `id 4` field. Version 1 code can read the version 2 file (it skips `id 4`), and version 2 code can read the version 1 file (it uses an empty default for `id 4`).

A damaged file is refused:

```
$ savekit-dump damaged.bin
error: not a valid savekit file: checksum mismatch: the file is corrupted
```

## The file format

A file is a 15-byte header followed by the payload. All numbers are little-endian.

```
 0         5       7         11        15
 +---------+-------+---------+---------+-----------------------+
 | "GRACE" | ver   | size    | crc32   | payload ...           |
 | 5 bytes | u16   | u32     | u32     | `size` bytes          |
 +---------+-------+---------+---------+-----------------------+
 |<------------ header (15 bytes) ---->|
```

| Field | Size | Purpose |
|---|---|---|
| magic | 5 bytes | `GRACE`, so a file that isn't ours is rejected straight away |
| version | u16 | the version of the container format (the header layout) |
| size | u32 | how many payload bytes follow (catches truncated or padded files) |
| crc32 | u32 | CRC-32 of the payload (catches damage) |

### The payload is a list of tagged fields

Every value in the payload is stored with a label, so the format can grow without breaking old files:

```
 +--------+--------+------------+----------------------+
 | id u16 | type u8| length u32 | data (length bytes)  |
 +--------+--------+------------+----------------------+
```

| Type | Value | Data |
|---|---|---|
| `U32` | 1 | a 32-bit number |
| `String` | 2 | length (u32), then the bytes |
| `U16Vector` | 3 | count (u32), then each element as u16 |
| `F32` | 4 | the raw IEEE-754 bits of a float |

### How versioning works

Because every field carries its `length`, a reader can step over a field without understanding it:

- **Old code reading a new file:** a field with an unknown id is skipped using its length.
- **New code reading an old file:** a missing field keeps its default value.

Field ids are part of the format: once a file has been written with an id, that id never changes meaning. Adding a field means picking a new id, and does not need a new container version. The container version only changes if the 15-byte header itself changes.

## Using the library

```cpp
#include "savekit/file_format.hpp"
#include "savekit/game_record.hpp"

savekit::GameRecord game;
game.white = "Felix";
game.black = "Stockfish";
game.moves = {3100, 13348, 1557, 14634, 1313};   // e2e4 e7e5 g1f3 b8c6 f1b5
game.move_times = {120, 340, 95, 410, 220};

// save: fields -> payload -> header + checksum -> file
savekit::write_file("game.bin", savekit::wrap_payload(savekit::save_game(game)));

// load: file -> checks -> payload -> fields
auto file = savekit::read_file("game.bin");
auto parsed = savekit::parse_file(file);   // throws savekit::FormatError if the file is bad
savekit::GameRecord loaded = savekit::load_game(parsed.payload);
```

To store your own data, write your own struct and give each member a field id. The templates pick the right encoding from the type:

```cpp
savekit::FieldWriter w;
w.write(1, std::string("Felix"));
w.write(2, std::uint32_t{1500});
w.write(3, std::vector<std::uint16_t>{1, 2, 3});

savekit::FieldReader r(w.bytes());
savekit::Field f;
while (r.next(f)) {
    if (f.id == 1) { std::string name = savekit::as<std::string>(f); }
}
```

At the lower level, `write_value(writer, x)` and `read_value<T>(reader)` work for `uint8/16/32/64_t`, `float`, `double`, `std::string` and `std::vector<T>` of any of those, including nested vectors.

## Building

Requires CMake 3.20+ and a C++20 compiler (GCC, Clang or MSVC). GoogleTest is downloaded automatically.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/tools/savekit-dump examples/game_v2.bin
```

| CMake option | Default | Meaning |
|---|---|---|
| `SAVEKIT_BUILD_TESTS` | `ON` | build the GoogleTest suite |
| `SAVEKIT_BUILD_TOOLS` | `ON` | build `savekit-dump` |
| `SAVEKIT_ENABLE_SANITIZERS` | `OFF` | AddressSanitizer + UBSan (GCC/Clang on Linux or macOS) |
| `SAVEKIT_WARNINGS_AS_ERRORS` | `OFF` | `-Werror` / `/WX` (on in CI) |

## Design notes

- **Never trust a length from a file.** Every count and length is checked against the bytes actually left before anything is reserved or copied, so a file claiming a 4 GB string is rejected immediately. A failed read leaves the reader where it started.
- **Fixed-size types and one byte order.** `int` and `long` vary between platforms, so the format uses `uint8_t` to `uint64_t` and always writes little-endian. Floats are written as their raw bits with `std::bit_cast`.
- **Tests include hostile cases.** One test flips every single bit of a valid file in turn and requires every flip to be rejected. Others cover lying lengths, truncated data, wrong types and unknown fields. The CRC-32 is checked against zlib's reference values (`"123456789"` gives `0xCBF43926`).
- **Two error types.** The low-level reader throws `std::out_of_range` when it runs out of bytes, and the higher layers turn that into `FormatError`, so callers only handle one kind of "bad file".
- **The dump tool survives broken data.** A field whose data doesn't match its type is shown as raw bytes instead of aborting, because a debugging tool is most needed when the data is wrong.
- **The CRC is for accidents, not attackers.** CRC-32 detects damage but is easy to forge on purpose, so this is integrity checking and not security.

## Project layout

```
include/savekit/   public headers
src/               library code
tools/dump/        savekit-dump command-line tool
tests/             GoogleTest unit tests
examples/          sample files (version 1 and version 2)
.github/workflows/ CI: GCC and Clang with sanitizers, MSVC
```

| Layer | Files |
|---|---|
| bytes to values and back | `binary_writer`, `binary_reader` |
| templates over all types | `serialize.hpp` |
| checksum | `crc32` |
| file header and disk I/O | `file_format` |
| tagged fields | `fields` |
| example data | `game_record` |
| readable output | `dump` |
