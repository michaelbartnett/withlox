// -*- c++ -*-

#ifndef HASHTABLE_H

#include "numeric_types.h"
#include "common.h"
#include <algorithm>
#include "MurmurHash3.h"
#include "memory.h"
#include <cassert>

/*
Hash tables!


http://www.reedbeta.com/blog/2015/01/12/data-oriented-hash-table/
http://burtleburtle.net/bob/hash/spooky.html
http://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf

https://gist.github.com/tonious/1377667
https://www.quora.com/How-would-you-implement-a-hash-table-in-C-from-scratch
http://stackoverflow.com/questions/1138742/looking-for-a-good-hash-table-implementation-in-c
http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html
https://www.cs.cmu.edu/~fp/courses/15122-f10/lectures/22-mem/hashtable.c
https://www.cs.princeton.edu/courses/archive/fall06/cos217/lectures/09HashTables-2x2.pdf
http://www.cprogramming.com/tutorial/computersciencetheory/hash-table.html
http://www.ks.uiuc.edu/Research/vmd/doxygen/hash_8c-source.html

http://see-programming.blogspot.com/2013/05/closed-hashing-linear-probing.html
http://www.austincc.edu/comer/ds14s/hashtabl.htm
http://codereview.stackexchange.com/questions/18883/hash-table-using-open-addressing-with-linear-probing
https://github.com/fuzxxl/yz
https://en.wikipedia.org/wiki/List_of_hash_functions
http://www.cse.yorku.ca/~oz/hash.html
http://cstheory.stackexchange.com/questions/9639/how-did-knuth-derive-a
http://www.burtleburtle.net/bob/hash/doobs.html
https://github.com/aappleby/smhasher
http://www.burtleburtle.net/bob/c/lookup2.c
http://www.burtleburtle.net/bob/c/lookup3.c
https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

http://www.gamedev.net/topic/634740-help-with-string-hashing/
http://www.isthe.com/chongo/tech/comp/fnv/
https://en.wikipedia.org/wiki/MurmurHash


AMAZING LINK:
http://programmers.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed

http://research.neustar.biz/2011/12/05/choosing-a-good-hash-function-part-1/
http://research.neustar.biz/2011/12/29/choosing-a-good-hash-function-part-2/
http://research.neustar.biz/2012/02/02/choosing-a-good-hash-function-part-3/



 */

/*
Strings and memory!

https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=managing+strings+in+C89+best+practices
https://en.wikibooks.org/wiki/C_Programming/Strings
http://www.drdobbs.com/managed-string-library-for-c/184402023?pgno=2
https://en.wikipedia.org/wiki/C_string_handling
http://stackoverflow.com/questions/17071460/best-practice-for-returning-a-variable-length-string-in-c
http://stackoverflow.com/questions/4580329/string-handling-practices-in-c
http://www.informit.com/articles/article.aspx?p=2036582&seqNum=4



 */


/*
Some bitsquid articles:

http://bitsquid.blogspot.com/2014/09/building-data-oriented-entity-system.html
http://bitsquid.blogspot.com/2015/12/data-driven-rendering-in-stingray.html
http://bitsquid.blogspot.com/2010/09/custom-memory-allocation-in-c.html
http://bitsquid.blogspot.com/2010/10/static-hash-values.html
http://bitsquid.blogspot.com/2010/10/time-step-smoothing.html
http://bitsquid.blogspot.com/2010/12/bitsquid-c-coding-style.html
http://bitsquid.blogspot.com/2011/01/managing-coupling.html
http://bitsquid.blogspot.com/2011/02/managing-decoupling-part-2-polling.html
http://bitsquid.blogspot.com/2011/02/some-systems-need-to-manipulate-objects.html
http://bitsquid.blogspot.com/2011/03/bitsquid-tech-benefits-of-data-driven.html
http://bitsquid.blogspot.com/2011/03/putting-some-of-power-of-programming.html
http://bitsquid.blogspot.com/2011/03/collaboration-and-merging.html
http://bitsquid.blogspot.com/2011/04/extreme-bug-hunting.html
http://bitsquid.blogspot.com/2011/04/universal-undo-copy-and-paste.html
http://bitsquid.blogspot.com/2011/05/flow-data-oriented-implementation-of.html
http://bitsquid.blogspot.com/2011/05/monitoring-your-game.html
http://bitsquid.blogspot.com/2011/06/strings-redux.html


 */



