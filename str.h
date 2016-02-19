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


Str str_alloc(u16 str_size);
Str str(const char *cstr, u16 len);
Str str(const char *cstr);
void str_free(Str *str);
void str_free(Str *str);
Str str_concated_v_impl(StrSlice first_slice...);


inline Str str(StrSlice strslice)
{
    return str(strslice.data, strslice.length);
}


inline Str str(const Str &src)
{
    return str(src.data, src.length);
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


#define str_concated(...) str_concated_v_impl(__VA_ARGS__, 0);


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






inline u32 str_hash(const char *const &str)
{
    u32 sum = 0;
    StrSlice slice = str_slice(str);
    for (int i = 0; i < slice.length; ++i) {
        sum += (u32)slice.data[i];
    }
    return sum;
}


#define STR_H
#endif
