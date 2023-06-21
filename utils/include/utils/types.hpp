#ifndef UTILS_TYPES_HPP
#define UTILS_TYPES_HPP

#include <cstdint>

namespace utils::types {
    
using u8 =  std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 =  std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

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



} /* namespace utils::types */

#endif /* UTILS_TYPES_HPP */