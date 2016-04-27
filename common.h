// -*- c++ -*-
#ifndef COMMON_H

#define XSTRFY(s) #s
#define STRFY(s) XSTRFY(s)

#define S__LINE__ STRFY(__LINE__)

#define HEADERFN inline

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

#include <cstdlib>
#include <cstdio>

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

#define IGNORE(x)

#define CALLOC(type) ((type *)std::calloc(1, sizeof (type)))
#define CALLOC_ARRAY(type, count) ((type *)std::calloc(count, sizeof (type)))

#define MALLOC(type) ((type *)std::malloc(sizeof (type)))
#define MALLOC_ARRAY(type, count) ((type *)std::malloc(count * sizeof (type)))

#define REALLOC(ptr, type) ((type *)std::realloc((ptr), sizeof (type)))
#define REALLOC_ARRAY(ptr, type, count) ptr = ((type *)std::realloc((ptr), (count) * sizeof (type)))

#define ZERO_ARRAY(ptr, type, count) std::memset((ptr), 0, sizeof (type) * (count))
#define ZERO_PTR(ptr) std::memset((ptr), 0, sizeof(*(ptr)))

#if __cplusplus < 201103L
#define OVERRIDE
#else
#define OVERRIDE override
#endif

// Clang/Xcode defines nullptr even with -std=C++03, so
// you can't rely on a __cplusplus check by itself,
// but you do need to still check for __cplusplus
// because linux is stupid.
#if !defined(nullptr) && (!defined(__cplusplus) || (__cplusplus < 201103L))
#define nullptr NULL
#endif

#if !defined(va_copy)
#define va_copy(d,s) __builtin_va_copy(d,s)
#endif

#define COMMON_H
#endif
