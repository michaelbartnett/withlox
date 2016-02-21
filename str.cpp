#include "str.h"


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
    assert(str->data);
    std::free(str->data);
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
