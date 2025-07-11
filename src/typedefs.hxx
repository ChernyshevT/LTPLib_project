#pragma once
#include <cfloat>
#include <cstdint>
#include <cstddef>
#include <complex>

typedef uint8_t     u8;  static_assert(sizeof(u8)  == 1);
typedef uint16_t    u16; static_assert(sizeof(u16) == 2);
typedef uint32_t    u32; static_assert(sizeof(u32) == 4);
typedef uint64_t    u64; static_assert(sizeof(u64) == 8);

typedef int8_t      i8;  static_assert(sizeof(i8)  == 1);
typedef int16_t     i16; static_assert(sizeof(i16) == 2);
typedef int32_t     i32; static_assert(sizeof(i32) == 4);
typedef int64_t     i64; static_assert(sizeof(i64) == 8);

typedef float       f32; static_assert(sizeof(f32) == 4);
typedef double      f64; static_assert(sizeof(f64) == 8);