// template<typename T>
// struct BasicEquality
// {
//     bool operator()(const T &a, const T &b)
//     {
//         return a == b;
//     }
// };


// template<typename T>
// struct IntegralHash
// {
//     u32 operator()(const T&value)
//     {
//         return ((u32)value + 23 * 17) ^ 541;
//     }
// };


template <typename T>
bool hashtable_keys_equal(const T &lhs, const T &rhs)
{
    return lhs == rhs;
}


template<typename T>
struct OAHashtable_DefaultKeysEqual
{
    bool operator()(const T &lhs, const T &rhs)
    {
        return hashtable_keys_equal(lhs, rhs);
    }
};


template<typename T>
struct OAHashtable_DefaultHash
{
    u32 operator()(const T& value)
    {
        return fn(value);
    }

    static u32 fn(const T& value)
    {
        const u32 seed = 541;
        u32 result;
        MurmurHash3_x86_32(&value, sizeof(T), seed, &result);
        return result;
    }
};


#define OAHASH_TPARAMS <typename TKey, typename TValue, typename FKeysEqual, typename FKeyHash>
#define OAHASH_TYPE OAHashtable <TKey, TValue, FKeysEqual, FKeyHash>
template
<
    typename TKey, typename TValue,
    typename FKeysEqual=OAHashtable_DefaultKeysEqual<TKey>,
    typename FKeyHash=OAHashtable_DefaultHash<TKey>
>
struct OAHashtable
{
    typedef FKeyHash HashFn;
    typedef FKeysEqual KeyEqualFn;
    typedef u32 hash_type;

    struct BucketState
    {
        enum Tag
        {
            Empty,
            Filled,
            Removed
        };
    };

    struct Bucket
    {
        u32 hash;
        u32 state; // wasteful
    };

    struct Entry
    {
        TKey key;
        TValue value;
    };

    hash_type bucket_count;
    Bucket *buckets;
    Entry *entries;
    u32 count;
    mem::IAllocator *allocator;
};


// template<typename TKey, typename TValue, typename FKeyHash, typename FKeysEqual>
template OAHASH_TPARAMS
void ht_deinit(OAHASH_TYPE *ht)
{
    assert((!ht->buckets) == (!ht->entries));
    if (ht->buckets)
    {
        ht->allocator->dealloc(ht->buckets);
        ht->allocator->dealloc(ht->entries);
    }
}


// template<typename TKey, typename TValue, typename FKeysEqual, typename FKeyHash>
template OAHASH_TPARAMS
void ht_init(OAHASH_TYPE *ht,
             u32 initial_bucket_count = 23,
             mem::IAllocator *allocator = nullptr)
{
    typedef typename OAHASH_TYPE::Entry Entry;
    typedef typename OAHASH_TYPE::Bucket Bucket;

    if (!allocator)
    {
        allocator = mem::default_allocator();
    }

    ht->allocator = allocator;
    ht->count = 0;
    ht->bucket_count = initial_bucket_count;

    if (initial_bucket_count == 0)
    {
        ht->buckets = nullptr;
        ht->entries = nullptr;
    }
    else
    {
        ht->buckets = MAKE_ZEROED_ARRAY(ht->allocator, initial_bucket_count, Bucket);
        ht->entries = MAKE_ZEROED_ARRAY(ht->allocator, initial_bucket_count, Entry);
    }
}


