// -*- c++ -*-
#ifndef COMMON_H

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define SANITY_CHECK 1

// this doesn't do anything, just an annotation for the reader's benefit
#define OUTPARAM

#define XSTRFY(s) #s
#define STRFY(s) XSTRFY(s)

#define S__LINE__ STRFY(__LINE__)

#define STATIC_ASSERT(label, expr) typedef int static_assertion__##label[(expr) ? 1 : -1]

#define COUNTOF(arr) ( \
   0 * sizeof(reinterpret_cast<const ::Bad_arg_to_COUNTOF*>(arr)) + \
   0 * sizeof(::Bad_arg_to_COUNTOF::check_type((arr), &(arr))) + \
   sizeof(arr) / sizeof((arr)[0]) )

struct Bad_arg_to_COUNTOF {
   class Is_pointer; // incomplete
   class Is_array {};
   template <typename T>
   static Is_pointer check_type(const T*, const T* const*);
   static Is_array check_type(const void*, const void*);
};


template <typename T>
inline const T &max(const T &a, const T &b)
{
    return (a >= b) ? a : b;
}

template <typename T>
inline const T &min(const T &a, const T &b)
{
    return (a <= b) ? a : b;
}

using std::printf;
using std::size_t;

#define UNUSED(x) (void)(x)

#define KILOBYTES(n) (n*1024)
#define MEGABYTES(n) (KILOBYTES(n)*1024)
#define GIGABYTES(n)

#define ARRAY_DIM(array) (sizeof(array) / sizeof(array[0]))

#define print(str) printf("%s", str)
#define printf_indent(indent, fmt, ...) printf("%*s" fmt, (indent), "", __VA_ARGS__)
#define printf_ln(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#define printf_ln_indent(indent, fmt, ...) printf("%*s" fmt "\n", indent, "", __VA_ARGS__)
#define println(text) printf("%s\n", text)
#define println_indent(indent, text) printf_ln_indent(indent, "%s", text)


// TODO(mike): Move into a platform header
#if __cplusplus < 201103L
    #define OVERRIDE
    #if defined(__clang__) || defined(__GNUC__)
        // #define THREAD_LOCAL __threadd
    #elif defined(_MSC_VER)
        // #define THREAD_LOCAL __declspec(thread)
    #else
        #error Need definitions for compiler
    #endif
#else
    #define OVERRIDE override
    // #define THREAD_LOCAL thread_local
#endif

// Clang/Xcode defines nullptr even with -std=C++03, so
// you can't rely on a __cplusplus check by itself,
// but you do need to still check for __cplusplus
// because linux is different because who knows.
#if !defined(nullptr) && (!defined(__cplusplus) || (__cplusplus < 201103L))
    #define nullptr NULL
#endif

#if !defined(va_copy)
    #define va_copy(d,s) __builtin_va_copy(d,s)
#endif

#define COMMON_H
#endif
