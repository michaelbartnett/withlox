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
    append(&log_entries, log_entry);
    concatenated_dirty = true;
}


void append_log(const char *string)
{
    append_log(str(string));
}


void append_log(const char *string, size_t length)
{
    assert(length < STR_LENGTH_MAX);
    append_log(str(string, (u16)length));
}


static void vlogf(const char *format, va_list vargs)
{
    // lazily allocate the output buffer
    if (!output_buffer)
    {
        output_buffer_size = FormatBuffer::default_format_buffer_capacity;
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
    return get(log_entries, i);
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
            Str *str = get(log_entries, i);
            concatenated->write(str->data, str->length);
            concatenated->write("\n", 1);
        }
        concatenated_dirty = false;
        next_log_entry_to_concat = log_entries.count;
    }

    Str result;
    assert(concatenated->cursor < STR_LENGTH_MAX);
    result.data = concatenated->buffer;
    result.length = (u16)concatenated->cursor;
    result.capacity = result.capacity;
    return result;
}


void init_formatbuffer(FormatBuffer *fb, size_t initial_capacity)
{
    fb->capacity = initial_capacity;
    fb->buffer = 0;
    fb->cursor = 0;
}

// FormatBuffer::FormatBuffer(size_t initial_capacity)
// {
//     init_formatbuffer(this, initial_capacity);
// }

// FormatBuffer::FormatBuffer()
// {
//     init_formatbuffer(this, default_format_buffer_capacity);
// }

FormatBuffer::~FormatBuffer()
{
    if (do_flush_on_destruct)
    {
        this->flush_to_log();
    }
    std::free(this->buffer);
}


void FormatBuffer::flush_to_log()
{
    if (this->cursor > 0)
    {
        logf("%s", this->buffer);
        this->clear();
    }
}


void FormatBuffer::write(const char *string, size_t length)
{
    size_t bytes_remaining = this->capacity - this->cursor;

    if (!this->buffer)
    {
        if (this->capacity < length)
        {
            this->capacity = length;
        }
        this->buffer = MALLOC_ARRAY(char, this->capacity);
        
    }
    else if (bytes_remaining <= length)
    {
        this->capacity = (this->capacity * 2 < this->capacity + length + 1
                          ? this->capacity + length + 1
                          : this->capacity * 2);
        REALLOC_ARRAY(this->buffer, char, this->capacity);
    }

    memcpy(this->buffer + this->cursor, string, length);
    this->cursor += length;
    this->buffer[this->cursor] = '\0';
}

void FormatBuffer::write_indent(int indent, const char *string)
{
    writef("%*s", indent, string);
}

void formatbuffer_v_writef(FormatBuffer *fmt_buf, const char *format, va_list vargs)
{
    if (!fmt_buf->buffer)
    {
        fmt_buf->buffer = MALLOC_ARRAY(char, fmt_buf->capacity);
    }

    size_t bytes_remaining = fmt_buf->capacity - fmt_buf->cursor;
    // ptrdiff_t bytes_remaining = (fmt_buf->buffer + fmt_buf->capacity) - fmt_buf->cursor;
    assert(bytes_remaining >= 0);

    int printf_result = vsnprintf(fmt_buf->buffer + fmt_buf->cursor,
                                (size_t)bytes_remaining, format, vargs);
    assert(printf_result >= 0);
    size_t byte_write_count = (size_t)printf_result;

    // format size should always be less than output size, otherwise we risk losing the null terminator
    if (byte_write_count >= bytes_remaining)
    {
        size_t new_bytes_remaining = byte_write_count - bytes_remaining + 1;
        assert(new_bytes_remaining > bytes_remaining);

        fmt_buf->capacity += (size_t)new_bytes_remaining;
        assert(fmt_buf->cursor + new_bytes_remaining == fmt_buf->capacity);

        REALLOC_ARRAY(fmt_buf->buffer, char, fmt_buf->capacity);

        int confirm_result = vsnprintf(fmt_buf->buffer + fmt_buf->cursor,
                                            (size_t)new_bytes_remaining, format, vargs);

        // sanity check
        assert(confirm_result >= 0);
        assert(confirm_result == printf_result);
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
