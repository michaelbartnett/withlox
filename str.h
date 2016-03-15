// -*- c++ -*-

#ifndef STR_H

#include "numeric_types.h"
#include "common.h"
#include "MurmurHash3.h"

#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <cstring>

typedef u16 StrLen;
#define STR_LENGTH_MAX UINT16_MAX

// For referencing string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct StrSlice
{
    const char *data;
    StrLen length;
};


// For owning string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct Str
{
    char *data;
    StrLen length;
    StrLen capacity;
};


Str str_alloc(StrLen str_size);
Str str(const char *cstr, StrLen len);
Str str(const char *cstr);
void str_free(Str *str);
Str str_concated_v_impl(StrSlice first_slice...);

StrSlice empty_str_slice();

inline Str str(StrSlice strslice)
{
    return str(strslice.data, strslice.length);
}


inline Str str(const Str &src)
{
    return str(src.data, src.length);
}
    
inline StrSlice str_slice(const char *cstr, size_t length)
{
    StrSlice result;
    assert(length < STR_LENGTH_MAX);
    result.length = (StrLen)length;
    result.data = cstr;
    return result;    
}

inline StrSlice str_slice(const char *begin, const char *end)
{
    assert(end >= begin);
    return str_slice(begin, (size_t)(end - begin));
}

inline StrSlice str_slice(const char *cstr)
{
    size_t len = strlen(cstr);
    return str_slice(cstr, len);
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

bool str_equal(const StrSlice &a, const StrSlice &b);
bool str_equal_ignore_case(const StrSlice &a, const StrSlice &b);

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



inline bool str_equal_ignore_case(const char *const &a, const char *const &b)
{
    return str_equal_ignore_case(str_slice(a), str_slice(b));
}


inline bool str_equal_ignore_case(const Str &a, const StrSlice &b)
{
    return str_equal_ignore_case(str_slice(a), b);
}

inline bool str_equal_ignore_case(const StrSlice &a, const Str &b)
{
    return str_equal_ignore_case(a, str_slice(b));
}

inline bool str_equal_ignore_case(const Str &a, const Str &b)
{
    return str_equal_ignore_case(str_slice(a), str_slice(b));
}

inline bool str_equal_ignore_case(const StrSlice &a, const char *b)
{
    return str_equal_ignore_case(a, str_slice(b));
}

inline bool str_equal_ignore_case(const char *a, const StrSlice &b)
{
    return str_equal_ignore_case(b, str_slice(a));
}


struct StrSliceHash
{
    u32 operator()(const StrSlice &slice)
    {
        const u32 seed = 541;
        u32 result;
        MurmurHash3_x86_32(slice.data, slice.length, seed, &result);
        return result;
    }
};

struct StrSliceEqual
{
    bool operator()(const StrSlice &lhs, const StrSlice &rhs)
    {
        return str_equal(lhs, rhs);
    }
};




// inline u32 str_hash(const char *const &str)
// {
//     u32 sum = 0;
//     StrSlice slice = str_slice(str);
//     for (int i = 0; i < slice.length; ++i) {
//         sum += (u32)slice.data[i];
//     }
//     return sum;
// }


#define STR_H
#endif
