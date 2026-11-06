#include "savekit/file_format.hpp"
#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"
#include "savekit/crc32.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>

namespace savekit {

std::vector<std::uint8_t> wrap_payload(std::span<const std::uint8_t> payload,
                                       std::uint16_t version) {
    BinaryWriter w;
    for (std::size_t i=0; i<kMagic.size(); i++) {
        w.write_u8(kMagic[i]);
    }

    w.write_u16(version);
    if (payload.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("payload too large");
    } else {
        w.write_u32(static_cast<std::uint32_t>(payload.size()));
    }
    w.write_u32(crc32(payload));

    for (std::size_t i = 0; i < payload.size(); i++) {
        w.write_u8(payload[i]);
    }

    return w.bytes();
}

ParsedFile parse_file(std::span<const std::uint8_t> file) {
    if (file.size() < kHeaderSize) {
        throw FormatError("file is too short to be a savekit file");
    }

    BinaryReader r(file);

    // Magic match check
    for (std::size_t i = 0; i < kMagic.size(); i++) {
        if (r.read_u8() != kMagic[i]) {
            throw FormatError("wrong magic number -> not a savekit file");
        }
    }

    //Version
    std::uint16_t version = r.read_u16();
    if (version == 0 || version > kFormatVersion) {
        throw FormatError("unsupported format version");
    }

    //Read the size and the CRC the header
    std::uint32_t size = r.read_u32();
    std::uint32_t stored_crc = r.read_u32();

    //The size must match the bytes
    if (size != r.remaining()) {
        throw FormatError("payload size does not match the file size");
    }

    //Copy the payload out, then check its fingerprint
    std::vector<std::uint8_t> payload(size);
    for (std::uint32_t i = 0; i < size; i++) {
        payload[i] = r.read_u8();
    }

    if (crc32(payload) != stored_crc) {
        throw FormatError("checksum mismatch: the file is corrupted");
    }

    // 7. Everything checked out
    ParsedFile result;
    result.version = version;
    result.payload = payload;
    return result;
}

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

std::vector<std::uint8_t> read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("read_file: could not open the file");
    }

    in.seekg(0, std::ios::end);
    std::size_t n = static_cast<std::size_t>(in.tellg());
    in.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> bytes(n);
    for (std::size_t i =0; i<n; i++) {
        bytes[i] = static_cast<std::uint8_t>(in.get());
    }
    return bytes;
}

}
