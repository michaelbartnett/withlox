// -*- c++ -*-
#ifndef BUCKETARRAY_H

#include "common.h"
#include "numeric_types.h"
#include "memory.h"
#include "dynarray.h"
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
    dynarray::init(&test, test_amount);
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


typedef u32 BucketItemCount;
#define BUCKETITEMCOUNT(n) U32(n)

template<typename T, BucketItemCount ItemCount>
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

    bool is_valid()
    {
        return bucket_index >= 0 && item_index >= 0;
    }
};


template <typename T>
struct IndexElemPair
{
    s32 index;
    T *elem;
};



template <typename T, BucketItemCount ItemCount = 64>
struct BucketArray
{
    DynArray<Bucket<T, ItemCount> *> all_buckets;
    DynArray<Bucket<T, ItemCount> *> vacancy_buckets;
    BucketItemCount count;
    BucketItemCount capacity;

    mem::IAllocator *allocator;

    T &operator[](BucketIndex bidx) const
    {
        ASSERT(BUCKETITEMCOUNT(bidx.bucket_index) < all_buckets.count);
        return all_buckets[bidx.bucket_index]->items[bidx.item_index];
    }

    T &operator[](BucketItemCount index) const
    {
        return at(index);
    }

    T &operator[](s32 index) const
    {
        return at(BUCKETITEMCOUNT(index));
    }

    T &at(BucketItemCount index) const;
};


