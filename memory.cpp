#include "memory.h"
#include "numeric_types.h"
#include "common.h"
#include "platform.h"
#include <cstdlib>
#include <new>
#include <cstring>
#include <vector>
#include <map>


#if defined(__APPLE__)
#include <malloc/malloc.h>

#define MALLOC_GET_SIZE(ptr) malloc_size(ptr)
#elif defined(__linux__)
#include <malloc.h>
#define MALLOC_GET_SIZE(ptr) malloc_usable_size(ptr)
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif

namespace mem
{

struct MemorySysState
{
    logf_fn *logf;
    void *userdata;
};

static MemorySysState sys_state;

void memory_init(logf_fn *logf, void *userdata)
{
    sys_state.logf = logf;
    sys_state.userdata = userdata;
}


////////////////////////////////////////////////////////
////////////////// Fallback Allocator //////////////////
////////////////////////////////////////////////////////

void *FallbackAllocator::realloc(void *ptr, size_t size, size_t align, AllocationMetadata meta) OVERRIDE
{
    UNUSED(align);
    UNUSED(meta);
    assert(size > 0);
    if (ptr)
    {
        bytecount -= payload_size_of(ptr);
        ptr = std::realloc(ptr, size);
    }
    else
    {
        ptr = std::malloc(size);
    }
    bytecount += size;
    return ptr;
}

void FallbackAllocator::dealloc(void *ptr) OVERRIDE
{
    bytecount -= payload_size_of(ptr);
    std::free(ptr);
}

size_t FallbackAllocator::bytes_allocated() OVERRIDE
{
    return bytecount;
}

size_t FallbackAllocator::payload_size_of(void *ptr) OVERRIDE
{
    return MALLOC_GET_SIZE(ptr);
}

void FallbackAllocator::log_allocations() OVERRIDE
{
    sys_state.logf(sys_state.userdata, "log_allocations not implemented for FallbackAllocator\n");
}


////////////////////////////////////////////////
////////////////// Mallocator //////////////////
////////////////////////////////////////////////

void Mallocator::log_allocations() OVERRIDE
{
    struct AllocLogInfo
    {
        void *header;
        size_t payload_size;
        u32 alignment;
        const char *sourceline;
        const char *category;
        
    };

    AllocLogInfo *allocinfo_list = MAKE_ARRAY(this, alloc_count, AllocLogInfo);
    size_t allocinfo_count = alloc_count - 1;

    logging_allocations = true;

    {
        size_t i = 0;
        MemBlockHeader *header = alloclist_head;
        assert(header->payload() == allocinfo_list);
        header = header->next;
        AllocLogInfo *allocinfo_iter = allocinfo_list;

        while (header)
        {
            allocinfo_iter->header = header;
            allocinfo_iter->payload_size = header->payload_size();
            allocinfo_iter->alignment = header->alignment;
            allocinfo_iter->sourceline = header->sourceline;
            allocinfo_iter->category = header->category;
            ++i;
            header = header->next;
            ++allocinfo_iter;
        }

        assert(i == allocinfo_count);
        assert(allocinfo_iter == allocinfo_list + allocinfo_count);
    }

    logging_allocations = false;

    sys_state.logf(sys_state.userdata, "--- LOG ALLOCS %lu ---\n", alloc_count);

    for (size_t i = 0; i < allocinfo_count; ++i)
    {
        sys_state.logf(sys_state.userdata,
                       "category: \"%s\", address: %p payload_size: %u, alignment: %u, source: \"%s\"\n",
                       allocinfo_list[i].category,
                       allocinfo_list[i].header,
                       allocinfo_list[i].payload_size,
                       allocinfo_list[i].alignment,
                       allocinfo_list[i].sourceline);
    }

    dealloc(allocinfo_list);
}


void *Mallocator::probe() OVERRIDE
{
    return alloclist_head;
}


void Mallocator::log_allocs_since_probe(void *probe) OVERRIDE
{
    struct AllocLogInfo
    {
        void *header;
        size_t total_size;
        size_t payload_size;
        u32 alignment;
        const char *sourceline;
        const char *category;
    };

    AllocLogInfo *allocinfo_list = MAKE_ARRAY(this, alloc_count, AllocLogInfo);
    size_t allocinfo_count = 0;

    logging_allocations = true;

    {
        MemBlockHeader *last = static_cast<MemBlockHeader *>(probe);
        size_t i = 0;
        MemBlockHeader *header = alloclist_head;
        assert(header->payload() == allocinfo_list);
        header = header->next;
        AllocLogInfo *allocinfo_iter = allocinfo_list;

        while (header != last && header)
        {
            allocinfo_iter->header = header;
            allocinfo_iter->total_size = header->total_size;
            allocinfo_iter->payload_size = header->payload_size();
            allocinfo_iter->alignment = header->alignment;
            allocinfo_iter->sourceline = header->sourceline;
            allocinfo_iter->category = header->category;
            ++i;
            header = header->next;
            ++allocinfo_iter;
        }

        allocinfo_count = i;
        assert(allocinfo_iter == allocinfo_list + allocinfo_count);
    }

    logging_allocations = false;

    sys_state.logf(sys_state.userdata, "--- LOG ALLOCS %lu ---\n", alloc_count);

    for (size_t i = 0; i < allocinfo_count; ++i)
    {
        sys_state.logf(sys_state.userdata,
                       "category: \"%s\", address: %p total_size: %u payload_size: %u, alignment: %u, source: \"%s\"\n",
                       allocinfo_list[i].category,
                       allocinfo_list[i].header,
                       allocinfo_list[i].total_size,
                       allocinfo_list[i].payload_size,
                       allocinfo_list[i].alignment,
                       allocinfo_list[i].sourceline);
    }

    dealloc(allocinfo_list);
}


Mallocator::MemBlockHeader *Mallocator::get_header(void *ptr)
{
    char *charptr = (char *)ptr;
    u8 offset = *(u8 *)(charptr - 1);
    MemBlockHeader *result = (MemBlockHeader *)(charptr - offset);
    return result;
}


size_t Mallocator::payload_size_of(void *ptr) OVERRIDE
{
    MemBlockHeader *hdr = get_header(ptr);
    return hdr->payload_size();
}


bool ALLOC_STACKTRACE = false;
void *Mallocator::realloc(void *ptr, size_t size, size_t align, AllocationMetadata meta) OVERRIDE
{
    assert(!logging_allocations);
    assert(size > 0);
    assert(align < UINT8_MAX);

    if (ALLOC_STACKTRACE) {
        std::printf("Allocated bytecount before: %lu", bytecount);
    }

    u8 align_u8 = (u8)align;

    // align must be a power of two
    size_t align_mask = align - 1;

    // this expands the header_size to an alignment boundary
    size_t aligned_header_size = (HeaderSize + align_mask) & ~align_mask;

    size_t header_and_payload_size = aligned_header_size + size;

    size_t misalign = header_and_payload_size & align_mask;
    size_t alloc_size = header_and_payload_size + (align - misalign);

    char *memblock = (char *)std::malloc(alloc_size);

    bytecount += alloc_size;

    MemBlockHeader *preexisting_hdr = nullptr;
    if (ptr)
    {
        char *charptr = (char *)ptr;
        u8 preexisting_offset = *(u8 *)(charptr - 1);
        preexisting_hdr = (MemBlockHeader *)(charptr - preexisting_offset);

        // Remove this from alloclist
        if (preexisting_hdr->next)
        {
            preexisting_hdr->next->prev = preexisting_hdr->prev;
        }
        if (preexisting_hdr->prev)
        {
            preexisting_hdr->prev->next = preexisting_hdr->next;
        }
        if (preexisting_hdr == alloclist_head)
        {
            alloclist_head = preexisting_hdr->next;
        }
    }


    MemBlockHeader *hdr = (MemBlockHeader *)memblock;
    hdr->total_size = alloc_size;
    hdr->alignment = align_u8;
    size_t offset = aligned_header_size + (align - misalign);
    assert(offset < UINT8_MAX);
    hdr->payload_offset = (u8)offset;

    // Set metadata
    hdr->sourceline = meta.sourceline;
    hdr->category = meta.category;

    // Update allocation list
    hdr->next = alloclist_head;
    hdr->prev = nullptr;
    if (alloclist_head)
    {
        alloclist_head->prev = hdr;
    }
    alloclist_head = hdr;

    u8 *result = (u8 *)hdr->payload();
    *((u8 *)(memblock + (hdr->payload_offset - 1))) = hdr->payload_offset;

    size_t preexisting_payload_size = 0;
    if (preexisting_hdr)
    {
        preexisting_payload_size = preexisting_hdr->payload_size();
        size_t copy_size = std::min(preexisting_hdr->payload_size(), hdr->payload_size());
        std::memcpy(result, ptr, copy_size);
        bytecount -= preexisting_hdr->total_size;
        assert(preexisting_hdr != alloclist_head);
        std::free(preexisting_hdr);
        assert(!alloclist_contains(preexisting_hdr));
    }

    validate_alloclist();

    if (!preexisting_hdr)
    {
        ++alloc_count;
    }

    if (ALLOC_STACKTRACE) {
        size_t requested = size - preexisting_payload_size;
        std::printf(", after: %lu, requested: %lu, total: %lu\n", bytecount, requested, alloc_size);
        print_stacktrace();
    }


    return result;
}


void Mallocator::dealloc(void *ptr) OVERRIDE
{
    if (ALLOC_STACKTRACE) {
        std::printf("Allocated bytecount before: %lu", bytecount);
    }

    ASSERT(!logging_allocations);

    if (!ptr) return;

    MemBlockHeader *hdr = get_header(ptr);

    size_t freed = hdr->total_size;

    bytecount -= hdr->total_size;
    if (hdr->next)
    {
        hdr->next->prev = hdr->prev;
    }
    if (hdr->prev)
    {
        hdr->prev->next = hdr->next;
    }
    if (hdr == alloclist_head)
    {
        alloclist_head = hdr->next;
    }
    std::free(hdr);

    assert(!alloclist_contains(hdr));

    --alloc_count;

    if (ALLOC_STACKTRACE) {
        std::printf(", after: %lu, freed: %lu\n", bytecount, freed);
        print_stacktrace();
    }

    validate_alloclist();
}


void Mallocator::validate_alloclist()
{
    MemBlockHeader *header = alloclist_head;
    MemBlockHeader *prev_header = 0;

    assert(!header || header->prev == 0);

    size_t i = 0;
    while (header)
    {
        assert(!(header->prev == 0 && header->next == 0) || alloc_count <= 1);

        assert(i <= alloc_count);
        ++i;
        prev_header = header;
        header = header->next;
    }

    assert(!prev_header || prev_header->next == 0);
}


bool Mallocator::alloclist_contains(void *ptr)
{
    MemBlockHeader *header = alloclist_head;

    size_t i = 0;
    while (header)
    {
        if (header == ptr)
        {
            return true;
        }

        assert(i <= alloc_count);
        ++i;
        header = header->next;
    }

    return false;
}


size_t Mallocator::bytes_allocated() OVERRIDE
{
    return bytecount;
}


static char mallocator_storage[sizeof(Mallocator)];
static Mallocator *mallocator_inst = 0;


IAllocator *default_allocator()
{
    if (!mallocator_inst)
    {
        mallocator_inst = new (mallocator_storage) Mallocator();
    }
    return mallocator_inst;
}


}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
