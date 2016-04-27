#ifndef MEMORY_H

// -*- c++ -*-
#include "common.h"
#include "numeric_types.h"
#include <cstring>

#define MAKE_OBJ(type, allocator) (type *)allocator->realloc(0, sizeof (type), 4)
#define MAKE_ARRAY(type, count, allocator) (type *)allocator->realloc(0, count * sizeof (type), 4)
#define MAKE_ZEROED_ARRAY(type, count, allocator) (type *)allocator->calloc(count * sizeof (type), 4)
// #define RESIZE_ARRAY(ptr, count, allocator) ( *)allocator->realloc(ptr, count * sizeof (*ptr), 4)
#define RESIZE_ARRAY(ptr, type, count, allocator) ptr = (type *)allocator->realloc(ptr, count * sizeof (type), 4)


namespace mem
{

using std::size_t;

typedef void logf_fn(void *userdata, const char *fmt, ...);

void memory_init(logf_fn *logf, void *userdata);

class IAllocator
{
public:
    virtual void *realloc(void *ptr, size_t size, size_t align) = 0;
    virtual void dealloc(void *ptr) = 0;
    virtual size_t bytes_allocated() = 0;
    virtual size_t payload_size_of(void *ptr) = 0;
    virtual void log_allocations() = 0;
    virtual ~IAllocator() {}

    void *calloc(size_t size, size_t align)
    {
        void *result = this->realloc(0, size, align);
        std::memset(result, 0, size);
        return result;
    }
};


class FallbackAllocator : public IAllocator
{
public:
    size_t bytecount;

    FallbackAllocator()
        : bytecount(0)
    {
    }

    virtual void   *realloc(void *ptr, size_t size, size_t align) OVERRIDE ;
    virtual void    dealloc(void *ptr) OVERRIDE;
    virtual size_t  bytes_allocated() OVERRIDE;
    virtual size_t  payload_size_of(void *ptr) OVERRIDE;
    virtual void    log_allocations() OVERRIDE;
};


class Mallocator : public IAllocator
{
public:
    struct MemBlockHeader
    {
        size_t total_size;
        MemBlockHeader *next;
        MemBlockHeader *prev;
        u8 alignment;
        u8 payload_offset;

        // This might be unnecessary
        // Make sure there's at least enough room to write the
        // lookback byte before the payload
        u8 pad_[1];

        size_t payload_size()
        {
            return total_size - payload_offset;
        }

        void *payload()
        {
            u8 *headerptr = (u8 *)this;
            u8 *payload = headerptr + payload_offset;
            return payload;
        }
    };

    size_t bytecount;
    size_t allocation_count;
    bool logging_allocations;
    int callcount;
    size_t alloc_count;
    MemBlockHeader *alloclist_head;

    static const size_t HeaderSize = sizeof(MemBlockHeader);
    STATIC_ASSERT(memblock_header_size_fits_in_byte, (16 + sizeof(MemBlockHeader)) <= UINT8_MAX);



Mallocator()
        : bytecount(0)
        , allocation_count(0)
        , logging_allocations(false)
        , callcount(0)
        , alloc_count(0)
        , alloclist_head(nullptr)
    {
    }


    virtual void log_allocations() OVERRIDE ;
    virtual size_t payload_size_of(void *ptr) OVERRIDE;
    virtual void *realloc(void *ptr, size_t size, size_t align) OVERRIDE;
    virtual void dealloc(void *ptr) OVERRIDE;
    virtual size_t bytes_allocated() OVERRIDE;
    MemBlockHeader *get_header(void *ptr);
    void validate_alloclist();
    bool alloclist_contains(void *ptr);
};


IAllocator *default_allocator();
void log_memcalls();
IAllocator *make_mallocator();
void *get_mallocator_header(IAllocator *allocator, void *ptr);
}


#define MEMORY_H
#endif
