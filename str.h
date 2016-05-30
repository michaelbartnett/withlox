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

template<typename TInteger>
StrLen STRLEN(TInteger i)
{
    assert(i < STR_LENGTH_MAX);
    return (StrLen)i;
}

// For referencing string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct StrSlice
{
    const char *data;
    StrLen length; // length does not include null terminator
};


// For owning string memory
// TODO(mike): 'length' should probably be 'size' because utf8
struct Str
{
    char *data;
    StrLen length;   // length does not include null terminator
    StrLen capacity; // capacity includes null terminator (length + 1 if at min capacity)
};


Str str_alloc(StrLen str_size);
Str str(const char *cstr, StrLen len);
Str str(const char *cstr);
void str_free(Str *str);
Str str_concated_v_impl(StrSlice first_slice...);
Str str_make_copy(const Str &str);
void str_copy(Str *dest, size_t dest_start, StrSlice src, size_t src_start, size_t count);
void str_copy_truncate(Str *dest, size_t dest_start, StrSlice src, size_t src_start, size_t count);
void str_overwrite(Str *dest, StrSlice src);
void str_ensure_capacity(Str *str, StrLen capacity);

StrSlice empty_str_slice();

inline Str str(StrSlice strslice)
{
    return str(strslice.data, strslice.length);
}


inline Str str(const Str &src)
{
    return str(src.data, src.length);
}


inline Str str_make_empty()
{
    return str("", 0);
}


inline Str str_make_zeroed()
{
    Str result = {};
    return result;
}


inline StrSlice str_slice(const char *cstr, size_t length)
{
    StrSlice result;
    result.length = STRLEN(length);
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


inline void str_append(Str *str, StrSlice strslice)
{
    str_copy(str, str->length, strslice, 0, strslice.length);
}


inline void str_append(Str *str, const Str appendstr)
{
    str_append(str, str_slice(appendstr));
}


inline void str_append(Str *str, char c)
{
    char tmp[2];
    tmp[0] = c;
    tmp[1] = '\0';
    str_append(str, str_slice(tmp, 1));
}


inline void str_clear(Str *str)
{
    if (str->data)
    {
        str->length = 0;
        str->data[0] = '\0';
    }
}


inline char str_popchar(Str *str)
{
    if (!str->data || str->length == 0)
    {
        return '\0';
    }

    --str->length;
    char result = str->data[str->length];
    str->data[str->length] = '\0';
    return result;
}


#define str_concated(...) str_concated_v_impl(__VA_ARGS__, 0);

bool str_equal(const StrSlice &a, const StrSlice &b);
bool str_equal_ignorecase(const StrSlice &a, const StrSlice &b);

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

inline bool str_equal_ignorecase(const char *const &a, const char *const &b)
{
    return str_equal_ignorecase(str_slice(a), str_slice(b));
}
inline bool str_equal_ignorecase(const Str &a, const StrSlice &b)
{
    return str_equal_ignorecase(str_slice(a), b);
}
inline bool str_equal_ignorecase(const StrSlice &a, const Str &b)
{
    return str_equal_ignorecase(a, str_slice(b));
}
inline bool str_equal_ignorecase(const Str &a, const Str &b)
{
    return str_equal_ignorecase(str_slice(a), str_slice(b));
}
inline bool str_equal_ignorecase(const StrSlice &a, const char *b)
{
    return str_equal_ignorecase(a, str_slice(b));
}
inline bool str_equal_ignorecase(const char *a, const StrSlice &b)
{
    return str_equal_ignorecase(b, str_slice(a));
}


bool str_endswith(StrSlice str, StrSlice suffix);
bool str_endswith_ignorecase(StrSlice str, StrSlice suffix);

inline bool str_endswith(Str str, Str suffix)
{
    return str_endswith(str_slice(str), str_slice(suffix));
}
inline bool str_endswith(Str str, StrSlice suffix)
{
    return str_endswith(str_slice(str), suffix);
}
inline bool str_endswith(Str str, const char *suffix)
{
    return str_endswith(str_slice(str), str_slice(suffix));
}
inline bool str_endswith(StrSlice str, Str suffix)
{
    return str_endswith(str, str_slice(suffix));
}
inline bool str_endswith(StrSlice str, const char *suffix)
{
    return str_endswith(str, str_slice(suffix));
}
inline bool str_endswith(const char *str, Str suffix)
{
    return str_endswith(str_slice(str), str_slice(suffix));
}
inline bool str_endswith(const char *str, StrSlice suffix)
{
    return str_endswith(str_slice(str), suffix);
}
inline bool str_endswith(const char *str, const char *suffix)
{
    return str_endswith(str_slice(str), str_slice(suffix));
}

inline bool str_endswith_ignorecase(Str str, Str suffix)
{
    return str_endswith_ignorecase(str_slice(str), str_slice(suffix));
}
inline bool str_endswith_ignorecase(Str str, StrSlice suffix)
{
    return str_endswith_ignorecase(str_slice(str), suffix);
}
inline bool str_endswith_ignorecase(Str str, const char *suffix)
{
    return str_endswith_ignorecase(str_slice(str), str_slice(suffix));
}
inline bool str_endswith_ignorecase(StrSlice str, Str suffix)
{
    return str_endswith_ignorecase(str, str_slice(suffix));
}
inline bool str_endswith_ignorecase(StrSlice str, const char *suffix)
{
    return str_endswith_ignorecase(str, str_slice(suffix));
}
inline bool str_endswith_ignorecase(const char *str, Str suffix)
{
    return str_endswith_ignorecase(str_slice(str), str_slice(suffix));
}
inline bool str_endswith_ignorecase(const char *str, StrSlice suffix)
{
    return str_endswith_ignorecase(str_slice(str), suffix);
}
inline bool str_endswith_ignorecase(const char *str, const char *suffix)
{
    return str_endswith_ignorecase(str_slice(str), str_slice(suffix));
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
