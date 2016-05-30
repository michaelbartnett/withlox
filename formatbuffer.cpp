#include "formatbuffer.h"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cstdint>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif


void fallback_flush_fn(void *userdata, const char *buffer, size_t bufsize)
{
    UNUSED(userdata);
    fwrite(buffer, 1, bufsize, stdout);
}


// Storage for defaults
void *FormatBuffer::default_flush_fn_userdata = nullptr;
FormatBuffer::flush_char_buffer_fn *FormatBuffer::default_flush_fn = &fallback_flush_fn;



void FormatBuffer::set_default_flush_fn(
    FormatBuffer::flush_char_buffer_fn *flush_fn_to_use,
    void *flush_fn_userdata)
{
    FormatBuffer::default_flush_fn_userdata = flush_fn_userdata;
    FormatBuffer::default_flush_fn = flush_fn_to_use;
}


void FormatBuffer::flush_on_destruct(
    bool enable_flush_on_destruct,
    flush_char_buffer_fn *flush_fn_to_use,
    void *flush_fn_userdata_to_use)
{
    this->flush_fn = flush_fn_to_use;
    this->flush_fn_userdata = flush_fn_userdata_to_use;
    do_flush_on_destruct = enable_flush_on_destruct;
}


FormatBuffer::~FormatBuffer()
{
    if (do_flush_on_destruct)
    {
        this->flush_to_log();
    }
    // mem::IAllocator *allocator = mem::default_allocator();
    allocator->dealloc(this->buffer);
}


void FormatBuffer::flush_to_log()
{
    void *userdata_used = default_flush_fn_userdata;
    flush_char_buffer_fn *flush_fn_used = default_flush_fn;

    if (this->flush_fn)
    {
        flush_fn_used = this->flush_fn;
    }
    if (!this->flush_fn_userdata)
    {
        userdata_used = this->flush_fn_userdata;
    }

    if (this->cursor > 0)
    {
        flush_fn_used(userdata_used, this->buffer, this->cursor);
        this->clear();
    }
}





static void ensure_formatbuffer_space(FormatBuffer *fb, size_t length)
{
    size_t bytes_remaining = fb->capacity - fb->cursor;

    if (!fb->buffer)
    {
        if (fb->capacity < length)
        {
            fb->capacity = length;
        }
        fb->buffer = MAKE_ARRAY(char, fb->capacity, fb->allocator);
    }
    else if (bytes_remaining <= length)
    {
        fb->capacity = (fb->capacity * 2 < fb->capacity + length + 1
                        ? fb->capacity + length + 1
                        : fb->capacity * 2);
        RESIZE_ARRAY(fb->buffer, char, fb->capacity, fb->allocator);
    }
}


void FormatBuffer::write(char c)
{
    const size_t minlength = 5;

    ensure_formatbuffer_space(this, minlength);

    // if (!this->buffer)
    // {
    //     // Just set to 5, because if we go hyper-unicode, at least we'll always have 4 bytes available plus null terminator
    //     if (this->capacity < 5)
    //     {
    //         this->capacity = 5;
    //     }
    //     this->buffer = MAKE_ARRAY(char, this->capacity, this->allocator);
    // }
    // else if (bytes_remaining < 2)
    // {
    //     this->capacity
    // }

    this->buffer[this->cursor] = c;
    ++this->cursor;
    this->buffer[this->cursor] = '\0';
}


void FormatBuffer::write(const char *string, size_t length)
{
    ensure_formatbuffer_space(this, length);


    // size_t bytes_remaining = this->capacity - this->cursor;


    // if (!this->buffer)
    // {
    //     if (this->capacity < length)
    //     {
    //         this->capacity = length;
    //     }
    //     this->buffer = MAKE_ARRAY(char, this->capacity, allocator);

    // }
    // else if (bytes_remaining <= length)
    // {
    //     this->capacity = (this->capacity * 2 < this->capacity + length + 1
    //                       ? this->capacity + length + 1
    //                       : this->capacity * 2);
    //     RESIZE_ARRAY(this->buffer, char, this->capacity, allocator);
    // }

    std::memcpy(this->buffer + this->cursor, string, length);
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
        fmt_buf->buffer = MAKE_ARRAY(char, fmt_buf->capacity, fmt_buf->allocator);
    }

    size_t bytes_remaining = fmt_buf->capacity - fmt_buf->cursor;

    int printf_result = - 1;

    {
        va_list vargs_copy;
        va_copy(vargs_copy, vargs);
        printf_result = vsnprintf(fmt_buf->buffer + fmt_buf->cursor,
                                  bytes_remaining, format, vargs_copy);
        va_end(vargs_copy);
    }

    assert(printf_result >= 0);
    size_t byte_write_count = (size_t)printf_result;

    // format size should always be less than output size, otherwise we risk losing the null terminator
    if (byte_write_count >= bytes_remaining)
    {
        // size_t new_bytes_remaining = (byte_write_count + 1) - bytes_remaining;
        size_t new_bytes_remaining = byte_write_count + 1;
        assert(new_bytes_remaining > bytes_remaining);

        fmt_buf->capacity += new_bytes_remaining - bytes_remaining;
        assert(fmt_buf->capacity - fmt_buf->cursor > byte_write_count);

        RESIZE_ARRAY(fmt_buf->buffer, char, fmt_buf->capacity, fmt_buf->allocator);

        int confirm_result = vsnprintf(fmt_buf->buffer + fmt_buf->cursor,
                                       new_bytes_remaining, format, vargs);

        // sanity check
        assert(confirm_result >= 0);
        assert(confirm_result == printf_result);
    }

    fmt_buf->cursor += byte_write_count;


    assert(fmt_buf->capacity < FormatBuffer::MaxCapacity);
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
