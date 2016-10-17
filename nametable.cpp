#include "nametable.h"

ptrdiff_t allocated_size(StrLen len)
{
    size_t needed_size = len + 1 + sizeof(StrLen);
    size_t result_size = sizeof(StrLen) * ((needed_size + sizeof(StrLen) - 1) /  sizeof(StrLen));
    assert((result_size % sizeof(StrLen)) == 0);
    ptrdiff_t result = PTRDIFF_T(result_size);
    return result;
}


bool nameref_identical(const NameRef &lhs, const NameRef &rhs)
{
    return lhs.offset == rhs.offset
        && lhs.table == rhs.table;
}


StrSlice str_slice(NameRef nameref)
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
        result.length = *reinterpret_cast<StrLen *>(storage + nameref.offset);
        result.data = (static_cast<char *>(storage) + offset + sizeof(StrLen));
    }

    return result;
}


namespace nametable
{

static const size_t InitialStorageOffset = 8;

void init(NameTable *nt, size_t storage_size, mem::IAllocator *allocator)
{
    // There should be 8 bytes of padding at the beginning of the
    // storage, so minimum storage size is 8. But, like, don't do that. It's weird.
    // The padding is so that ptrdiff_t == 0 can mean not-found.
    size_t capacity = storage_size < InitialStorageOffset ? InitialStorageOffset : storage_size;
    nt->storage_capacity = capacity;
    // nt->storage = CALLOC_ARRAY(char, capacity);

    if (!allocator)
    {
        allocator = mem::default_allocator();
    }

    nt->storage = MAKE_ZEROED_ARRAY(allocator, capacity, char);

    // had to use reinterpret_cast because -Wcast-align
    *reinterpret_cast<u64 *>(nt->storage) = 0xdeadbeefdeadbeef;

    nt->next_storage_offset = InitialStorageOffset;
    ht_init(&nt->lookup, 29, allocator);
}


NameRef first(NameTable *nt)
{
    NameRef result;
    result.table = nt;
    result.offset = InitialStorageOffset;
    return result;
}


NameRef next(NameRef nameref)
{
    StrLen namelen = *reinterpret_cast<StrLen *>(nameref.table->storage + nameref.offset);
    NameRef result;
    result.table = nameref.table;
    ptrdiff_t new_offset = (nameref.offset + allocated_size(namelen));
    result.offset = (new_offset < PTRDIFF_T(nameref.table->storage_capacity))
        ? new_offset
        : 0;
    return result;
}


NameRef find(NameTable *nt, StrSlice name)
{
    NameRef result;
    result.table = nt;
    ptrdiff_t *offset = ht_find(&nt->lookup, name);
    result.offset = offset ? *offset : 0;
    return result;
}


NameRef find_or_add(NameTable *nt, StrSlice name)
{
    NameRef result = find(nt, name);
    if (result.offset)
    {
        return result;
    }

    // calculated total size to allocate, includes room for length at
    // front, chars, null terminator, and pads to align with StrLen
    ptrdiff_t alloc_size = allocated_size(name.length);

    assert(PTRDIFF_T(nt->storage_capacity) - nt->next_storage_offset > alloc_size);
    // ASSERT(nt->storage_capacity - (size_t)nt->next_storage_offset > size);

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

    // bump next_storage_offset by the calculated alloc_size
    nt->next_storage_offset += alloc_size;

    ht_set(&nt->lookup, name, result.offset);

    return result;
}

NameRef find_or_add(NameTable *nt, const char *name)
{
    return find_or_add(nt, str_slice(name));
}

}
