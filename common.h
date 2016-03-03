#ifndef COMMON_H

#include <cstdlib>
#include <cstdio>

using std::printf;
using std::size_t;

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

#define REALLOC(ptr, type) ((type *)std::realloc(ptr, sizeof (type)))
#define REALLOC_ARRAY(ptr, type, count) ((type *)std::realloc(ptr, count * sizeof (type)))

#define ZERO_ARRAY(ptr, type, count) std::memset(ptr, 0, sizeof (type) * count)
#define ZERO_PTR(ptr) std::memset(ptr, 0, sizeof(*ptr))

#define COMMON_H
#endif