// template <typename TKey, typename TValue, typename FKeysEqual, typename FKeyHash>
template OAHASH_TPARAMS
void ht_rehash(OAHASH_TYPE *ht, u32 new_bucket_count)
{
    typedef typename OAHASH_TYPE::Entry Entry;
    typedef typename OAHASH_TYPE::Bucket Bucket;
    typedef typename OAHASH_TYPE::BucketState BucketState;

    // don't shrink
    new_bucket_count = std::max(ht->bucket_count, new_bucket_count);

    Bucket *new_buckets = MAKE_ZEROED_ARRAY(ht->allocator, new_bucket_count, Bucket);
    Entry *new_entries = MAKE_ZEROED_ARRAY(ht->allocator, new_bucket_count, Entry);

    for (u32 i = 0; i < ht->bucket_count; ++i)
    {
        u32 hash = ht->buckets[i].hash;
        u32 start_bucket = hash % new_bucket_count;;
        bool picked = false;
        u32 picked_index = 0;
        if (ht->buckets[i].state != BucketState::Filled)
        {
            continue;
        }

        for (u32 inew = start_bucket; inew < new_bucket_count; ++inew)
        {
            if (new_buckets[inew].state != BucketState::Filled)
            {
                picked = true;
                picked_index = inew;
                break;
            }
        }

        if (!picked)
        {
            for (u32 inew = 0; inew < start_bucket; ++inew)
            {
                if (new_buckets[inew].state != BucketState::Filled)
                {
                    picked = true;
                    picked_index = inew;
                    break;
                }
            }
        }

        assert(picked);
        new_entries[picked_index] = ht->entries[i];
        new_buckets[picked_index] = ht->buckets[i];
    }

    ht->allocator->dealloc(ht->buckets);
    ht->allocator->dealloc(ht->entries);
    ht->bucket_count = new_bucket_count;
    ht->buckets = new_buckets;
    ht->entries = new_entries;
}


template OAHASH_TPARAMS
bool ht_find_or_add_entry(typename OAHASH_TYPE::Entry **result, OAHASH_TYPE *ht, TKey key)
{
    typedef typename OAHASH_TYPE::BucketState BucketState;
    typename OAHASH_TYPE::HashFn hashfn;
    typename OAHASH_TYPE::KeyEqualFn keys_equal_fn;
    typedef typename OAHASH_TYPE::Entry Entry;;

    if (ht->count * 3 + 1 > ht->bucket_count * 2)
    {
        const typename OAHASH_TYPE::hash_type min_buckets = 7;
        const typename OAHASH_TYPE::hash_type calculated_buckets = ht->bucket_count * 2 + 1;
        const typename OAHASH_TYPE::hash_type new_bucket_count =
            min_buckets > calculated_buckets
            ? min_buckets
            : calculated_buckets;

        ht_rehash(ht, new_bucket_count);
    }

    u32 hash = hashfn(key);
    u32 bucket_idx = hash % ht->bucket_count;
    u32 picked_index = 0;
    bool picked = false;

    for (u32 i = bucket_idx, e = ht->bucket_count; i < e; ++i)
    {
        if (ht->buckets[i].state == BucketState::Filled)
        {
            if (ht->buckets[i].hash == hash
                && keys_equal_fn(key, ht->entries[i].key))
            {
                *result = &ht->entries[i];
                return true;
            }
        }
        else
        {
            picked = true;
            picked_index = i;
            break;
        }
    }

    if (!picked_index)
    {
        for (u32 i = 0, e = bucket_idx; i < e; ++i)
        {
            if (ht->buckets[i].state == BucketState::Filled)
            {
                if (ht->buckets[i].hash == hash
                    && keys_equal_fn(key, ht->entries[i].key))
                {
                    *result = &ht->entries[i];
                    return true;
                }
            }
            else
            {
                picked = true;
                picked_index = i;
                break;
            }
        }
    }
    assert(picked);

    u32 count_inc = ht->buckets[picked_index].state == BucketState::Empty;
    assert(count_inc == 0 || count_inc == 1);
    ht->count += count_inc;
    ht->buckets[picked_index].hash = hash;
    ht->buckets[picked_index].state = BucketState::Filled;
    ht->entries[picked_index].key = key;
    mem::zero_obj(ht->entries[picked_index].value);

    assert(ht->count <= ht->bucket_count);

    *result = ht->entries + picked_index;
    return false;
}


