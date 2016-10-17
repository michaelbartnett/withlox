// -*- c++ -*-
#ifndef NAMETABLE_H

#include "str.h"
#include "hashtable.h"
#include "memory.h"


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


struct NameRefIdentical
{
    bool operator()(const NameRef &a, const NameRef &b)
    {
        return nameref_identical(a, b);
    }
};



inline bool hashtable_keys_equal(const NameRef &lhs, const NameRef &rhs)
{
    return nameref_identical(lhs, rhs);
}


StrSlice str_slice(NameRef nameref);

inline Str str(NameRef nameref)
{
    return str(str_slice(nameref));
}

namespace nametable
{

void init(NameTable *nt, size_t storage_size, mem::IAllocator *allocator = nullptr);

NameRef find(NameTable *nt, StrSlice name);

NameRef find(NameTable *nt, const char *name);

NameRef find_or_add(NameTable *nt, StrSlice name);

NameRef find_or_add(NameTable *nt, const char *name);

NameRef first(NameTable *nt);

NameRef next(NameRef nameref);


inline NameRef find_or_add(NameTable *nt, const char *name, size_t length)
{
    StrSlice nameslice = str_slice(name, STRLEN(length));
    return find_or_add(nt, nameslice);
}

}

#define NAMETABLE_H
#endif
