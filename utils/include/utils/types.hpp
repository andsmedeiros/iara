#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <cstdint>

namespace utils::types {
    
using u8 =  std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using u8_least = std::uint_least8_t;
using u16_least = std::uint_least16_t;
using u32_least = std::uint_least32_t;
using u64_least = std::uint_least64_t;

using u8_fast = std::uint_fast8_t;
using u16_fast = std::uint_fast16_t;
using u32_fast = std::uint_fast32_t;
using u64_fast = std::uint_fast64_t;

using u_max = std::uintmax_t;
using u_ptr = std::uintptr_t;

using i8 =  std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using i8_least = std::int_least8_t;
using i16_least = std::int_least16_t;
using i32_least = std::int_least32_t;
using i64_least = std::int_least64_t;

using i8_fast = std::int_fast8_t;
using i16_fast = std::int_fast16_t;
using i32_fast = std::int_fast32_t;
using i64_fast = std::int_fast64_t;

using i_max = std::intmax_t;
using i_ptr = std::intptr_t;

constexpr static inline u8 operator "" _u8(unsigned long long value) {
    return static_cast<u8>(value); 
}
constexpr static inline u16 operator "" _u16(unsigned long long value) {
    return static_cast<u16>(value); 
}
constexpr static inline u32 operator "" _u32(unsigned long long value) {
    return static_cast<u32>(value); 
}
constexpr static inline u64 operator "" _u64(unsigned long long value) {
    return static_cast<u64>(value); 
}

constexpr static inline u8_least operator "" _u8_least(unsigned long long value) {
    return static_cast<u8_least>(value);
}
constexpr static inline u16_least operator "" _u16_least(unsigned long long value) {
    return static_cast<u16_least>(value);
}
constexpr static inline u32_least operator "" _u32_least(unsigned long long value) {
    return static_cast<u32_least>(value);
}
constexpr static inline u64_least operator "" _u64_least(unsigned long long value) {
    return static_cast<u64_least>(value);
}

constexpr static inline u8_fast operator "" _u8_fast(unsigned long long value) {
    return static_cast<u8_fast>(value);
}
constexpr static inline u16_fast operator "" _u16_fast(unsigned long long value) {
    return static_cast<u16_fast>(value);
}
constexpr static inline u32_fast operator "" _u32_fast(unsigned long long value) {
    return static_cast<u32_fast>(value);
}
constexpr static inline u64_fast operator "" _u64_fast(unsigned long long value) {
    return static_cast<u64_fast>(value);
}

constexpr static inline u_max operator "" _u_max(unsigned long long value) {
    return static_cast<u_max>(value);
}
constexpr static inline u_ptr operator "" _u_ptr(unsigned long long value) {
    return static_cast<u_ptr>(value);
}

constexpr static inline i8 operator "" _i8(unsigned long long value) {
    return static_cast<i8>(value); 
}
constexpr static inline i16 operator "" _i16(unsigned long long value) {
    return static_cast<i16>(value); 
}
constexpr static inline i32 operator "" _i32(unsigned long long value) {
    return static_cast<i32>(value); 
}
constexpr static inline i64 operator "" _i64(unsigned long long value) {
    return static_cast<i64>(value); 
}

constexpr static inline i8_least operator "" _i8_least(unsigned long long value) {
    return static_cast<i8_least>(value);
}
constexpr static inline i16_least operator "" _i16_least(unsigned long long value) {
    return static_cast<i16_least>(value);
}
constexpr static inline i32_least operator "" _i32_least(unsigned long long value) {
    return static_cast<i32_least>(value);
}
constexpr static inline i64_least operator "" _i64_least(unsigned long long value) {
    return static_cast<i64_least>(value);
}

constexpr static inline i8_fast operator "" _i8_fast(unsigned long long value) {
    return static_cast<i8_fast>(value);
}
constexpr static inline i16_fast operator "" _i16_fast(unsigned long long value) {
    return static_cast<i16_fast>(value);
}
constexpr static inline i32_fast operator "" _i32_fast(unsigned long long value) {
    return static_cast<i32_fast>(value);
}
constexpr static inline i64_fast operator "" _i64_fast(unsigned long long value) {
    return static_cast<i64_fast>(value);
}

constexpr static inline i_max operator "" _i_max(unsigned long long value) {
    return static_cast<i_max>(value);
}
constexpr static inline i_ptr operator "" _i_ptr(unsigned long long value) {
    return static_cast<i_ptr>(value);
}

} /* namespace utils::types */

#endif /* UTILS_TYPES_HPP */