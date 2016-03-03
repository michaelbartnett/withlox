#include "common.h"
#include "platform.h"
#include "Windows.h"
#include <sys/stat.h>
#include <cassert>

void end_of_program()
{
    waitkey();
}

void waitkey()
{
	system("pause");
}


Str read_file(const char *filename)
{
    char curdir[MAX_PATH];
    GetCurrentDirectory(sizeof(curdir), curdir);
    printf_ln("Current directory: %s", curdir);
    
    Str result = {};
    struct stat statbuf;

    int err = stat(filename, &statbuf);

    if (0 != err)
    {
        std::printf("Failed to stat file: %s\nReason: %s\n", filename, std::strerror(err));
        return result;
    }

    assert(statbuf.st_size > 0);
    u16 filesize = (u16)statbuf.st_size;
    result = str_alloc(filesize);
    std::FILE *f = std::fopen(filename, "r");
    size_t bytes_read = fread(result.data, 1, filesize, f);
    result.length = filesize;
    fclose(f);

    assert(bytes_read == filesize);

    return result;
}


static u64 counter_frequency = 0;

u64 query_abstime()
{
    u64 result;
	LARGE_INTEGER li;
    bool ok = QueryPerformanceCounter(&li);
    assert(ok);
	result = li.QuadPart;
    return result;
}


u64 nanoseconds_since(u64 later, u64 earlier)
{
    if (counter_frequency == 0) {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        counter_frequency = li.QuadPart;
    }

    u64 diff = later - earlier;

    // 10^9 nanoseconds in a second
    u64 result = 1000000000;
    result /= counter_frequency;
    return result;
}
