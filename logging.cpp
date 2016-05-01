// logging
#include "logging.h"
#include "dynarray.h"
#include "str.h"
#include "memory.h"
#include "common.h"
#include "formatbuffer.h"
#include <cstdarg>
#include <cstdio>
#include <cstddef>

static DynArray<Str> log_entries = {};
static char *output_buffer = 0;
static size_t output_buffer_size = 0;
static FormatBuffer *concatenated = 0;
static u32 next_log_entry_to_concat = 0;
static bool concatenated_dirty = false;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif


void append_log(Str log_entry)
{
    // for (int i = log_entry.length - 1; i >= 0; --i)
    // {
    //     if (log_entry.data[i] == '\n')
    //     {
    //         log_entry.data[i] = '\0';
    //         --log_entry.length;
    //     }
    //     else
    //     {
    //         break;
    //     }
    // }
    dynarray_append(&log_entries, log_entry);
    concatenated_dirty = true;
}


void append_log(const char *string)
{
    append_log(str(string));
}


void append_log(const char *string, size_t length)
{
    append_log(str(string, STRLEN(length)));
}


static void vlogf(const char *format, va_list vargs)
{
    mem::IAllocator *allocator = mem::default_allocator();
    // lazily allocate the output buffer
    if (!output_buffer)
    {
        output_buffer_size = FormatBuffer::DefaultCapacity;
        // output_buffer = MALLOC_ARRAY(char, output_buffer_size);
        output_buffer = MAKE_ARRAY(char, output_buffer_size, allocator);
    }

    int format_size = vsnprintf(output_buffer, output_buffer_size, format, vargs);
    assert(format_size >= 0);

    // format size should always be less than output size, otherwise we risk losing the null terminator
    if ((size_t)format_size >= output_buffer_size)
    {
        output_buffer_size = (size_t)format_size + 1;
        // REALLOC_ARRAY(output_buffer, char, output_buffer_size);
        RESIZE_ARRAY(output_buffer, char, output_buffer_size, allocator);
        int confirm_format_size = vsnprintf(output_buffer, output_buffer_size, format, vargs);
        assert(confirm_format_size == format_size);
    }

    // ^ could extract that into a dynamic format buffer class

    if (!log_entries.data)
    {
        dynarray_init(&log_entries, 1024);
    }

    assert(output_buffer_size < STR_LENGTH_MAX);

    append_log(output_buffer, (size_t)format_size);

    println(output_buffer);
}


void logf(const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    vlogf(format, vargs);
    va_end(vargs);
}


void logln(const char *string)
{
    logf("%s\n", string);
}


void logf_ln(const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    vlogf(format, vargs);
    va_end(vargs);
    logln("");
}


u32 log_count()
{
    return log_entries.count;
}

Str *get_log(u32 i)
{
    return &log_entries[i];
}


void log_write_with_userdata(void *userdata, const char *buffer, size_t length)
{
    UNUSED(userdata);
    append_log(str(buffer, STRLEN(length)));
}

void logf_with_userdata(void *userdata, const char *format, ...)
{
    UNUSED(userdata);
    va_list vargs;
    va_start(vargs, format);
    vlogf(format, vargs);
    va_end(vargs);
}


Str concatenated_log()
{
    if (!concatenated)
    {
        concatenated_dirty = true;
        concatenated = new FormatBuffer(128); // ugh, new
    }

    if (concatenated_dirty)
    {
        for (u32 i = next_log_entry_to_concat; i < log_entries.count; ++i)
        {
            Str *str = &log_entries[i];
            concatenated->write(str->data, str->length);
            concatenated->write("\n", 1);
        }
        concatenated_dirty = false;
        next_log_entry_to_concat = log_entries.count;
    }

    Str result;
    result.length = STRLEN(concatenated->cursor);
    result.data = concatenated->buffer;
    result.capacity = result.capacity;
    return result;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
