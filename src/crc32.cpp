#include "savekit/crc32.hpp"
#include <array>

namespace savekit {

// look up table so 8 rounds arent recomputated every single time
static std::array<std::uint32_t, 256> make_table() {
    std::array<std::uint32_t, 256> table{};

    for (std::uint32_t i = 0; i < 256; i++) {
        std::uint32_t c = i;

        for (int j = 0; j < 8; j++) {
            if (c & 1) { //checks if first digit of c is 1
                c = 0xEDB88320 ^ (c >> 1);   // move everything right, then XOR with the magic number i.e. flip whever magic number has 1
            } else {
                c = c >> 1;
            }
        }

        table[i] = c;
    }

    return table;
}

std::uint32_t crc32(std::span<const std::uint8_t> data) noexcept {

    static const std::array<std::uint32_t, 256> table = make_table();

    std::uint32_t crc = 0xFFFFFFFF;  

    for (std::size_t i = 0; i < data.size(); i++) {
        std::uint32_t byte = data[i];
        std::uint32_t index = (crc ^ byte) & 0xFF;// mix the byte into the low 8 bits
        crc = table[index] ^ (crc >> 8);// updates fingerprint by combinding table result and old fingerprint
    }

    return crc ^ 0xFFFFFFFF;// flip all the bits at the end 
}

}
