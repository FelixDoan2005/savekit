#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "savekit/binary_reader.hpp"
#include "savekit/binary_writer.hpp"

namespace savekit {

// write_value and read_value replace write_u8 / write_u32 / ... and read_u8 / read_u32 / ...
// one name for every type, the compiler picks the right code from the type you pass in
//     write_value(w, std::uint32_t{1000});            // same bytes as w.write_u32(1000)
//     std::string s = read_value<std::string>(r);
//
// supported: u8, u16, u32, u64, float, double, std::string, and std::vector<T> of any of those
// a vector is a count (4 bytes) then each element written with write_value

// is T a std::vector<something>? (already written, its boilerplate)
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
void write_value([[maybe_unused]] BinaryWriter& w, [[maybe_unused]] const T& value) {
    if constexpr (std::is_same_v<T, std::uint8_t>) {
        // TODO: w.write_u8(value);
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, float>) {
        // TODO
    } else if constexpr (std::is_same_v<T, double>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::string>) {
        // TODO
    } else if constexpr (is_vector_v<T>) {
        // TODO: 1. if value.size() doesn't fit in a std::uint32_t, throw std::length_error
        //       2. w.write_u32(the count)
        //       3. for (const auto& item : value) write_value(w, item);   <- calls itself!
    } else {
        static_assert(always_false_v<T>, "savekit: write_value does not support this type");
    }
}

template <typename T>
T read_value([[maybe_unused]] BinaryReader& r) {
    if constexpr (std::is_same_v<T, std::uint8_t>) {
        // TODO: return r.read_u8();
    } else if constexpr (std::is_same_v<T, std::uint16_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::uint32_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::uint64_t>) {
        // TODO
    } else if constexpr (std::is_same_v<T, float>) {
        // TODO
    } else if constexpr (std::is_same_v<T, double>) {
        // TODO
    } else if constexpr (std::is_same_v<T, std::string>) {
        // TODO
    } else if constexpr (is_vector_v<T>) {
        // TODO: using Item = typename T::value_type;
        //       1. remember r.position()
        //       2. count = r.read_u32()
        //       3. every element takes at least 1 byte, so if count > r.remaining() the count is a lie:
        //          r.set_position(start) and throw std::out_of_range. Do this BEFORE reserve() or a loop.
        //       4. T result; result.reserve(count); then count times: result.push_back(read_value<Item>(r))
        //          If an element read throws, put the position back (try / catch (...) { set_position; throw; }).
        //       5. return result;
    } else {
        static_assert(always_false_v<T>, "savekit: read_value does not support this type");
    }

    return T{};   // TODO: delete this line once every branch above returns its own value
}

}
