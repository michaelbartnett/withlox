// -*- c++ -*-
#ifndef NAMETABLE_H

#include "str.h"
#include "hashtable.h"


struct NameTable
{
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

NameRef nametable_find(NameTable *nt, const char *name);

NameRef nametable_find_or_add(NameTable *nt, StrSlice name);

NameRef nametable_find_or_add(NameTable *nt, const char *name);

inline NameRef nametable_find_or_add(NameTable *nt, const char *name, size_t length)
{
    assert(length < UINT16_MAX);
    StrSlice nameslice = str_slice(name, (u16)length);
    return nametable_find_or_add(nt, nameslice);
}

#define NAMETABLE_H
#endif
