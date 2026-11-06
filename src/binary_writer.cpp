#include "savekit/binary_writer.hpp"

namespace savekit {

void BinaryWriter::write_u8(std::uint8_t value) {
    buffer_.push_back(value);
}

void BinaryWriter::write_u32(std::uint32_t value) {
    for (int i=0; i<4; i++) {
        write_u8(static_cast<std::uint8_t>((value>>(8*i)) & 0xFF));
    }
}

}
