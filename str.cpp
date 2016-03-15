#include "str.h"

static char empty_string_storage[] = { '\0' };
static char *empty_string = empty_string_storage;


StrSlice empty_str_slice()
{
    StrSlice result;
    result.data = empty_string;
    result.length = 0;
    return result;
}


Str str_alloc(StrLen str_size)
{
    assert(str_size < UINT16_MAX);
    Str result;
    result.data = MALLOC_ARRAY(char, str_size + 1);
    result.data[0] = 0;
    result.length = 0;
    result.capacity = (StrLen)str_size;
    return result;
}


Str str(const char *cstr, StrLen len)
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
    return str(cstr, (StrLen)len);
}


void str_free(Str *str)
{
    assert(str->capacity);
    std::free(str->data);
    str->data = empty_string;
    str->capacity = 0;
    str->length = 0;
}


Str str_concated_v_impl(StrSlice first_slice...)
{
    StrLen len = first_slice.length;
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

static bool is_ascii_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}


static char to_ascii_lower(char c)
{
    if (is_ascii_upper(c))
    {
        c += ('a' - 'A');
    }

    return c;
}


bool str_equal_ignore_case(const StrSlice &a, const StrSlice &b)
{    
    if (a.length != b.length)
    {
        return false;
    }

    const char *ca = a.data;
    const char *cb = b.data;
    for (int i = 0, len = a.length; i < len; ++i)
    {
        char achar = to_ascii_lower(ca[i]);
        char bchar = to_ascii_lower(cb[i]);

        if (achar != bchar)
        {
            return false;
        }
    }

    return true;
}


bool str_equal(const StrSlice &a, const StrSlice &b)
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
