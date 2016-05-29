#include "str.h"
#include "memory.h"
#include "common.h"

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
    result.data = MAKE_ARRAY(char, str_size + 1, mem::default_allocator());
    result.data[0] = 0;
    result.length = 0;
    result.capacity = (StrLen)str_size;
    return result;
}


void str_ensure_capacity(Str *str, StrLen capacity)
{
    if (str->capacity < capacity)
    {
        str->capacity = capacity;
        RESIZE_ARRAY(str->data, char, str->capacity, mem::default_allocator());
    }
}


Str str_make_copy(const Str &str)
{
    assert(str.capacity >= str.length + 1);
    Str result = str_alloc(str.capacity);
    std::strncpy(result.data, str.data, str.length + 1);
    result.data[result.length] = 0;
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
    return str(cstr, STRLEN(len));
}


void str_free(Str *str)
{
    assert(str->capacity);
    mem::default_allocator()->dealloc(str->data);
    str->data = empty_string;
    str->capacity = 0;
    str->length = 0;
}


// TODO(mike): unit test
void str_copy(Str *dest, size_t dest_start, StrSlice src, size_t src_start, size_t count)
{
    StrLen sl_src_start = STRLEN(src_start);
    StrLen sl_count = STRLEN(count);
    assert(sl_src_start + sl_count <= src.length);

    StrLen sl_dest_start = STRLEN(dest_start);
    StrLen final_length = max<StrLen>(sl_dest_start + sl_count, dest->length);
    assert(final_length > sl_dest_start || sl_count == 0);


    StrLen capacity_required = sl_dest_start + sl_count + 1;
    str_ensure_capacity(dest, capacity_required);
    std::strncpy(dest->data + sl_dest_start, src.data + sl_src_start, sl_count);


    if (final_length > dest->length)
    {
        dest->data[final_length] = '\0';
    }

    dest->length = final_length;
}


void str_copy_truncate(Str *dest, size_t dest_start, StrSlice src, size_t src_start, size_t count)
{
    str_copy(dest, dest_start, src, src_start, count);
    *(dest->data + dest_start + count) = '\0';
}


void str_overwrite(Str *dest, StrSlice src)
{
    str_copy(dest, 0, src, 0, src.length);
    dest->data[src.length] = '\0';
    dest->length = src.length;
}


union SliceOrZero
{
    StrSlice slice;
    int zero;
};


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
