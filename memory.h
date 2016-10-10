// -*- c++ -*-

#ifndef MEMORY_H

#include "common.h"
#include "numeric_types.h"
#include <cstring>

#define DEFAULT_ALIGN 8

#define MAKE_OBJ(allocator, ...) (__VA_ARGS__ *)allocator->realloc( \
        0, sizeof (__VA_ARGS__), DEFAULT_ALIGN,                     \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__))
#define MAKE_ARRAY(allocator, count, ...) (__VA_ARGS__ *)allocator->realloc( \
        0, count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,                 \
        (mem::AllocationMetadata(__FILE__ ":" S__LINE__)))
#define MAKE_ZEROED_ARRAY(allocator, count, ...) (__VA_ARGS__ *)allocator->calloc( \
        count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,                    \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__))
#define RESIZE_ARRAY(allocator, ptr, count, ...) ptr = (__VA_ARGS__ *)allocator->realloc( \
        ptr, count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,               \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__))

#define MAKE_OBJ_CAT(allocator, category, ...) (__VA_ARGS__ *)allocator->realloc( \
        0, sizeof (__VA_ARGS__), DEFAULT_ALIGN,                         \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__, category))
#define MAKE_ARRAY_CAT(allocator, count, category, ...) (__VA_ARGS__ *)allocator->realloc( \
        0, count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,                 \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__, category))
#define MAKE_ZEROED_ARRAY_CAT(allocator, count, category, ...) (__VA_ARGS__ *)allocator->calloc( \
        count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,                    \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__, category))
#define RESIZE_ARRAY_CAT(allocator, ptr, count, category, ...) ptr = (__VA_ARGS__ *)allocator->realloc( \
        ptr, count * sizeof (__VA_ARGS__), DEFAULT_ALIGN,               \
        mem::AllocationMetadata(__FILE__ ":" S__LINE__, category))


namespace mem
{

extern bool ALLOC_STACKTRACE;

using std::size_t;

typedef void logf_fn(void *userdata, const char *fmt, ...);

void memory_init(logf_fn *logf, void *userdata);


class AllocationMetadata
{
public:
    const char *sourceline;
    const char *category;

    AllocationMetadata()
        : sourceline("")
        , category("")
    {
    }

    AllocationMetadata(const char *sourceline_)
        : sourceline(sourceline_)
        , category("")
    {
    }


    AllocationMetadata(const char *sourceline_, const char *category_)
        : sourceline(sourceline_)
        , category(category_)
    {
    }
};


class IAllocator
{
public:
    virtual void *realloc(void *ptr, size_t size, size_t align, AllocationMetadata meta) = 0;
    virtual void dealloc(void *ptr) = 0;
    virtual size_t bytes_allocated() = 0;
    virtual size_t payload_size_of(void *ptr) = 0;
    virtual void log_allocations() = 0;

    virtual void* probe() = 0;
    virtual void log_allocs_since_probe(void *probe) = 0;

    virtual ~IAllocator() {}

    void *calloc(size_t size, size_t align, AllocationMetadata meta)
    {
        void *result = this->realloc(0, size, align, meta);
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

    virtual void   *realloc(void *ptr, size_t size, size_t align, AllocationMetadata meta) OVERRIDE ;
    virtual void    dealloc(void *ptr) OVERRIDE;
    virtual size_t  bytes_allocated() OVERRIDE;
    virtual size_t  payload_size_of(void *ptr) OVERRIDE;
    virtual void    log_allocations() OVERRIDE;

    virtual void* probe() OVERRIDE
    {
        assert(!(bool)"Not supported");
    }
    virtual void log_allocs_since_probe(void *probe) OVERRIDE
    {
        UNUSED(probe);
        assert(!(bool)"Not supported");
    }
};


class Mallocator : public IAllocator
{
public:
    struct MemBlockHeader
    {
        size_t total_size;
        MemBlockHeader *next;
        MemBlockHeader *prev;
        const char *category;
        const char *sourceline;
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
    virtual void *realloc(void *ptr, size_t size, size_t align, AllocationMetadata meta) OVERRIDE;
    virtual void dealloc(void *ptr) OVERRIDE;
    virtual size_t bytes_allocated() OVERRIDE;

    virtual void* probe() OVERRIDE;
    virtual void log_allocs_since_probe(void *probe) OVERRIDE;

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
