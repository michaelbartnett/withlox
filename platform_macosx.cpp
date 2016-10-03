// -*- c++ -*-

#include "platform.h"
#include "formatbuffer.h"
#include "logging.h"
#include <sys/stat.h>
#include <sys/param.h>
#include <mach/mach_time.h>
// #include <errno.h>
#include <cerrno>
#include <dirent.h>
#include <unistd.h>


#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>


#define MAX_OP_TRIES 10


PlatformError current_dir(OUTPARAM Str *result)
{
    str_ensure_capacity(result, MAXPATHLEN);
    char *ok = getcwd(result->data, result->capacity);

    PlatformError error_result = ok
        ? PlatformError::from_code(0)
        : PlatformError::from_code(errno);
    assert(error_result.code == 0 || error_result.code == EACCES);

    return error_result;
}


PlatformError change_dir(const char *path)
{
    PlatformError result = {};
    int had_error = chdir(path);
    if (had_error)
    {
        result = PlatformError::from_code(errno);
    }
    return result;
}


PlatformError resolve_path(Str *dest, const char *path)
{
    PlatformError error = {};
    str_ensure_capacity(dest, MAXPATHLEN);

    bool ok = realpath(path, dest->data);

    if (!ok)
    {
        error = PlatformError::from_code(errno);
    }

    return error;
}


void end_of_program()
{
    // noop
}

void waitkey()
{
    println("Press return to continue...");
    getchar();
}


PlatformError read_filesize(OUTPARAM size_t *filesize, const char *filename)
{
    assert(filesize);
    struct stat statbuf;
    int err = stat(filename, &statbuf);
    PlatformError error_result = {};

    if (0 != err)
    {
        error_result = PlatformError::from_code(errno);
        *filesize = 0;
    }
    else
    {
        *filesize = (size_t)max(statbuf.st_size, 0LL);
    }

    return error_result;
}


Str read_file(const char *filename)
{
    Str result = {};
    // struct stat statbuf;
    // int err = stat(filename, &statbuf);

    // if (0 != err)
    // {
    //     std::printf("Failed to stat file: %s\nReason: %s\n", filename, std::strerror(err));
    //     return result;
    // }

    // assert(statbuf.st_size > 0);
    // u16 filesize = (u16)statbuf.st_size;

    size_t read_filesize_result;
    PlatformError err = read_filesize(&read_filesize_result, filename);
    if (err.is_error())
    {
        logf("Failed to stat file: %s\nReason: %s", filename, err.message.data);
    }

    u16 filesize = STRLEN(read_filesize_result);
    result = str_alloc(filesize);
    std::FILE *f = std::fopen(filename, "r");
    assert(f);
    size_t bytes_read = fread(result.data, 1, filesize, f);
    result.length = filesize;
    fclose(f);

    assert(bytes_read == filesize);

    return result;
}


FileReadResult::ErrorKind check_file_error(int err)
{
    switch (err)
    {
        case 0:
            return FileReadResult::NoError;
        case ENOENT:
        case ENOTDIR:
            return FileReadResult::NotFound;
        case ENAMETOOLONG:
            return FileReadResult::MaxPathExceeded;
        case EEXIST:
            return FileReadResult::AlreadyExists;
        default:
            break;
    }
    return FileReadResult::Other;
}


// void log_file_error(FileReadResult error, const char *prefix, const char *filename)
// {
//     FormatBuffer fb;
//     fb.writef(fmt_with_filename, filename);
//     logf_ln("%s: %s\n", prefix, std::strerror(err));
// }


