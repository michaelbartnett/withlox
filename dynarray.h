// -*- c++ -*-

#ifndef DYNARRAY_H

#include "platform.h"
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
    ASSERT(i >= 0);
    ASSERT((DynArrayCount)(i) < DYNARRAY_COUNT_MAX);
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
        return at(index);
    }

    T &at(DynArrayCount index) const
    {
        ASSERT(index < count);
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
    dynarr->data = capacity > 0 ? MAKE_ARRAY(dynarr->allocator, capacity, T) : 0;
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
    if (dynarray->capacity)
    {
        dynarray->allocator->dealloc(dynarray->data);
        dynarray->data = 0;
        dynarray->count = 0;
        dynarray->capacity = 0;
    }
    else
    {
        assert(dynarray->data == 0);
        assert(dynarray->count == 0);
        assert(dynarray->capacity == 0);
    }
}


template<typename T>
void dynarray_ensure_capacity(DynArray<T> *dynarray, DynArrayCount min_capacity)
{
    if (dynarray->capacity < min_capacity)
    {
        dynarray->capacity = min_capacity;
        RESIZE_ARRAY(dynarray->allocator, dynarray->data, dynarray->capacity, T);
    }
}


template<typename T>
T *dynarray_append(DynArray<T> *dynarray)
{
    if (! dynarray->allocator)
    {
        dynarray->allocator = mem::default_allocator();
    }

    assert(dynarray->count <= dynarray->capacity);

    if (dynarray->count == dynarray->capacity)
    {
        DynArrayCount new_capacity = (dynarray->capacity + 1) * 2;
        RESIZE_ARRAY(dynarray->allocator, dynarray->data, new_capacity, T);
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
void dynarray_pop(DynArray<T> *dynarray)
{
    if (dynarray->count > 0)
    {
        --dynarray->count;
    }
    // return dynarray->count;
}


template <typename T>
void dynarray_popnum(DynArray<T> *dynarray, DynArrayCount num)
{
    dynarray->count -= min(num, dynarray->count);
    // return dynarray->count;
}


template <typename T>
void dynarray_swappop(DynArray<T> *da, DynArrayCount idx)
{
    if (da->count > 0)
    {
        da->data[idx] = da->data[da->count - 1];
    }
    --da->count;
}


template<typename T>
void dynarray_clear(DynArray<T> *dynarray)
{
    dynarray->count = 0;
}


template<typename T>
void dynarray_copy(DynArray<T> *dest, DynArrayCount dest_start, const DynArray<T> *src, DynArrayCount src_start, DynArrayCount count)
{
    assert(src_start + count <= src->count);

    DynArrayCount final_count = max<DynArrayCount>(dest_start + count, dest->count);
    assert(final_count > dest_start || count == 0);

    DynArrayCount capacity_required = dest_start + count;
    dynarray_ensure_capacity(dest, capacity_required);
    std::memcpy(dest + dest_start, src + src_start, count * sizeof(T));
    dest->count = final_count;
}


template<typename T>
void dynarray_copy(DynArray<T> *dest, const DynArray<T> *src)
{
    assert(src->count <= dest->capacity);
    dynarray_copy(dest, 0, src, 0, dest->count);
}


template<typename T>
DynArray<T> dynarray_clone(const DynArray<T> *src)
{
    DynArray<T> result;
    dynarray_init(&result, src->count);
    dynarray_copy(&result, src);
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
