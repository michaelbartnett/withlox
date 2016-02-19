// -*- c++ -*-

#ifndef HASHTABLE_H

#include "numeric_types.h"
#include "common.h"
#include <algorithm>
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


template<typename T>
bool basic_hash_key_equality(const T &a, const T &b)
{
    return a == b;
}


template<typename T>
u32 basic_integral_hash(const T &value)
{
    return ((u32)value + 23 * 17) ^ 541;
}


template<typename TKey, typename TValue>
struct OAHashtable
{
    typedef u32 (HashFn)(const TKey &val);
    typedef bool (KeysEqualFn)(const TKey &a, const TKey &b);

    struct BucketState
    {
        enum Enum
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

    HashFn *hashfn;
    KeysEqualFn *keys_equal_fn;
    u32 bucket_count;
    Bucket *buckets;
    Entry *entries;
    u32 count;
};


template <typename TKey, typename TValue>
void ht_deinit(OAHashtable<TKey, TValue> *ht)
{
    free(ht->buckets);
    free(ht->entries);
}

template <typename TKey, typename TValue>
OAHashtable<TKey, TValue>
ht_init(u32 bucket_count,
        typename OAHashtable<TKey, TValue>::HashFn hashfn,
        typename OAHashtable<TKey, TValue>::KeysEqualFn keys_equal_fn)
{
    typedef typename OAHashtable<TKey, TValue>::Entry Entry;
    typedef typename OAHashtable<TKey, TValue>::Bucket Bucket;

    OAHashtable<TKey, TValue> ht;
    ht.count = 0;
    ht.hashfn = hashfn;
    ht.keys_equal_fn = keys_equal_fn;
    ht.bucket_count = bucket_count;
    ht.buckets = CALLOC_ARRAY(Bucket, bucket_count);
    ht.entries = CALLOC_ARRAY(Entry, bucket_count);
    return ht;
}


template <typename TKey, typename TValue>
void ht_rehash(OAHashtable<TKey, TValue> *ht, u32 new_bucket_count)
{
    typedef typename OAHashtable<TKey, TValue>::Entry Entry;
    typedef typename OAHashtable<TKey, TValue>::Bucket Bucket;
    typedef typename OAHashtable<TKey, TValue>::BucketState BucketState;

    // don't shrink
    new_bucket_count = std::max(ht->bucket_count, new_bucket_count);

    Bucket *new_buckets = CALLOC_ARRAY(Bucket, new_bucket_count);
    Entry *new_entries = CALLOC_ARRAY(Entry, new_bucket_count);

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

    free(ht->buckets);
    free(ht->entries);
    ht->bucket_count = new_bucket_count;
    ht->buckets = new_buckets;
    ht->entries = new_entries;    
}


template <typename TKey, typename TValue>
TValue *ht_find(OAHashtable<TKey, TValue> *ht, TKey key)
{
    typedef typename OAHashtable<TKey, TValue>::BucketState BucketState;

    u32 hash = ht->hashfn(key);
    u32 bucket_idx = hash % ht->bucket_count;

    for (u32 i = bucket_idx, e = ht->bucket_count; i < e; ++i)
    {        
        switch ((typename BucketState::Enum)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return 0;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && ht->keys_equal_fn(key, ht->entries[i].key))
                {
                    return &ht->entries[i].value;
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    for (u32 i = 0, e = bucket_idx; i < e; ++i)
    {
        switch ((typename BucketState::Enum)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return 0;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && ht->keys_equal_fn(key, ht->entries[i].key))
                {
                    return &ht->entries[i].value;
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    return 0;
}


template <typename TKey, typename TValue>
void ht_set(OAHashtable<TKey, TValue> *ht, TKey key, TValue value)
{
    // typedef OAHashtable<TKey, TValue>::Entry Entry;
    // typedef OAHashtable<TKey, TValue>::Bucket Bucket;
    typedef typename OAHashtable<TKey, TValue>::BucketState BucketState;

    // if (ht->count * 3 > ht->bucket_count * 2)
    // {
    //     ht_rehash(ht, ht->bucket_count * 2);
    // }

    u32 hash = ht->hashfn(key);

    u32 bucket_idx = hash % ht->bucket_count;
    u32 picked_index = ht->bucket_count;
    bool picked = false;

    for (u32 i = bucket_idx, ie = ht->bucket_count; i < ie; ++i)
    {
        if (ht->buckets[i].state != BucketState::Filled
            || ht->keys_equal_fn(ht->entries[i].key, key))
        {
            picked_index = i;
            picked = true;
            break;
        }
    }
    if (!picked)
    {
        for (u32 i = 0, ie = bucket_idx; i < ie; ++i)
        {
            if (ht->buckets[i].state != BucketState::Filled ||
                ht->keys_equal_fn(ht->entries[i].key, key))
            {
                picked_index = i;
                picked = true;
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
    ht->entries[picked_index].value = value;

    assert(ht->count <= ht->bucket_count);
}


template <typename TKey, typename TValue>
bool ht_remove(OAHashtable<TKey, TValue> *ht, TKey key)
{
    // typedef OAHashtable<TKey, TValue>::Entry Entry;
    // typedef OAHashtable<TKey, TValue>::Bucket Bucket;
    typedef typename OAHashtable<TKey, TValue>::BucketState::Enum BucketState;

    u32 hash = ht->hashfn(key);
    u32 bucket_idx = hash % ht->bucket_count;

    for (u32 i = bucket_idx, e = ht->bucket_count; i < e; ++i)
    {        
        switch ((BucketState)ht->buckets[i].state)
        {
            case BucketState::Empty:
                return false;

            case BucketState::Filled:
                if (ht->buckets[i].hash == hash
                    && ht->keys_equal_fn(key, ht->entries[i].key))
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
                    && ht->keys_equal_fn(key, ht->entries[i].key))
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


#define HASHTABLE_H
#endif
