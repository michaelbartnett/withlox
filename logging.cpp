// logging
#include "logging.h"
#include "dynarray.h"
#include "str.h"
#include "common.h"
#include <cstdarg>
#include <cstdio>
#include <cstddef>

static DynArray<Str> log_entries = {};
static char *output_buffer = 0;
static size_t output_buffer_size = 0;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif



static void vlogf(const char *format, va_list vargs)
{
    // lazily allocate the output buffer
    if (!output_buffer)
    {
        output_buffer_size = 1024;
        output_buffer = MALLOC_ARRAY(char, output_buffer_size);
    }

    int format_size = vsnprintf(output_buffer, output_buffer_size, format, vargs);
    assert(format_size >= 0);

    // format size should always be less than output size, otherwise we risk losing the null terminator
    if ((size_t)format_size >= output_buffer_size)
    {
        output_buffer_size = (size_t)format_size + 1;
        REALLOC_ARRAY(output_buffer, char, output_buffer_size);
        int confirm_format_size = vsnprintf(output_buffer, output_buffer_size, format, vargs);
        assert(confirm_format_size == format_size);
    }

    // ^ could extract that into a dynamic format buffer class

    if (!log_entries.data)
    {
        dynarray_init(&log_entries, 1024);
    }

    assert(output_buffer_size < STR_LENGTH_MAX);
    
    append(&log_entries, str(output_buffer, (StrLen)output_buffer_size));

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
    return get(log_entries, i);
}



void init_formatbuffer(FormatBuffer *fb, size_t initial_capacity)
{
    fb->capacity = initial_capacity;
    fb->buffer = MALLOC_ARRAY(char, fb->capacity);
    fb->cursor = fb->buffer;
}

FormatBuffer::FormatBuffer(size_t initial_capacity)
{
    init_formatbuffer(this, initial_capacity);
}

FormatBuffer::FormatBuffer()
{
    init_formatbuffer(this, 1024);
}

void formatbuffer_v_writef(FormatBuffer *fmt_buf, const char *format, va_list vargs)
{
    ptrdiff_t bytes_remaining = (fmt_buf->buffer + fmt_buf->capacity) - fmt_buf->cursor;
    assert(bytes_remaining >= 0);

    int format_size = vsnprintf(fmt_buf->cursor, (size_t)bytes_remaining, format, vargs);
    assert(format_size >= 0);
    int byte_write_count = format_size;

    // format size should always be less than output size, otherwise we risk losing the null terminator
    if (format_size >= bytes_remaining)
    {
        ptrdiff_t new_bytes_remaining = format_size - bytes_remaining + 1;
        assert(new_bytes_remaining > bytes_remaining);

        fmt_buf->capacity += (size_t)new_bytes_remaining;
        assert(fmt_buf->cursor + new_bytes_remaining == fmt_buf->buffer + fmt_buf->capacity);

        REALLOC_ARRAY(fmt_buf->buffer, char, fmt_buf->capacity);

        int confirm_format_size = vsnprintf(fmt_buf->cursor, (size_t)new_bytes_remaining, format, vargs);

        // sanity check
        assert(confirm_format_size == format_size);
    }

    fmt_buf->cursor += byte_write_count;

    assert(fmt_buf->capacity < STR_LENGTH_MAX);    
}


void FormatBuffer::writef(const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    formatbuffer_v_writef(this, format, vargs);
    va_end(vargs);
}


void FormatBuffer::writeln(const char *string)
{
    writef("%s\n", string);
}


void FormatBuffer::writef_ln(const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    formatbuffer_v_writef(this, format, vargs);
    va_end(vargs);
    writeln("");
}


void FormatBuffer::writef_indent(int indent, const char *format, ...)
{
    this->writef("%*s", indent, "");
    va_list vargs;
    va_start(vargs, format);
    formatbuffer_v_writef(this, format, vargs);
    va_end(vargs);
}


void FormatBuffer::writef_ln_indent(int indent, const char *format, ...)
{
    this->writef("%*s", indent, "");
    va_list vargs;
    va_start(vargs, format);
    formatbuffer_v_writef(this, format, vargs);
    writeln("");
    va_end(vargs);
}


void FormatBuffer::writeln_indent(int indent, const char *string)
{
    this->writef("%*s%s\n", indent, "", string);
}


#ifdef __clang__
#pragma clang diagnostic pop
#endif