template OAHASH_TPARAMS
typename OAHASH_TYPE::Entry *ht_find_entry(OAHASH_TYPE *ht, TKey key)
{
    typedef typename OAHASH_TYPE::BucketState BucketState;
    typename OAHASH_TYPE::HashFn hashfn;
    typename OAHASH_TYPE::KeyEqualFn keys_equal_fn;
    typedef typename OAHASH_TYPE::Entry Entry;;

    u32 hash = hashfn(key);
    u32 bucket_idx = hash % ht->bucket_count;

    for (u32 i = bucket_idx, e = ht->bucket_count; i < e; ++i)
    {
        switch ((typename BucketState::Tag)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return nullptr;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && keys_equal_fn(key, ht->entries[i].key))
                {
                    return &ht->entries[i];
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    for (u32 i = 0, e = bucket_idx; i < e; ++i)
    {
        switch ((typename BucketState::Tag)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return nullptr;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && keys_equal_fn(key, ht->entries[i].key))
                {
                    return &ht->entries[i];
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    return nullptr;
}


template OAHASH_TPARAMS
TValue *ht_find(OAHASH_TYPE *ht, TKey key)
{
    typedef typename OAHASH_TYPE::Entry Entry;

    Entry *entry = ht_find_entry(ht, key);
    return entry ? &entry->value : nullptr;
}


// returns true if something was occupying that spot
template OAHASH_TPARAMS
bool ht_set_if_unset(OUTPARAM TValue **p_stored_value, OAHASH_TYPE *ht, TKey key, TValue value)
{
    typedef typename OAHASH_TYPE::Entry Entry;

    Entry *entry;
    bool was_occupied = ht_find_or_add_entry(&entry, ht, key);

    if (!was_occupied)
    {
        entry->value = value;
    }

    if (p_stored_value)
    {
        *p_stored_value = &entry->value;
    }

    return was_occupied;
}


// returns true if something was occupying that spot
template OAHASH_TPARAMS
bool ht_set_if_unset(OAHASH_TYPE *ht, TKey key, TValue value)
{
    return ht_set_if_unset(static_cast<TValue **>(nullptr), ht, key, value);
}


// returns true if slot was previously occupied
template OAHASH_TPARAMS
bool ht_set(OUTPARAM TValue **p_stored_value, OAHASH_TYPE *ht, TKey key, TValue value)
{
    typedef typename OAHASH_TYPE::Entry Entry;
    Entry *entry;
    bool was_occupied = ht_find_or_add_entry(&entry, ht, key);
    entry->value = value;

    if (p_stored_value)
    {
        *p_stored_value = &entry->value;
    }

    return was_occupied;
}


// returns true if slot was previously occupied
template OAHASH_TPARAMS
bool ht_set(OAHASH_TYPE *ht, TKey key, TValue value)
{
    return ht_set<TKey, TValue>(nullptr, ht, key, value);
}


template OAHASH_TPARAMS
bool ht_remove(OAHASH_TYPE *ht, TKey key)
{
    typedef typename OAHASH_TYPE::BucketState::Tag BucketState;

    typename OAHASH_TYPE::HashFn hashfn;
    typename OAHASH_TYPE::KeyEqualFn keys_equal_fn;

    u32 hash = hashfn(key);
    u32 bucket_idx = hash % ht->bucket_count;

    for (u32 i = bucket_idx, e = ht->bucket_count; i < e; ++i)
    {
        switch ((BucketState)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return false;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && keys_equal_fn(key, ht->entries[i].key))
                {
                    ht->buckets[i].state = BucketState::Removed;
                    --ht->count;
                    return true;
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    for (u32 i = 0, e = bucket_idx; i < e; ++i)
    {
        switch ((BucketState)ht->buckets[i]->state)
        {
            case BucketState::Empty:
                return false;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && keys_equal_fn(key, ht->entries[i].key))
                {
                    ht->buckets[i].state = BucketState::Removed;
                    --ht->count;
                    return true;
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    return false;
}


#undef OAHASH_TYPE
#define HASHTABLE_H
#endif
