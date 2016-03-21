// -*- c++ -*-

#ifndef NUMERIC_TYPES_H

#include <cstdint>
#include <cassert>

typedef float f32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

inline u16 U16(int32_t i)
{
    assert(i <= UINT16_MAX);
    return (u16)i;
}

inline u32 U32(int64_t i)
{
    assert(i <= UINT32_MAX);
    return (u32)i;
}

inline s32 S32(uint32_t u)
{
    assert(u <= INT32_MAX);
    return (s32)u;
}

#define NUMERIC_TYPES_H
#endif
