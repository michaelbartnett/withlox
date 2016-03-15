// -*- c++ -*-

#ifndef LOGGING_H

#include "str.h"
#include "numeric_types.h"


#if defined(__clang__) || defined(__GNUC__)
#define FORMAT_ARG_FLAGS(FMT) __attribute__((format(printf, FMT, (FMT+1))))
#else
#error Add a define for FORMAT_ARG_FLAGS for this compiler
#endif


// returns number of log entries
u32 log_count();

// returns log entry at index
Str *get_log(u32 i);
Str concatenated_log();

// append to the log entries directly without writing to stdout
void append_log(const char *string);

// printf-like functions that prints to stdout and adds to global log_entries
void logf(const char *format, ...) FORMAT_ARG_FLAGS(1);
void logln(const char *string);
void logf_ln(const char *format, ...);
inline void log(const char *string)
{
    logf("%s", string);
}


class FormatBuffer
{
public:
    FormatBuffer()
        : capacity(default_format_buffer_capacity)
        , buffer(0)
        , cursor(0)
        , do_flush_on_destruct(false)
        {   
        }

    FormatBuffer(size_t initial_capacity)
        : capacity(initial_capacity)
        , buffer(0)
        , cursor(0)
        , do_flush_on_destruct(false)
        {
        }

    ~FormatBuffer();

    void write(const char *string, size_t length);
    void write(const char *string) { this->writef("%s", string); }
    void writeln(const char *string);
    void writef(const char *format, ...) FORMAT_ARG_FLAGS(2);
    void writef_ln(const char *string, ...) FORMAT_ARG_FLAGS(2);
    void writef_indent(int indent, const char *string, ...) FORMAT_ARG_FLAGS(3);
    void writef_ln_indent(int indent, const char *string, ...) FORMAT_ARG_FLAGS(3);
    void writeln_indent(int indent, const char *string);
    void clear() { this->cursor = 0; }

    void flush_on_destruct(bool enable_flush_on_destruct = true)
    {
        do_flush_on_destruct = enable_flush_on_destruct;
    }

    void flush_to_log();

    size_t capacity;
    char *buffer;
    size_t cursor;
    bool do_flush_on_destruct;

    static const size_t default_format_buffer_capacity = 1024;
};


#undef FORMAT_ARG_FLAGS

#define LOGGING_H
#endif