FileReadResult read_text_file(Str *dest, const char *filename)
{
    // FileReadResult result;

    // struct stat statbuf;
    // int err = stat(filename, &statbuf);

    // if (err)
    // {
        // return FileReadResult::from_error_code(err);
    // }

    // assert(statbuf.st_size > 0);
    size_t filesize_outparam;
    PlatformError readsize_error = read_filesize(&filesize_outparam, filename);

    if (readsize_error.is_error())
    {
        FileReadResult error_result = {check_file_error(readsize_error.code), readsize_error};
        return error_result;
    }

    // NOTE(mike): read_filesize doesn't allocate anything in
    // readsize_error if there is no error should probably make the
    // other PlatformError instances do the same thing

    StrLen filesize = STRLEN(filesize_outparam);
    str_ensure_capacity(dest, filesize);
    std::FILE *f = std::fopen(filename, "r");

    if (!f)
    {
        assert(errno);
        // result.platform_error = PlatformError::from_code(errno);
        // result.error_kind = check_file_error(result.platform_error.code);
        // return result;
        return FileReadResult::from_error_code(errno);
    }

    size_t bytes_read = fread(dest->data, 1, filesize, f);

    if (bytes_read != filesize)
    {
        // we shouldn't have have the EOF...
        assert(!feof(f));

        // so just try to return an error, assert that there is an error
        // result.platform_error = PlatformError::from_code(ferror(f));
        // result.error_kind = check_file_error(result.platform_error.code);
        FileReadResult error_result = FileReadResult::from_error_code(ferror(f));
        assert(error_result.platform_error.code != 0);
        return error_result;
    }

    dest->length = filesize;

    int fcloseerr = fclose(f);
    assert(fcloseerr == 0);

    // result.platform_error = PlatformError::from_code(0);
    // result.error_kind = FileReadResult::NoError;
    return FileReadResult::from_error_code(0);
}



static mach_timebase_info_data_t mach_timebase = {};


u64 query_abstime()
{
    return mach_absolute_time();
}


u64 nanoseconds_since(u64 later, u64 earlier)
{
    if (mach_timebase.denom == 0) {
        mach_timebase_info(&mach_timebase);
    }

    u64 diff = later - earlier;
    return diff * mach_timebase.numer / mach_timebase.denom;
}


PlatformError PlatformError::from_code(ErrorCode error_code)
{
    Str errormsg;
    if (error_code == 0)
    {
        errormsg = str("");
    }
    else
    {
        errormsg = str_alloc(MaxMessageLen);
        int strerror_error = strerror_r(error_code, errormsg.data, errormsg.capacity);
        if (strerror_error == ERANGE)
        {
            logln("WARNING: error message truncated");
        }
        else if (strerror_error != 0)
        {
            logf("Unexpected error reading strerror: %s", strerror(strerror_error));
        }
        else
        {
            assert(strerror_error == 0 || strerror_error == ERANGE);
        }
    }

    PlatformError result = {error_code, errormsg};
    return result;
}


FileReadResult FileReadResult::from_error_code(ErrorCode code)
{
    FileReadResult result;
    result.error_kind = check_file_error(code);
    result.platform_error = PlatformError::from_code(code);
    return result;
}



void print_stacktrace(int skip_frames)
{
    void* callstack[128];
    int n_frames = backtrace(callstack, 128);

    // std::fprintf(stream, "...................................\n");

    // char **btsymbols = backtrace_symbols(callstack + skip_frames, n_frames - skip_frames);
    // for (int i = 1; i < n_frames - skip_frames; ++i) {
    //     std::fprintf(stream, "%s\n", btsymbols[i]);
    // }

    // std::fprintf(stream, "...................................\n");

    // for (int i = skip_frames + 1; i < n_frames; ++i) {
    //     Dl_info dlinfo;
    //     std::fprintf(stream, "\n");
    //     if (dladdr(callstack[i], &dlinfo)) {

    //         std::fprintf(stream, "addr -> %p\nfname -> %s, fbase -> %p\nsname -> %s, sbase -> %p\n",
    //                      callstack[i], dlinfo.dli_fname, dlinfo.dli_fbase, dlinfo.dli_sname, dlinfo.dli_saddr);

    //         char demangled[256];
    //         int status = 0;
    //         size_t bufsize = sizeof(demangled);
    //         abi::__cxa_demangle(dlinfo.dli_sname, demangled, &bufsize, &status);
    //         if (status == 0) {
    //             std::fprintf(stream, "%s\n", demangled);
    //         } else {
    //             std::fprintf(stream, "(mangled): %s\n", dlinfo.dli_sname);
    //         }

    //     } else {
    //         std::fprintf(stream, "failed to get dl info for call frame\n");
    //     }
    // }

    // std::fprintf(stream, "\n...................................\n\n");

    {
        char cmdbuf[512];
        pid_t pid = getpid();
        for (int i = 1 + skip_frames; i < n_frames; ++i) {
            std::sprintf(cmdbuf, "xcrun atos -d -p %i %p", pid, callstack[i]);
            std::system(cmdbuf);
        }
    }
}


//////////////// DirLister BEGIN ////////////////

