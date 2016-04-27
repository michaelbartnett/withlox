#include "nametable.h"


bool nameref_identical(const NameRef &lhs, const NameRef &rhs)
{
    return lhs.offset == rhs.offset
        && lhs.table == rhs.table;
}


StrSlice str_slice(const NameRef &nameref)
{
    StrSlice result;
    assert((size_t)nameref.offset < nameref.table->storage_capacity);
    assert((size_t)nameref.offset % sizeof(StrLen) == 0);

    if (!nameref.offset || !nameref.table)
    {
        result = empty_str_slice();
    }
    else
    {
        ptrdiff_t offset = nameref.offset;
        char *storage = nameref.table->storage;
        result.length = (StrLen)*(storage + nameref.offset);
        result.data = ((char *)storage + offset + sizeof(StrLen));
    }

    return result;
}


void nametable_init(NameTable *nt, size_t storage_size, mem::IAllocator *allocator)
{
    // There should be 8 bytes of padding at the beginning of the
    // storage, so minimum storage size is 8. But, like, don't do that. It's weird.
    // The padding is so that ptrdiff_t == 0 can mean not-found.
    size_t capacity = storage_size < 8 ? 8 : storage_size;
    nt->storage_capacity = capacity;
    // nt->storage = CALLOC_ARRAY(char, capacity);

    if (!allocator)
    {
        allocator = mem::default_allocator();
    }

    nt->storage = MAKE_ZEROED_ARRAY(char, capacity, allocator);

    // had to use reinterpret_cast because -Wcast-align
    *reinterpret_cast<u64 *>(nt->storage) = 0xdeadbeefdeadbeef;

    nt->next_storage_offset = 8;
    ht_init(&nt->lookup, 29, allocator);
}


NameRef nametable_find(NameTable *nt, StrSlice name)
{
    NameRef result;
    result.table = nt;
    ptrdiff_t *offset = ht_find(&nt->lookup, name);
    result.offset = offset ? *offset : 0;
    return result;
}

// NameRef nametable_find(NameTable *nt, const char *name)
// {
//     return nametable_find(nt, str_slice(name));
// }


NameRef nametable_find_or_add(NameTable *nt, StrSlice name)
{
    NameRef result = nametable_find(nt, name);
    if (result.offset)
    {
        return result;
    }

    size_t size = name.length + 1 + sizeof(StrLen);
    assert(nt->storage_capacity - (size_t)nt->next_storage_offset > size);

    // Nameref stores offset into nametable storage.
    // The memory starts with a length of size StrLen (u16 at the time this was written)
    // After the StrLen, the null-terminated character string folows
    // The offset is always aligned with StrLen
    result.offset = nt->next_storage_offset;
    char *storage_location = nt->storage + nt->next_storage_offset;

    StrLen *length_storage = reinterpret_cast<StrLen *>(storage_location);
    *length_storage = name.length;

    char *str_storage = storage_location + sizeof(StrLen);
    std::strncpy(str_storage, name.data, name.length + 1);
    
    StrSlice key;
    key.length = name.length;
    key.data = str_storage;
    
    // set next_storage_offset to past end of null terminator, and add
    // padding if not aligned with the size of StrLen
    size_t size_modulo = size % sizeof(StrLen);
    size_t align_padding = size_modulo == 0 ? 0 : sizeof(StrLen) - size_modulo;
    nt->next_storage_offset += size + align_padding;

    ht_set(&nt->lookup, name, result.offset);

    return result;
}

NameRef nametable_find_or_add(NameTable *nt, const char *name)
{
    return nametable_find_or_add(nt, str_slice(name));
}
