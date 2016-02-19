#include "platform.h"


Str read_file(const char *filename)
{
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
