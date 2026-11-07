#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
#include <bit>
#include <limits>
#include <stdexcept>


#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

namespace savekit {

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

// makes the static_assert fail only if the "unsupported type" branch is actually used
template <typename>
inline constexpr bool always_false_v = false;

template <typename T>
void write_value(BinaryWriter& w,  const T& value) {
    if constexpr (std::is_same_v<T, std::uint8_t>) {
        w.write_u8(value);       // T is a u8, so call the u8 writer
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        w.write_u16(value);
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        w.write_u32(value);
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        w.write_u64(value);
    } else if constexpr (std::is_same_v<T, float>) {
        w.write_u32(std::bit_cast<std::uint32_t>(value));
    } else if constexpr (std::is_same_v<T, double>) {
        w.write_u64(std::bit_cast<std::uint64_t>(value));
    } else if constexpr (std::is_same_v<T, std::string>) {
        w.write_string(value);
    } else if constexpr (is_vector_v<T>) {
        const std::uint32_t max_len = std::numeric_limits<std::uint32_t>::max();
        if (value.size() > max_len) {   // count has to fit in 4 bytes
            throw std::length_error("BinaryWriter: vector too long");
        }
        w.write_u32(static_cast<std::uint32_t>(value.size()));

        for (std::size_t i = 0; i < value.size(); i++) {
            write_value(w, value[i]);  
        }
    } else {
        static_assert(always_false_v<T>, "savekit: write_value does not support this type");
    }
}

template <typename T>
T read_value(BinaryReader& r) {
    if constexpr (std::is_same_v<T, std::uint8_t>) {
        return r.read_u8();
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        return r.read_u16();
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        return r.read_u32();
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        return r.read_u64();
    } else if constexpr (std::is_same_v<T, float>) {
        return r.read_f32();
    } else if constexpr (std::is_same_v<T, double>) {
        return r.read_f64();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return r.read_string();
    } else if constexpr (is_vector_v<T>) {
        std::size_t prev_pos_ = r.position();       // remember where we started
        std::uint32_t length = r.read_u32();
        if (length > r.remaining()) {   // every item takes at least 1 byte. if the count is a lie go back and throw
            r.set_position(prev_pos_);
            throw std::out_of_range("BinaryReader: vector longer than data");
        }
        T result;
        using Item = typename T::value_type;
        // typename tells compiler its a type
        // T::value_type means look inside T and get the type e.g read_value<std::vector<float>>(r)
        //                                                        T is std::vector<float>
        //                                                        T::value_type looks inside and gets float
        //                                                        so put it T::value_type into the <> for read_value call
        result.reserve(length);
        try {
            for (std::size_t i=0; i<length; i++) {
                result.push_back(read_value<Item>(r));
            }
        } catch (...) {
            r.set_position(prev_pos_);   // an item failed halfway, go back to the start
            throw;
        }

        return result;

    } else {
        static_assert(always_false_v<T>, "savekit: read_value does not support this type");
    }
}

}
