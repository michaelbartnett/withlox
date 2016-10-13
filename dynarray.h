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

    T &operator[](s32 index) const
    {
        return at(DYNARRAY_COUNT(index));
    }

    T &at(DynArrayCount index) const
    {
        ASSERT(index < count);
        return this->data[index];
    }
};


namespace dynarray
{

template<typename T>
void init(DynArray<T> *dynarr, DynArrayCount capacity,
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
DynArray<T> init(DynArrayCount capacity)
{
    DynArray<T> result;
    init(&result, capacity);
    return result;
}


template<typename T>
void deinit(DynArray<T> *dynarray)
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


template <typename T>
bool in_range(DynArray<T> *da, DynArrayCount idx)
{
    return idx < da->count;
}


template <typename T>
bool contains(DynArray<T> *da, T *ptr)
{
    uintptr_t pointer = static_cast<uintptr_t>(ptr);
    uintptr_t data    = static_cast<uintptr_t>(da->data);
    uintptr_t end     = static_cast<uintptr_t>(da->data + da->count);
    return pointer >= data && pointer < end;
}


// template <typename T>
// bool try_index_of(OUTPARAM DynArrayCount *index, DynArray<T> *da, T *ptr)
// {
//     if (contains(da, ptr)) {
//         *index = index_of(da, ptr);
//         return true;
//     }

//     return false;
// }


// template <typename T>
// DynArrayCount index_of(DynArray<T> *da, T *ptr)
// {
//     ASSERT(contains(da, ptr));
//     return static_cast<DynArrayCount>(ptr - da->data);
// }


template<typename T>
bool try_find_index(OUTPARAM DynArrayCount *index, const DynArray<T> *da, T value)
{
    for (DynArrayCount i = 0, e = da->count; i < e; ++i)
    {
        if ((*da)[i] == value)
        {
            *index = i;
            return true;
        }
    }

    *index = DYNARRAY_COUNT_MAX;
    return false;
}


template<typename T>
T *find(const DynArray<T> *da, T value)
{
    DynArrayCount idx;
    if (try_find_index(&idx, da, value))
    {
        return &da->at(idx);
    }

    return nullptr;
}



template<typename T>
void ensure_capacity(DynArray<T> *dynarray, DynArrayCount min_capacity)
{
    if (dynarray->capacity < min_capacity)
    {
        dynarray->capacity = min_capacity;
        RESIZE_ARRAY(dynarray->allocator, dynarray->data, dynarray->capacity, T);
    }
}


template<typename T>
T *last(DynArray<T> *dynarray)
{
    return dynarray->data + (dynarray->count - 1);
}


template<typename T>
T *append(DynArray<T> *dynarray)
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

    return last(dynarray);
}


template<typename T>
T *append(DynArray<T> *dynarray, T item)
{
    T *result = dynarray::append(dynarray);
    *result = item;//T(item); // PODs only here
    return result;
}


template <typename T>
void swappop(DynArray<T> *da, DynArrayCount idx)
{
    if (da->count > 0)
    {
        da->data[idx] = da->data[da->count - 1];
    }
    --da->count;
}

template<typename T>
void set(DynArray<T> *dynarray, u32 index, T value)
{
    assert(dynarray->count > index);
    dynarray->data[index] = value;
}


template<typename T>
void pop(DynArray<T> *dynarray)
{
    if (dynarray->count > 0)
    {
        --dynarray->count;
    }
    // return dynarray->count;
}


template <typename T>
void popnum(DynArray<T> *dynarray, DynArrayCount num)
{
    dynarray->count -= min(num, dynarray->count);
    // return dynarray->count;
}


template<typename T>
void clear(DynArray<T> *dynarray)
{
    dynarray->count = 0;
}


template<typename T>
void copy(DynArray<T> *dest, DynArrayCount dest_start, const DynArray<T> *src, DynArrayCount src_start, DynArrayCount count)
{
    ASSERT(src_start + count <= src->count);

    DynArrayCount final_count = max<DynArrayCount>(dest_start + count, dest->count);
    ASSERT(final_count > dest_start || count == 0);

    DynArrayCount capacity_required = dest_start + count;
    dynarray::ensure_capacity(dest, capacity_required);
    std::memcpy(dest->data + dest_start, src->data + src_start, count * sizeof(T));
    dest->count = final_count;
}


template<typename T>
void copy(DynArray<T> *dest, const DynArray<T> *src)
{
    ASSERT(src->count <= dest->capacity);
    dynarray::copy(dest, 0, src, 0, src->count);
}


template<typename T>
void append_from(DynArray<T> *dest, const DynArray<T> *src)
{
    dynarray::copy(dest, dest->count, src, 0, src->count);
}


template<typename T>
DynArray<T> clone(const DynArray<T> *src)
{
    DynArray<T> result;
    dynarray::init(&result, src->count);
    dynarray::copy(&result, src);
    return result;
}


template<typename Compare, typename T>
void sort_unstable(const DynArray<T> *dynarray, const Compare &comp)
{
    std::sort(dynarray->data,
              dynarray->data + dynarray->count,
              comp);
}

}

#define DYNARRAY_H
#endif
