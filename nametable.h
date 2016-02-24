// -*- c++ -*-
#ifndef NAMETABLE_H

#include "str.h"
#include "hashtable.h"


struct NameTable
{
    struct StrSliceHash
    {
        u32 operator()(const StrSlice &slice)
        {
            const u32 seed = 541;
            u32 result;
            MurmurHash3_x86_32(slice.data, slice.length, seed, &result);
            return result;
        }
    };

    struct StrSliceEqual
    {
        bool operator()(const StrSlice &lhs, const StrSlice &rhs)
        {
            return str_equal(lhs, rhs);
        }
    };


    // This will own the memory for strings
    // And a hashtable mapping keys back to an offset from that memory
    // That offset is the symbol, the handle.
    char *storage;
    size_t storage_capacity;
    ptrdiff_t next_storage_offset;

    OAHashtable<StrSlice, std::ptrdiff_t, StrSliceEqual, StrSliceHash> lookup;
};


struct NameRef
{
    ptrdiff_t offset;
    NameTable *table;
};

bool nameref_identical(const NameRef &lhs, const NameRef &rhs);


inline bool hashtable_keys_equal(const NameRef &lhs, const NameRef &rhs)
{
    return nameref_identical(lhs, rhs);
}


StrSlice str_slice(const NameRef &nameref);

void nametable_init(NameTable *nt, size_t storage_size);

NameRef nametable_find(NameTable *nt, StrSlice name);

NameRef nametable_find_or_add(NameTable *nt, StrSlice name);

NameRef nametable_find_or_add(NameTable *nt, const char *name);

#define NAMETABLE_H
#endif
