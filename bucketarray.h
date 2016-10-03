// -*- c++ -*-
#ifndef BUCKETARRAY_H

#include "common.h"
#include "numeric_types.h"
#include "memory.h"

#include "platform.h" // uggggghhhhh


/*
TODO(mike): unit test with doctest or something

    const int test_amount = 25;

    BucketArray<TypeDescriptor, 9> typedesc_bucket;
    bucketarray_init(&typedesc_bucket);
    u32 typeids = 0;
    for (int i = 0; i < test_amount; ++i)
    {
        TypeDescriptor tdtest = {};
        tdtest.type_id = typeids;
        ++typeids;
        *bucketarray_add(&typedesc_bucket).elem = tdtest;
    }

    for (int i = 0; i < test_amount; ++i)
    {
        if (i % 2 == 0)
        {
            bucketarray_remove(&typedesc_bucket, i);
        }
    }
    {
        TypeDescriptor tdtest = {};
        tdtest.type_id = typeids;
        *bucketarray_add(&typedesc_bucket).elem = tdtest;
    }

    DynArray<u32> test;
    dynarray_init(&test, test_amount);
    test.count = test_amount;
    for (int i = test_amount - 1; i >= 0; --i)
    {
        TypeDescriptor *td;
        if (bucketarray_get_if_not_empty(&typedesc_bucket, i, &td))
        {
            test[DYNARRAY_COUNT(i)] = td->type_id;
        }
    }

    for (DynArrayCount i = 0, e = test.count; i < e; ++i)
    {
        logf_ln("%i: %u", i, test[i]);
    }

    for (int i = 0; i < test_amount; ++i)
    {
        TypeDescriptor *td = &typedesc_bucket[i];
        BucketIndex idx = bucketarray_bucketindex_of_ptr(&typedesc_bucket, td);
        s32 indexified = bucketarray_indexify(&typedesc_bucket, idx);
        ASSERT(i == indexified);
    }

    return 0;

 */


template<typename T, size_t ItemCount>
struct Bucket
{
    u8 occupied[ItemCount];
    T items[ItemCount];
    size_t count;
    DynArrayCount index;
};


struct BucketIndex
{
    s32 bucket_index;
    s32 item_index;
};


template <typename T>
struct IndexElemPair
{
    s32 index;
    T *elem;
};


template <typename T, size_t ItemCount = 64>
struct BucketArray
{
    DynArray<Bucket<T, ItemCount> *> all_buckets;
    DynArray<Bucket<T, ItemCount> *> vacancy_buckets;
    size_t count;

    mem::IAllocator *allocator;

    T &operator[](BucketIndex bidx) const
    {
        ASSERT(bidx.bucket_index < all_buckets.count);
        return all_buckets[bidx.bucket_index].items[bidx.item_index];
    }

    T &operator[](s32 index) const
    {
        return at(index);
    }

    T &at(s32 index) const;
};



template<typename T, size_t ItemCount>
BucketIndex bucketarray_translate_index(const BucketArray<T, ItemCount> *ba, s32 index)
{
    ASSERT(index >= 0);
    BucketIndex result;
    DynArrayCount dacidx = DYNARRAY_COUNT(index);
    result.bucket_index = dacidx / ItemCount;
    ASSERT(DYNARRAY_COUNT(result.bucket_index) < ba->all_buckets.count);
    result.item_index = dacidx % ItemCount;
    ASSERT(DYNARRAY_COUNT(result.bucket_index) <= ItemCount);
    return result;
}


template<typename T, size_t ItemCount>
s32 bucketarray_indexify(const BucketArray<T, ItemCount> *ba, BucketIndex bucket_idx)
{
    UNUSED(ba);
    return bucket_idx.bucket_index * S32(ItemCount) + bucket_idx.item_index;
}


template <typename T, size_t ItemCount>
bool bucketarray_get_if_not_empty(const BucketArray<T, ItemCount> *ba, s32 index, T **output)
{
    BucketIndex idx = bucketarray_translate_index(ba, index);
    Bucket<T, ItemCount> *b = ba->all_buckets[DYNARRAY_COUNT(idx.bucket_index)];

    bool found = b->occupied[idx.item_index];
    *output = found ? &b->items[idx.item_index] : nullptr;
    return found;
}


