#ifndef COMMON_H

#define CALLOC(type) ((type *)std::calloc(1, sizeof (type)))
#define CALLOC_ARRAY(type, count) ((type *)std::calloc(count, sizeof (type)))

#define MALLOC(type) ((type *)std::malloc(sizeof (type)))
#define MALLOC_ARRAY(type, count) ((type *)std::malloc(count * sizeof (type)))

#define REALLOC(ptr, type) ((type *)std::realloc(ptr, sizeof (type)))
#define REALLOC_ARRAY(ptr, type, count) ((type *)std::realloc(ptr, count * sizeof (type)))

#define ZERO_ARRAY(ptr, type, count) std::memset(ptr, 0, sizeof (type) * count)

#define COMMON_H
#endif
