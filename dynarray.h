// -*- c++ -*-

#ifndef DYNARRAY_H

#include "common.h"
#include "numeric_types.h"
#include <cassert>

typedef u32 DynArrayCount;
#define DYNARRAY_COUNT_MAX UINT32_MAX

template<typename T>
struct DynArray
{

    T *data;
    DynArrayCount count;
    DynArrayCount capacity;    
};


template<typename T>
void dynarray_init(DynArray<T> *dynarr, DynArrayCount capacity)
{
    dynarr->data = capacity > 0 ? MALLOC_ARRAY(T, capacity) : 0;
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
    free(dynarray->data);
    dynarray->data = 0;
    dynarray->count = 0;
    dynarray->capacity = 0;
}

 

template<typename T>
T *append(DynArray<T> *dynarray)
{
    // assert(dynarray->data);
    assert(dynarray->count <= dynarray->capacity);

    if (dynarray->count == dynarray->capacity)
    {
        DynArrayCount new_capacity = (dynarray->capacity + 1) * 2;
        dynarray->data = REALLOC_ARRAY(dynarray->data, T, new_capacity);
        dynarray->capacity = new_capacity;
    }

    ++dynarray->count;

    return last(dynarray);
}


template<typename T>
T *append(DynArray<T> *dynarray, T item)
{
    T *result = append(dynarray);
    *result = item;
    return result;
}

template<typename T>
T *last(DynArray<T> *dynarray)
{
    return dynarray->data + (dynarray->count - 1);
}

template<typename T>
T *get(const DynArray<T> &dynarray, DynArrayCount index)
{
    return &dynarray.data[index];
}

template<typename T>
T *get(const DynArray<T> *dynarray, DynArrayCount index)
{
    return &dynarray->data[index];
}

template<typename T>
void set(DynArray<T> *dynarray, u32 index, T value)
{
    assert(dynarray->count > index);
    dynarray->data[index] = value;
}

template<typename T>
u32 pop(const DynArray<T> dynarray)
{
    if (dynarray->count > 0)
    {
        --dynarray->count;
    }
    return dynarray->count;
}

template<typename T>
void clear(const DynArray<T> dynarray)
{
    dynarray->count = 0;
}
#define DYNARRAY_H
#endif
