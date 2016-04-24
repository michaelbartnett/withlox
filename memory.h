#ifndef MEMORY_H

// -*- c++ -*-
#include <cstddef>

#define MAKE_OBJ(type, allocator) (type *)allocator->realloc(0, sizeof (type), 4)
#define MAKE_ARRAY(type, count, allocator) (type *)allocator->realloc(0, count * sizeof (type), 4)
// #define RESIZE_ARRAY(ptr, count, allocator) ( *)allocator->realloc(ptr, count * sizeof (*ptr), 4)
#define RESIZE_ARRAY(ptr, type, count, allocator) ptr = (type *)allocator->realloc(ptr, count * sizeof (type), 4)


namespace mem
{

using std::size_t;

class IAllocator
{
public:
    virtual void *realloc(void *ptr, size_t size, size_t align) = 0;
    virtual void dealloc(void *ptr) = 0;
    virtual size_t bytes_allocated() = 0;
    virtual size_t payload_size_of(void *ptr) = 0;
    virtual void log_allocations() = 0;
    virtual ~IAllocator() {}
};

IAllocator *default_allocator();
void log_memcalls();
IAllocator *make_mallocator();
void *get_mallocator_header(IAllocator *allocator, void *ptr);
}


#define MEMORY_H
#endif
