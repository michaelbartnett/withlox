// -*- c++ -*-

#ifndef NUMERIC_TYPES_H

#include <cstdint>
#include <cassert>
#include <cstddef>

typedef float f32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Checked Casts

#if !defined(CHECKED_CASTS)
#define CHECKED_CASTS 1
#endif

#if CHECKED_CASTS

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"

#include <cassert>

#define DEF_CAST_FN2(src_type, dest_type, check1, check2)       \
    inline dest_type checked_cast_to_ ## dest_type(src_type n)  \
    {                                                           \
        assert(n check1);                                       \
        assert(n check2);                                       \
        return static_cast<dest_type>(n);                       \
    }

#define DEF_CAST_FN1(src_type, dest_type, check1)               \
    inline dest_type checked_cast_to_ ## dest_type(src_type n)  \
    {                                                           \
        assert(n check1);                                       \
        return static_cast<dest_type>(n);                       \
    }

#define DEF_S8_CAST(src_type) DEF_CAST_FN2(src_type, s8, >= INT8_MIN, <= INT8_MAX)
DEF_S8_CAST(s16)
DEF_S8_CAST(s32)
DEF_S8_CAST(s64)
#undef DEF_S8_CAST
#define DEF_S8_CAST(src_type) DEF_CAST_FN1(src_type, s8, <= INT8_MAX)
DEF_S8_CAST(u8)
DEF_S8_CAST(u16)
DEF_S8_CAST(u32)
DEF_S8_CAST(u64)
#undef DEF_S8_CAST

#define DEF_U8_CAST(src_type) DEF_CAST_FN2(src_type, u8, >= 0, <= UINT8_MAX)
DEF_U8_CAST(s8)
DEF_U8_CAST(s16)
DEF_U8_CAST(s32)
DEF_U8_CAST(s64)
DEF_U8_CAST(u16)
DEF_U8_CAST(u32)
DEF_U8_CAST(u64)
#undef DEF_U8_CAST

#define DEF_S16_CAST(src_type) DEF_CAST_FN2(src_type, s16, >= INT16_MIN, <= INT16_MAX)
DEF_S16_CAST(s32)
DEF_S16_CAST(s64)
#undef DEF_S16_CAST
#define DEF_S16_CAST(src_type) DEF_CAST_FN1(src_type, s16, <= INT16_MAX)
DEF_S16_CAST(u16)
DEF_S16_CAST(u32)
DEF_S16_CAST(u64)
#undef DEF_S16_CAST

#define DEF_U16_CAST(src_type) DEF_CAST_FN2(src_type, u16, >= 0, <= UINT16_MAX)
DEF_U16_CAST(s8)
DEF_U16_CAST(s16)
DEF_U16_CAST(s32)
DEF_U16_CAST(s64)
DEF_U16_CAST(u32)
DEF_U16_CAST(u64)
#undef DEF_U16_CAST

#define DEF_S32_CAST(src_type) DEF_CAST_FN2(src_type, s32, >= INT32_MIN, <= INT32_MAX)
DEF_S32_CAST(s64)
DEF_S32_CAST(intptr_t)
#undef DEF_S32_CAST
// DEF_CAST_FN2(s64, s32, >= INT32_MIN, <= INT32_MAX)
#define DEF_S32_CAST(src_type) DEF_CAST_FN1(src_type, s32, <= INT32_MAX)
DEF_S32_CAST(u32)
DEF_S32_CAST(u64)
DEF_S32_CAST(size_t)

#define DEF_U32_CAST(src_type) DEF_CAST_FN1(src_type, u32, >= 0)
DEF_U32_CAST(s8)
DEF_U32_CAST(s16)
DEF_U32_CAST(s32)
#undef DEF_U32_CAST
#define DEF_U32_CAST(src_type) DEF_CAST_FN2(src_type, u32, >= 0, <= UINT32_MAX)
DEF_U32_CAST(s64)
DEF_U32_CAST(u64)
#undef DEF_U32_CAST

// #define DEF_S64_CAST(src_type) DEF_CAST_FN2(src_type, s64, >= INT64_MIN, <= INT64_MAX)
#define DEF_S64_CAST(src_type) DEF_CAST_FN1(src_type, s64, <= INT64_MAX)
DEF_S64_CAST(u64)
DEF_S64_CAST(ptrdiff_t)

#define DEF_INTPTR_CAST(src_type) DEF_CAST_FN1(src_type, intptr_t, <= INTPTR_MAX)
DEF_INTPTR_CAST(u64)
DEF_INTPTR_CAST(size_t)

#define DEF_U64_CAST(src_type) DEF_CAST_FN1(src_type, u64, >= 0)
DEF_U64_CAST(s8)
DEF_U64_CAST(s16)
DEF_U64_CAST(s32)
DEF_U64_CAST(s64)
#undef DEF_U64_CAST

#define DEF_SIZE_T_CAST(src_type) DEF_CAST_FN1(src_type, size_t, >= 0)
DEF_SIZE_T_CAST(s8)
DEF_SIZE_T_CAST(s16)
DEF_SIZE_T_CAST(s32)
DEF_SIZE_T_CAST(s64)
DEF_SIZE_T_CAST(intptr_t)
#undef DEF_SIZE_T_CAST

#undef DEF_CAST_FN

#define CHECKED_CAST_MACRO(type, n) checked_cast_to_ ## type(n)

#pragma clang diagnostic pop

// if CHECKED_CASTS
#else

#define DEF_CAST_MACRO(type, n) static_cast<type>(n)

#endif

#define S8(n) CHECKED_CAST_MACRO(s8, n)
#define S16(n) CHECKED_CAST_MACRO(s16, n)
#define S32(n) CHECKED_CAST_MACRO(s32, n)
#define S64(n) CHECKED_CAST_MACRO(s64, n)

#define U8(n) CHECKED_CAST_MACRO(u8, n)
#define U16(n) CHECKED_CAST_MACRO(u16, n)
#define U32(n) CHECKED_CAST_MACRO(u32, n)
#define U64(n) CHECKED_CAST_MACRO(u64, n)

#define INTPTR_T(n) CHECKED_CAST_MACRO(intptr_t, n)
#define PTRDIFF_T(n) CHECKED_CAST_MACRO(intptr_t, n)
#define SIZE_T(n) CHECKED_CAST_MACRO(size_t, n)

// END Checked Casts



template< class T, class U >
T narrow_cast( U u ) //NOEXCEPT
{
    return static_cast<T>( u );
}


// jacked from https://github.com/martinmoene/gsl-lite/blob/master/include/gsl/gsl-lite.h
// and tweaked
template< class T, class U >
T narrow( U u ) //NOEXCEPT
{
    T t = narrow_cast<T>( u );

    assert( static_cast<U>( t ) != u );
    // if ( static_cast<U>( t ) != u )
    // {
    //     throw narrowing_error();
    // }

    // No gsl type traits
    // Don't assume T() works:
    assert( ( t < 0 ) != ( u < 0 ) );
    // if ( ( t < 0 ) != ( u < 0 ) )
    // {
    //     throw narrowing_error();
    // }
    return t;
}



#define NUMERIC_TYPES_H
#endif
