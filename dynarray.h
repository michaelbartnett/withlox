// -*- c++ -*-

#ifndef DYNARRAY_H

#include "common.h"
#include "numeric_types.h"
#include <cassert>

typedef u32 DynArrayCount;

template<typename T>
struct DynArray
{
    typedef u32 count_type;

    T *data;
    count_type count;
    count_type capacity;    
};


template<typename T>
void dynarray_init(DynArray<T> *dynarr, DynArrayCount capacity)
{
    dynarr->data = MALLOC_ARRAY(T, capacity);
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
    assert(dynarray->data);
    assert(dynarray->count <= dynarray->capacity);

    if (dynarray->count == dynarray->capacity)
    {
        DynArrayCount new_capacity = dynarray->capacity * 2;
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
T *get(const DynArray<T> &dynarray, DynArrayCount index)
{
    return &dynarray.data[index];
}

#define DYNARRAY_H
#endif