template <typename T, size_t ItemCount>
T &BucketArray<T, ItemCount>::at(s32 index) const
{
    BucketIndex idx = bucketarray_translate_index(this, index);
    Bucket<T, ItemCount> *b = all_buckets[DYNARRAY_COUNT(idx.bucket_index)];
    return b->items[idx.item_index];
}


template<typename T, size_t ItemCount>
BucketIndex bucketarray_bucketindex_of_ptr(BucketArray<T, ItemCount> *ba, T *elem)
{
    BucketIndex result;
    result.bucket_index = -1;
    result.item_index = -1;

    intptr_t ptr = reinterpret_cast<intptr_t>(elem);
    for (s32 i = 0, e = S32(ba->all_buckets.count); i < e; ++i)
    {
        intptr_t bucket_items = reinterpret_cast<intptr_t>(ba->all_buckets[DYNARRAY_COUNT(i)]->items);
        intptr_t past_bucketitems_end = bucket_items + static_cast<intptr_t>(ItemCount * sizeof(T));
        if ((ptr >= bucket_items) && (ptr < past_bucketitems_end))
        {
            intptr_t diff = ptr - bucket_items;
            result.bucket_index = i;
            intptr_t itemidx = diff / INTPTR_T(sizeof(T));
            result.item_index = S32(itemidx);
            break;
        }
    }

    return result;
}


template<typename T, size_t ItemCount>
void bucketarray_init(BucketArray<T, ItemCount> *ba,
                      mem::IAllocator *allocator = nullptr,
                      DynArrayCount    initial_bucket_capacity = 0)
{
    if (!allocator)
    {
        allocator = mem::default_allocator();
    }

    ba->allocator = allocator;
    dynarray_init(&ba->all_buckets, initial_bucket_capacity, allocator);
    dynarray_init(&ba->vacancy_buckets, initial_bucket_capacity, allocator);
}


template<typename T, size_t ItemCount>
Bucket<T, ItemCount> *bucketarray_addbucket(BucketArray<T, ItemCount> *ba)
{
    Bucket<T, ItemCount> *bucket = MAKE_OBJ(ba->allocator, Bucket<T, ItemCount>);
    bucket->index = ba->all_buckets.count;
    bucket->count = 0;
    zero_obj(bucket->occupied);
    dynarray_append(&ba->all_buckets, bucket);
    return *dynarray_append(&ba->vacancy_buckets, bucket);
}


template<typename T, size_t ItemCount>
IndexElemPair<T> bucketarray_add(BucketArray<T, ItemCount> *ba)
{
    DynArrayCount vacant_bucket_count = ba->vacancy_buckets.count;
    Bucket<T, ItemCount> *bucket;

    if (vacant_bucket_count > 0)
    {
        bucket = ba->vacancy_buckets[vacant_bucket_count - 1];
    }
    else
    {
        bucket = bucketarray_addbucket(ba);
    }

    ASSERT(bucket);
    ASSERT(bucket->count < ItemCount);

    s32 selected_index = -1;
    for (s32 i = 0, e = ItemCount; i < e; ++i)
    {
        if (!bucket->occupied[i])
        {
            bucket->occupied[i] = true;
            selected_index = i;
            break;
        }
    }

    ASSERT(selected_index >= 0);

    ++bucket->count;

    if (bucket->count == ItemCount)
    {
        dynarray_swappop(&ba->vacancy_buckets, vacant_bucket_count - 1);
    }

    ++ba->count;

    IndexElemPair<T> result = {S32(bucket->index) + selected_index, &bucket->items[selected_index]};
    return result;
}


template<typename T, size_t ItemCount>
void bucketarray_remove(BucketArray<T, ItemCount> *ba, s32 index)
{
    BucketIndex idx = bucketarray_translate_index(ba, index);

    Bucket<T, ItemCount> *b = ba->all_buckets[DynArrayCount(idx.bucket_index)];
    b->occupied[idx.item_index] = false;
    --b->count;
    --ba->count;
}


#define BUCKETARRAY_H
#endif
