// -*- c++ -*-

#ifndef LOGGING_H

#include "memory.h"
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

// Access to the global concatenation of all log entries that can fit in a Str
Str *concatenated_log();
void clear_concatenated_log();

// append to the log entries directly without writing to stdout
void append_log(const char *string);

// printf-like functions that prints to stdout and adds to global log_entries
void log_write_with_userdata(void *userdata, const char *buffer, size_t length);
void logf_with_userdata(void *userdata, const char *format, ...) FORMAT_ARG_FLAGS(2);
void logf(const char *format, ...) FORMAT_ARG_FLAGS(1);
void logln(const char *string);
void logf_ln(const char *format, ...) FORMAT_ARG_FLAGS(1);

inline void log(const char *string)
{
    logf("%s", string);
}


#undef FORMAT_ARG_FLAGS

#define LOGGING_H
#endif
