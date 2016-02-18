// -*- c++ -*-

#ifndef STR_H

#include "numeric_types.h"
#include "common.h"

#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <cstring>

// For referencing string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct StrSlice
{
    const char *data;
    u16 length;
};


// For owning string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct Str
{
    char *data;
    u16 length;
    u16 capacity;
};


Str str_alloc(u16 str_size)
{
    assert(str_size < UINT16_MAX);
    Str result;
    result.data = MALLOC_ARRAY(char, str_size + 1);
    result.data[0] = 0;
    result.length = 0;
    result.capacity = (u16)str_size;
    return result;
}


Str str(const char *cstr, u16 len)
{
    Str result = str_alloc(len);
    result.length = result.capacity;

    // careful: https://randomascii.wordpress.com/2013/04/03/stop-using-strncpy-already/
    std::strncpy(result.data, cstr, len + 1);
    result.data[result.length] = 0;

    return result;
}

Str str(const char *cstr)
{
    size_t len = std::strlen(cstr);
    assert(len < UINT16_MAX);
    return str(cstr, (u16)len);
}

Str str(StrSlice strslice)
{
    return str(strslice.data, strslice.length);
}

Str str(const Str &src)
{
    return str(src.data, src.length);
}

void str_free(Str *str)
{
    assert(str->data);
    std::free(str->data);
    str->capacity = 0;
    str->length = 0;
}


inline StrSlice str_slice(const char* cstr)
{
    StrSlice result;
    result.data = cstr;
    size_t len = strlen(cstr);
    assert(len < UINT16_MAX);
    result.length = (u16)len;
    return result;
}


inline StrSlice str_slice(const Str &str)
{
    StrSlice result;
    result.data = str.data;
    result.length = str.length;
    return result;
}


union SliceOrZero
{
    StrSlice slice;
    int zero;
};


Str str_concated_v_impl(StrSlice first_slice...)
{
    u16 len = first_slice.length;
    va_list args;
    int count = 0;
    va_start(args, first_slice);
    for (;;)
    {
        SliceOrZero arg = va_arg(args, SliceOrZero);
        if (!arg.zero)
        {
            break;
        }

        StrSlice slice = arg.slice;

        len += slice.length;
        ++count;
    }
    va_end(args);

    Str result = str_alloc(len);
    result.length = len;
    char *location = result.data;
    std::strncpy(location, first_slice.data, first_slice.length);
    location += first_slice.length;
    va_start(args, first_slice);
    for (int i = 0; i < count; ++i)
    {
        SliceOrZero arg = va_arg(args, SliceOrZero);
        StrSlice slice = arg.slice;
        std::strncpy(location, slice.data, slice.length);

        location += slice.length;
    }
    va_end(args);

    result.data[result.length] = 0;

    return result;
}

#define str_concated(...) str_concated_v_impl(__VA_ARGS__, 0);

// Str str_concated(StrSlice a, StrSlice b)
// {
//     Str result = str_alloc(a.length + b.length);
//     std::strncpy(result.data, a.data, a.length);
//     std::strncpy(result.data + a.length, b.data, b.length);
//     return result;
// }


inline bool str_equal(const StrSlice &a, const StrSlice &b)
{
    if (a.length != b.length)
    {
        return false;
    }

    const char *ca = a.data;
    const char *cb = b.data;
    for (int i = 0, len = a.length; i < len; ++i)
    {
        if (ca[i] != cb[i])
        {
            return false;
        }
    }

    return true;
}


inline bool str_equal(const char *const &a, const char *const &b)
{
    return str_equal(str_slice(a), str_slice(b));
}


inline bool str_equal(const Str &a, const StrSlice &b)
{
    return str_equal(str_slice(a), b);
}

inline bool str_equal(const StrSlice &a, const Str &b)
{
    return str_equal(a, str_slice(b));
}

inline bool str_equal(const Str &a, const Str &b)
{
    return str_equal(str_slice(a), str_slice(b));
}

inline bool str_equal(const StrSlice &a, const char *b)
{
    return str_equal(a, str_slice(b));
}

inline bool str_equal(const char *a, const StrSlice &b)
{
    return str_equal(b, str_slice(a));
}


#define STR_H
#endif
