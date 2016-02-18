// -*- c++ -*-

#ifndef DYNARRAY_H

#include "common.h"
#include "numeric_types.h"


template<typename T>
struct DynArray
{
    T *data;
    u32 count;
    u32 capacity;    
};


template<typename T>
DynArray<T> dynarray_alloc(u32 capacity)
{
    DynArray<T> result;
    result.data = MALLOC_ARRAY(T, capacity);
    result.count = 0;
    result.capacity = capacity;
    return result;
}


template<typename T>
T *append(DynArray<T> *dynarray)
{
    assert(dynarray->data);
    assert(dynarray->count <= dynarray->capacity);

    if (dynarray->count == dynarray->capacity)
    {
        u32 new_capacity = dynarray->capacity * 2;
        dynarray->data = REALLOC_ARRAY(dynarray->data, T, new_capacity);
        dynarray->capacity = new_capacity;
    }

    ++dynarray->count;

    return last(dynarray);
}


template<typename T>
void append(DynArray<T> *dynarray, T item)
{
    append(dynarray);
    dynarray->data[dynarray->count] = item;
}

template<typename T>
T *last(DynArray<T> *dynarray)
{
    return dynarray->data + (dynarray->count - 1);
}

template<typename T>
T *get(const DynArray<T> &dynarray, u32 index)
{
    return &dynarray.data[index];
}

#define DYNARRAY_H
#endif