static void DirLister_init(DirLister *dl, Str path)
{
    ZERO_PTR(dl);

    DIR *dirp = opendir(path.data);

    if (!dirp)
    {
        dl->error = PlatformError::from_code(errno);
        dl->pimpl = nullptr;
        return;
    }

    // Assumes ownership of path
    // Always end in a path separator
    if (path.data[path.length - 1] != '/')
    {
        str_append(&path, '/');
    }
    else
    {
        while (path.length > 2 && path.data[path.length - 2] == '/')
        {
            str_popchar(&path);
        }
    }

    dl->path = path;
    dl->stream_loc = 0;
    dl->pimpl = dirp;

    // current.access_path will always be prefixed by path
    str_overwrite(&dl->current.access_path, str_slice(dl->path));
    dl->current.name = str_slice(dl->current.access_path.data + dl->current.access_path.length, size_t(0));
}


static void DirLister_deinit(DirLister *dl)
{
    assert(dl->pimpl);

    str_clear(&dl->current.access_path);
    dl->current.is_file = false;
    dl->current.is_directory = false;
    dl->current.filesize = 0;

    DIR *dirp = (DIR *)dl->pimpl;

    for (s32 i = 0; i < MAX_OP_TRIES; ++i)
    {
        int close_error = closedir(dirp);

        if (close_error != EINTR)
        {
            goto CloseDirSuccess;
        }

        // otherwise, we got it EINTR, which means interrupted, so try
        // to close it a few more times
    }
    ASSERT_MSG("closedir failed after " STRFY(MAX_OP_TRIES) " tries, kept getting interrupted");

CloseDirSuccess:
    dl->pimpl = nullptr;
}


DirLister::DirLister(const char *dirpath)
{
    DirLister_init(this, str(dirpath));
}
DirLister::DirLister(const char *dirpath, size_t length)
{
    DirLister_init(this, str(dirpath, STRLEN(length)));
}
DirLister::DirLister(const StrSlice dirpath)
{
    DirLister_init(this, str(dirpath));
}
// DirLister::DirLister(const Str &path)
// {
//     DirLister_init(this, path.data);
// }


DirLister::~DirLister()
{
    if (pimpl)
    {
        DirLister_deinit(this);
    }

    if (this->current.name.data)
    {
        str_free(&this->current.access_path);
    }
}


bool DirLister::next()
{
    if (!this->pimpl) return false;

    DIR *dirp = (DIR *)this->pimpl;

    for (;;)
    {
        dirent entry;
        dirent *pentry;
        int read_error = readdir_r(dirp, &entry, &pentry);

        if (read_error)
        {
            this->error = PlatformError::from_code(read_error);
            return false;
        }

        if (!pentry)
        {
            DirLister_deinit(this);
            return false;
        }

        

        switch (entry.d_type)
        {
            case DT_REG:
                this->current.is_file = true;
                this->current.is_directory = false;

                break;

            case DT_DIR:
            {
                // Skip dot directories
                if ((entry.d_namlen == 1 && entry.d_name[0] == '.') ||
                    (entry.d_namlen == 2 && entry.d_name[0] == '.' && entry.d_name[1] == '.'))
                {
                    continue;
                }

                this->current.is_file = false;
                this->current.is_directory = true;

                StrSlice nameslice = str_slice(entry.d_name);
                str_copy_truncate(&this->current.access_path, this->path.length, nameslice, 0, nameslice.length);
                this->current.name = str_slice(this->current.access_path.data + this->path.length);
                break;
            }

            default:
                // Skip anything that's not a file or a directo ry (i.e. no symlinks)
                continue;
        }

        this->stream_loc = telldir(dirp);

        StrSlice nameslice = str_slice(entry.d_name);
        str_copy_truncate(&this->current.access_path, this->path.length, nameslice, 0, nameslice.length);
        this->current.name = str_slice(this->current.access_path.data + this->path.length);

        if (this->current.is_file)
        {
            size_t filesize_result;
            PlatformError staterror = read_filesize(OUTPARAM &filesize_result, this->current.access_path.data);
            if (staterror.is_error())
            {
                this->error = staterror;
                return false;
            }
            this->current.filesize = filesize_result;
        }
        else
        {
            this->current.filesize = 0;
        }

        break;
    }

    return true;
}

/////////////// DirLister END ///////////////