namespace bucketarray
{

template<typename T, BucketItemCount ItemCount>
void init(BucketArray<T, ItemCount> *ba,
          mem::IAllocator *allocator = nullptr,
          DynArrayCount    initial_bucket_capacity = 0)
{
    if (!allocator)
    {
        allocator = mem::default_allocator();
    }

    ba->allocator = allocator;
    ba->count = 0;
    ba->capacity = 0;
    dynarray::init(&ba->all_buckets, initial_bucket_capacity, allocator);
    dynarray::init(&ba->vacancy_buckets, initial_bucket_capacity, allocator);
}


template<typename T, BucketItemCount ItemCount>
BucketIndex translate_index(const BucketArray<T, ItemCount> *ba, BucketItemCount index)
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


template<typename T, BucketItemCount ItemCount>
bool exists(OUTPARAM BucketIndex *out_bidx, BucketArray<T, ItemCount> *ba, BucketItemCount index)
{
    if (index >= ba->capacity)
    {
        return false;
    }

    BucketIndex bidx = translate_index(ba, index);
    *out_bidx = bidx;
    return ba->all_buckets[bidx.bucket_index]->occupied[bidx.item_index];
}


template<typename T, BucketItemCount ItemCount>
bool exists(BucketArray<T, ItemCount> *ba, BucketItemCount index)
{
    if (index >= ba->capacity)
    {
        return false;
    }

    BucketIndex bidx = translate_index(ba, index);
    return ba->all_buckets[bidx.bucket_index]->occupied[bidx.item_index];
}


template<typename T, BucketItemCount ItemCount>
bool exists(BucketIndex *out_bidx, BucketArray<T, ItemCount> *ba, s32 index)
{
    if (index < 0)
    {
        return false;
    }
    return exists(out_bidx, ba, BUCKETITEMCOUNT(index));
}


template<typename T, BucketItemCount ItemCount>
bool exists(BucketArray<T, ItemCount> *ba, s32 index)
{
    if (index < 0)
    {
        return false;
    }
    return exists(ba, BUCKETITEMCOUNT(index));
}


template<typename T, BucketItemCount ItemCount>
BucketIndex bucketindex_of(BucketArray<T, ItemCount> *ba, T *elem)
{
    BucketIndex result;
    result.bucket_index = -1;
    result.item_index = -1;

    intptr_t ptr = reinterpret_cast<intptr_t>(elem);
    for (BucketItemCount i = 0, e = u32(ba->all_buckets.count); i < e; ++i)
    {
        intptr_t bucket_items = reinterpret_cast<intptr_t>(ba->all_buckets[DYNARRAY_COUNT(i)]->items);
        intptr_t past_bucketitems_end = bucket_items + static_cast<intptr_t>(ItemCount * sizeof(T));
        if ((ptr >= bucket_items) && (ptr < past_bucketitems_end))
        {
            intptr_t diff = ptr - bucket_items;
            result.bucket_index = S32(i);
            intptr_t itemidx = diff / INTPTR_T(sizeof(T));
            result.item_index = S32(itemidx);
            break;
        }
    }

    return result;
}


template<typename T, BucketItemCount ItemCount>
Bucket<T, ItemCount> *addbucket(BucketArray<T, ItemCount> *ba)
{
    Bucket<T, ItemCount> *bucket = MAKE_OBJ(ba->allocator, Bucket<T, ItemCount>);
    bucket->index = ba->all_buckets.count;
    bucket->count = 0;
    ba->capacity += ItemCount;
    mem::zero_obj(bucket->occupied);
    dynarray::append(&ba->all_buckets, bucket);
    return *dynarray::append(&ba->vacancy_buckets, bucket);
}


template<typename T, BucketItemCount ItemCount>
bool remove_at(BucketArray<T, ItemCount> *ba, BucketIndex bidx)
{
    Bucket<T, ItemCount> *b = ba->all_buckets[DynArrayCount(bidx.bucket_index)];
    if (!b->occupied[bidx.item_index]) {
        return false;
    }

    b->occupied[bidx.item_index] = false;
    --b->count;
    --ba->count;
    return true;
}


template<typename T, BucketItemCount ItemCount>
bool remove_at(BucketArray<T, ItemCount> *ba, s32 index)
{
    BucketIndex bidx = bucketarray::translate_index(ba, index);
    return remove(ba, bidx);
}


template <typename T, BucketItemCount ItemCount>
BucketIndex remove(BucketArray<T, ItemCount> *ba, T *item)
{
    BucketIndex bidx = bucketindex_of(ba, item);
    BucketIndex result = {-1, -1};

    if (bidx.bucket_index >= 0)
    {
        bool removed = remove_at(ba, bidx);

        if (removed)
        {
            result = bidx;
        }
    }

    return result;
}


template<typename T, BucketItemCount ItemCount>
IndexElemPair<T> add(BucketArray<T, ItemCount> *ba)
{
    DynArrayCount vacant_bucket_count = ba->vacancy_buckets.count;
    Bucket<T, ItemCount> *bucket;

    if (vacant_bucket_count > 0)
    {
        bucket = ba->vacancy_buckets[vacant_bucket_count - 1];
    }
    else
    {
        bucket = bucketarray::addbucket(ba);
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
        dynarray::swappop(&ba->vacancy_buckets, vacant_bucket_count - 1);
    }

    ++ba->count;

    IndexElemPair<T> result = {S32(bucket->index) + selected_index, &bucket->items[selected_index]};
    return result;
}


template <typename T, BucketItemCount ItemCount>
bool get_if_not_empty(OUTPARAM T **output, const BucketArray<T, ItemCount> *ba, BucketIndex bidx)
{
    Bucket<T, ItemCount> *b = ba->all_buckets[DYNARRAY_COUNT(bidx.bucket_index)];
    bool found = b->occupied[bidx.item_index];
    if (output) *output = found ? &b->items[bidx.item_index] : nullptr;
    return found;
}


template <typename T, BucketItemCount ItemCount>
bool get_if_not_empty(OUTPARAM T **output, const BucketArray<T, ItemCount> *ba, BucketItemCount index)
{
    BucketIndex bidx = bucketarray::translate_index(ba, index);
    return get_if_not_empty(output, ba, bidx);
}

}


template <typename T, BucketItemCount ItemCount>
T &BucketArray<T, ItemCount>::at(BucketItemCount index) const
{
    BucketIndex idx = bucketarray::translate_index(this, index);
    Bucket<T, ItemCount> *b = all_buckets[DYNARRAY_COUNT(idx.bucket_index)];
    return b->items[idx.item_index];
}


#define BUCKETARRAY_H
#endif
