// -*- c++ -*-

#include "str.h"
#include "numeric_types.h"
#include "common.h"


void end_of_program();

void waitkey();

Str read_file(const char *filename);

u64 query_abstime();

u64 nanoseconds_since(u64 later, u64 earlier);


inline double microseconds_since(u64 later, u64 earlier)
{
    return (double)nanoseconds_since(later, earlier) * 0.001;
}

inline double milliseconds_since(u64 later, u64 earlier)
{
    return microseconds_since(later, earlier) * 0.001;
}

inline double seconds_since(u64 later, u64 earlier)
{
    return milliseconds_since(later, earlier) * 0.001;
}

inline u64 nanoseconds_since(u64 earlier)
{
    return nanoseconds_since(query_abstime(), earlier);
}

inline double microseconds_since(u64 earlier)
{
    return (double)microseconds_since(query_abstime(), earlier);
}

inline double milliseconds_since(u64 earlier)
{
    return milliseconds_since(query_abstime(), earlier);
}

inline double seconds_since(u64 earlier)
{
    return seconds_since(query_abstime(), earlier);
}

// class FilesystemWalker
// {
//     virtual const char *current_name();
//     virtual bool current_isdir();
//     virtual bool has_next();

//     virtual ~IFilesystemWalker()
//     {
//     }
// }
