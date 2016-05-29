// -*- c++ -*-
#ifndef FORMATBUFFER_H

#if defined(__clang__) || defined(__GNUC__)
#define FORMAT_ARG_FLAGS(FMT) __attribute__((format(printf, FMT, (FMT+1))))
#else
#error Add a define for FORMAT_ARG_FLAGS for this compiler
#endif

#include "memory.h"


class FormatBuffer
{
public:
    typedef void flush_char_buffer_fn(void *userdata, const char *buf, size_t size);

    size_t capacity;
    char *buffer;
    size_t cursor;
    flush_char_buffer_fn *flush_fn;
    void *flush_fn_userdata;
    bool do_flush_on_destruct;
    mem::IAllocator *allocator;

    static void *default_flush_fn_userdata;
    static flush_char_buffer_fn *default_flush_fn;
    static const size_t DefaultCapacity = 1024;
    static const size_t MaxCapacity = UINT16_MAX;

    FormatBuffer()
        : capacity(DefaultCapacity)
        , buffer(0)
        , cursor(0)
        , flush_fn(nullptr)
        , do_flush_on_destruct(false)
        , allocator(mem::default_allocator())
        {
        }

    FormatBuffer(mem::IAllocator *use_allocator)
        : capacity(DefaultCapacity)
        , buffer(0)
        , cursor(0)
        , do_flush_on_destruct(false)
        , allocator(use_allocator ? use_allocator : mem::default_allocator())
        {
        }

    FormatBuffer(size_t initial_capacity, mem::IAllocator *use_allocator = nullptr)
        : capacity(initial_capacity > 0 ? initial_capacity : 2)
        , buffer(0)
        , cursor(0)
        , do_flush_on_destruct(false)
        , allocator(use_allocator ? use_allocator : mem::default_allocator())
        {
        }

    ~FormatBuffer();

    void flush_to_log();

    void write(char c);
    void write(const char *string, size_t length);
    void write(const char *string) { this->writef("%s", string); }
    void write_indent(int indent, const char *string);
    void writeln(const char *string);
    void writef(const char *format, ...) FORMAT_ARG_FLAGS(2);
    void writef_ln(const char *string, ...) FORMAT_ARG_FLAGS(2);
    void writef_indent(int indent, const char *string, ...) FORMAT_ARG_FLAGS(3);
    void writef_ln_indent(int indent, const char *string, ...) FORMAT_ARG_FLAGS(3);
    void writeln_indent(int indent, const char *string);
    void clear() { this->cursor = 0; }


    void flush_on_destruct(
        bool enable_flush_on_destruct = true,
        flush_char_buffer_fn *flush_fn_to_use = nullptr,
        void *flush_fn_userdata_to_use = nullptr);

    static void set_default_flush_fn(
        flush_char_buffer_fn *flush_fn_to_use = nullptr,
        void *flush_userdata = nullptr);

private:
    FormatBuffer(const FormatBuffer& other); // copy constructor
    FormatBuffer& operator=(const FormatBuffer& other); // copy assignment
};



#undef FORMAT_ARG_FLAGS

#define FORMATBUFFER_H
#endif
