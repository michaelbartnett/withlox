// -*- c++ -*-

#ifndef DYNARRAY_H

#include "common.h"
#include "numeric_types.h"
#include "memory.h"
#include <cassert>
#include <algorithm>

typedef u32 DynArrayCount;
#define DYNARRAY_COUNT_MAX UINT32_MAX


template<typename TInteger>
DynArrayCount DYNARRAY_COUNT(TInteger i)
{
    assert(i < DYNARRAY_COUNT_MAX);
    return (DynArrayCount)i;
}


template<typename T>
struct DynArray
{
    T *data;
    DynArrayCount count;
    DynArrayCount capacity;
    mem::IAllocator *allocator;

    T &operator[](DynArrayCount index) const
    {
        return this->data[index];
    }
};


template<typename T>
void dynarray_init(DynArray<T> *dynarr, DynArrayCount capacity,
                   mem::IAllocator *allocator = nullptr)
{
    if (!allocator)
    {
        allocator = mem::default_allocator();
    }
    dynarr->allocator = allocator;
    dynarr->data = capacity > 0 ? MAKE_ARRAY(T, capacity, dynarr->allocator) : 0;
    dynarr->count = 0;
    dynarr->capacity = capacity;
}


template<typename T>
DynArray<T> dynarray_init(DynArrayCount capacity)
{
    DynArray<T> result;
    dynarray_init(&result, capacity);
    return result;
}


template<typename T>
void dynarray_deinit(DynArray<T> *dynarray)
{
    dynarray->allocator->dealloc(dynarray->data);
    dynarray->data = 0;
    dynarray->count = 0;
    dynarray->capacity = 0;
}

 

template<typename T>
T *dynarray_append(DynArray<T> *dynarray)
{
    // assert(dynarray->data);
    assert(dynarray->count <= dynarray->capacity);

    if (dynarray->count == dynarray->capacity)
    {
        DynArrayCount new_capacity = (dynarray->capacity + 1) * 2;
        RESIZE_ARRAY(dynarray->data, T, new_capacity, dynarray->allocator);
        dynarray->capacity = new_capacity;
    }

    ++dynarray->count;

    return dynarray_last(dynarray);
}


template<typename T>
T *dynarray_append(DynArray<T> *dynarray, T item)
{
    T *result = dynarray_append(dynarray);
    *result = item;
    return result;
}


template<typename T>
T *dynarray_last(DynArray<T> *dynarray)
{
    return dynarray->data + (dynarray->count - 1);
}


template<typename T>
void dynarray_set(DynArray<T> *dynarray, u32 index, T value)
{
    assert(dynarray->count > index);
    dynarray->data[index] = value;
}


template<typename T>
u32 dynarray_pop(const DynArray<T> *dynarray)
{
    if (dynarray->count > 0)
    {
        --dynarray->count;
    }
    return dynarray->count;
}


template<typename T>
void dynarray_clear(const DynArray<T> *dynarray)
{
    dynarray->count = 0;
}


template<typename T>
void dynarray_copy(const DynArray<T> *src, DynArray<T> *dest, DynArrayCount start_index, DynArrayCount count)
{
    assert(count <= src->count - start_index);
    assert(count <= dest->capacity - start_index);
    DynArrayCount idx = start_index;
    for (DynArrayCount i = 0; i < count; ++i)
    {
        dest->data[idx] = src->data[idx];
        ++idx;
    }
    dest->count += count - (dest->count - start_index);
}


template<typename T>
void dynarray_copy_at(const DynArray<T> *src, DynArray<T> *dest, DynArrayCount start_index)
{
    DynArrayCount count = (src->count < dest->capacity ? src->count : dest->capacity) - start_index;
    dynarray_copy(src, dest, start_index, count);
}


template<typename T>
void dynarray_copy(const DynArray<T> *src, DynArray<T> *dest)
{
    assert(src->count <= dest->capacity);
    dynarray_copy_at(src, dest, 0);
}


template<typename T>
void dynarray_copy_count(const DynArray<T> *src, DynArray<T> *dest, DynArrayCount count)
{
    assert(count <= src->count);
    assert(count <= dest->capacity);
    dynarray_copy(src, dest, 0, count);
}


template<typename T>
DynArray<T> dynarray_clone(const DynArray<T> *src)
{
    DynArray<T> result;
    dynarray_init(&result, src->count);
    dynarray_copy(src, &result);
    return result;
}


template<typename Compare, typename T>
void dynarray_sort_unstable(const DynArray<T> *dynarray)
{
    Compare comp;
    std::sort(dynarray->data,
              dynarray->data + dynarray->count,
              comp);
}


#define DYNARRAY_H
#endif
