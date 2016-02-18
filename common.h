#ifndef COMMON_H

#define MALLOC(type) ((type *)std::malloc(sizeof (type)))
#define MALLOC_ARRAY(type, count) ((type *)std::malloc(count * sizeof (type)))
#define REALLOC(ptr, type) ((type *)std::realloc(ptr, sizeof (type)))
#define REALLOC_ARRAY(ptr, type, count) ((type *)std::realloc(ptr, count * sizeof (type)))

#define COMMON_H
#endif
