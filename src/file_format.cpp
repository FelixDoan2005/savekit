#include "savekit/file_format.hpp"
#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"
#include "savekit/crc32.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>

namespace savekit {

//reminder - payload = actual data (the bytes you want to save) without the header
//file = header (magic, version, size, crc) + payload

// puts the 15 byte header in front of the payload and returns the whole file as bytes
std::vector<std::uint8_t> wrap_payload(std::span<const std::uint8_t> payload,
                                       std::uint16_t version) {
    BinaryWriter w;

    // magic (5 bytes)
    for (std::size_t i=0; i<kMagic.size(); i++) {
        w.write_u8(kMagic[i]);
    }

    // version (2 bytes)
    w.write_u16(version);

    // payload size (4 bytes)
    if (payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("payload too large");
    } else {
        w.write_u32(static_cast<std::uint32_t>(payload.size()));
    }

    // crc of the payload (4 bytes)
    w.write_u32(crc32(payload));

    // the payload itself
    for (std::size_t i = 0; i < payload.size(); i++) {
        w.write_u8(payload[i]);
    }

    return w.bytes();
}

// checks the header and crc. gives back the payload only if everything is fine
ParsedFile parse_file(std::span<const std::uint8_t> file) {
    // is it even long enough for a header? (check before making the reader)
    if (file.size() < kHeaderSize) {
        throw FormatError("file is too short to be a savekit file");
    }

    BinaryReader r(file);

    // magic: first 5 bytes must spell GRACE
    for (std::size_t i = 0; i < kMagic.size(); i++) {
        if (r.read_u8() != kMagic[i]) {
            throw FormatError("wrong magic number -> not a savekit file");
        }
    }

    // version: one we understand
    std::uint16_t version = r.read_u16();
    if (version == 0 || version > kFormatVersion) {
        throw FormatError("unsupported format version");
    }

    // the size and crc the header says
    std::uint32_t size = r.read_u32();
    std::uint32_t stored_crc = r.read_u32();

    // size must match the bytes left (too short and too long are both bad)
    if (size != r.remaining()) {
        throw FormatError("payload size does not match the file size");
    }

    // copy the payload out
    std::vector<std::uint8_t> payload(size);
    for (std::uint32_t i = 0; i < size; i++) {
        payload[i] = r.read_u8();
    }

    // fingerprint of what we read must match the one in the header
    if (crc32(payload) != stored_crc) {
        throw FormatError("checksum mismatch: the file is corrupted");
    }

    // all checks passed
    ParsedFile result;
    result.version = version;
    result.payload = payload;
    return result;
}

// saves bytes to a file on disk
void write_file(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    // binary mode, so Windows doesn't change 0x0A bytes into 0x0D 0x0A
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("write_file: could not open the file");
    }

    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    out.close();

    if (!out) {
        throw std::runtime_error("write_file: writing failed");
    }
}

// loads a whole file from disk into bytes
std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("read_file: could not open the file");
    }

    in.seekg(0, std::ios::end);                              // jump to the end
    std::size_t n = static_cast<std::size_t>(in.tellg());    // end position = file size
    in.seekg(0, std::ios::beg);                              // jump back to the start

    std::vector<std::uint8_t> bytes(n);
    for (std::size_t i =0; i<n; i++) {
        bytes[i] = static_cast<std::uint8_t>(in.get());
    }
    return bytes;
}

}
