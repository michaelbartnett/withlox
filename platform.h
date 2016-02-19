// -*- c++ -*-

#include "str.h"
#include <cstdlib>
#include <sys/stat.h>
#include <mach/mach_time.h>

#include "numeric_types.h"

Str read_file(const char *filename);

static mach_timebase_info_data_t mach_timebase = {};


inline u64 query_abstime()
{
    return mach_absolute_time();
}


inline u64 nanoseconds_since(u64 later, u64 earlier)
{
    if (mach_timebase.denom == 0) {
        mach_timebase_info(&mach_timebase);
    }

    u64 diff = later - earlier;
    return diff * mach_timebase.numer / mach_timebase.denom;
}

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
    return nanoseconds_since(mach_absolute_time(), earlier);
}

inline double microseconds_since(u64 earlier)
{
    return (double)microseconds_since(mach_absolute_time(), earlier);
}

inline double milliseconds_since(u64 earlier)
{
    return milliseconds_since(mach_absolute_time(), earlier);
}

inline double seconds_since(u64 earlier)
{
    return seconds_since(mach_absolute_time(), earlier);
}
