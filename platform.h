// -*- c++ -*-

#ifndef PLATFORM_H

#include "str.h"
#include "numeric_types.h"
#include "common.h"
#include "memory.h"


#define ASSERT_MSG(msg) ASSERT(!(bool)(msg))

#define ASSERT(cond) assert((platform_assert(cond)))


typedef s32 ErrorCode;

struct PlatformError
{
    static const StrLen MaxMessageLen = 256;

    ErrorCode code;
    Str message;

    bool is_error()
    {
        return code != 0;
    }

    void release()
    {
        if (message.data) str_free(&message);
    }

    static PlatformError from_code(ErrorCode code);
};


struct FileReadResult
{
    enum ErrorKind
    {
        NoError,
        AccessDenied,
        NotFound,
        MaxPathExceeded,
        AlreadyExists,
        Other
    };

    ErrorKind error_kind;
    PlatformError platform_error;

    void release()
    {
        platform_error.release();
    }

    static FileReadResult from_error_code(ErrorCode code);
};


// TODO(mike): get_stacktrace which writes to a char buffer
void print_stacktrace(int skip_frames=0);

inline bool platform_assert(bool condition)
{
    if (condition) return true;
    std::printf(
        "ASSERTION FAILED\n\n"
        "---------------------------------------\n\n");
    print_stacktrace(1);
    std::printf(
        "\n---------------------------------------\n");
    return false;
}


PlatformError current_dir(OUTPARAM Str *path);

PlatformError change_dir(const char *path);

PlatformError resolve_path(OUTPARAM Str *dest, const char *path);


inline PlatformError change_dir(const Str path)
{
    return change_dir(path.data);
}


inline PlatformError change_dir(StrSlice path)
{
    Str pathstr = str(path);
    PlatformError result = change_dir(pathstr);
    str_free(&pathstr);
    return result;
}


void end_of_program();

void waitkey();

Str read_file(const char *filename);

FileReadResult read_text_file(Str *output, const char *filename);

void log_file_error(FileReadResult error, const char *prefix);

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


struct DirEntry
{
    bool is_file;
    bool is_directory;
    size_t filesize;
    StrSlice name;
    Str access_path;
};


class DirLister
{
public:
    Str path;
    DirEntry current;
    PlatformError error;
    s64 stream_loc;
    void *pimpl;

    DirLister(const char *path, size_t length);

    DirLister(const char *path);
    DirLister(const StrSlice path);

    ~DirLister();

    bool has_error()
    {
        return error.is_error();
    }

    bool next();
};

#define PLATFORM_H
#endif
