#include "types.h"
#include "json_error.h"

#include "json.h"

#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>

#include "common.h"
#include "platform.cpp"

using std::printf;
using std::size_t;


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
};


template <typename TKey, typename TValue>
void ht_init(OAHashtable<TKey, TValue> *ht, u32 bucket_count,
             typename OAHashtable<TKey, TValue>::HashFn hashfn,
             typename OAHashtable<TKey, TValue>::KeysEqualFn keys_equal_fn)
{
    typedef typename OAHashtable<TKey, TValue>::Entry Entry;
    typedef typename OAHashtable<TKey, TValue>::Bucket Bucket;

    ht->hashfn = hashfn;
    ht->keys_equal_fn = keys_equal_fn;
    ht->bucket_count = bucket_count;
    ht->buckets = MALLOC_ARRAY(Bucket, bucket_count);
    ht->entries = MALLOC_ARRAY(Entry, bucket_count);
}


template <typename TKey, typename TValue>
TValue *ht_find(OAHashtable<TKey, TValue> *ht, TKey key)
{
    // typedef OAHashtable<TKey, TValue>::Entry Entry;
    // typedef OAHashtable<TKey, TValue>::Bucket Bucket;
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


    // TODO(mike): rehash if too few buckets (2/3 is what Nathan Reed does. )

    u32 hash = ht->hashfn(key);

    u32 bucket_idx = hash % ht->bucket_count;
    u32 picked_index = ht->bucket_count;
    bool picked = false;

    for (u32 i = bucket_idx, ie = ht->bucket_count; i < ie; ++i)
    {
        if (ht->buckets[i].state != BucketState::Filled)
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
            if (ht->buckets[i].state != BucketState::Filled)
            {
                picked_index = i;
                picked = true;
                break;
            }
        }
    }

    assert(picked);

    ht->buckets[picked_index].hash = hash;
    ht->buckets[picked_index].state = BucketState::Filled;
    ht->entries[picked_index].key = key;
    ht->entries[picked_index].value = value;
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
                    return true;
                }
                break;

            case BucketState::Removed:
                break;
        }
    }

    return false;    
}



// template<typename TKey, typename TValue>
// void hashtable_init(hashtable *table, u32 tablesize, Hashtable<T>::HashFn *hashfn)
// {
//     table->hashfn = hashfn;
//     table->hash_count = tablesize;
//     table->hash_to_indices = MALLOC_ARRAY(T, tablesize);
//     table->data_count = tableisze;
//     table->values = MALLOC_ARRAY(TKey, tablesize);
//     table->keys = MALLOC_ARRAY(TValue, tablesize)
// }


// template<typename TKey, typename TValue>
// void hashtable_set(Hashtable *table, TKey key, TValue val)
// {
//     u32 hash = hashfn(val);
//     u32 hash_idx = hash % hash_count;
//     u32 idx = hash_to_indices[hash_idx];    
// }


// void hashtable_get(Hashtable *table, )


// TypeMemberSet *type_member_set_alloc(s32 count)
// {
//     TypeMemberSet *result = MALLOC(TypeMemberSet);

//     result->count = count;
//     if (count > 0) {
//         result->items = MALLOC_ARRAY(TypeMember, count);
//     } else {
//         result->items = 0;
//     }
//     return result;
// }


TypeDescriptor *type_desc_from_json(json_value_s *jv)
{
    TypeDescriptor *result = MALLOC(TypeDescriptor);
    memset(result, 0, sizeof(*result));
 
    json_type_e jvtype = (json_type_e)jv->type;
    switch (jvtype)
    {
        case json_type_string:
            result->type_id = TypeID::String;
            break;
        case json_type_number:
            result->type_id = TypeID::Float;
            break;

        case json_type_object:
        {
            json_object_s *jso = (json_object_s *)jv->payload;
            assert(jso->length < UINT32_MAX);
            result->members = dynarray_alloc<TypeMember>((u32)jso->length);

            for (json_object_element_s *elem = jso->start;
                 elem;
                 elem = elem->next)
            {
                TypeMember *member = append(&result->members);
                assert(elem->name->string_size < UINT16_MAX);
                member->name = str(elem->name->string, (u16)elem->name->string_size);
                member->type_desc = type_desc_from_json(elem->value);
            }
            result->type_id = TypeID::Compound;
            break;
        }

        case json_type_array:
            assert(0);
            break;
 
        case json_type_true:
        case json_type_false:
            result->type_id = TypeID::Bool;
            break;

       case json_type_null:
            result->type_id = TypeID::None;
            break   ;
    }

    return result;
}

TypeDescriptor *clone(const TypeDescriptor *type_desc);

DynArray<TypeMember> clone(const DynArray<TypeMember> memberset)
{
    DynArray<TypeMember> result = dynarray_alloc<TypeMember>(memberset.count);
    
    // TypeMemberSet *result = type_member_set_alloc(memberset->count);

    for (u32 i = 0; i < memberset.count; ++i)
    {
        TypeMember *src_member = get(memberset, i);
        TypeMember *dest_member = append(&result);
        dest_member->name = str(src_member->name);
        dest_member->type_desc = clone(src_member->type_desc);
    }

    return result;
}


TypeDescriptor *clone(const TypeDescriptor *type_desc)
{
    TypeDescriptor *result = MALLOC(TypeDescriptor);
    
    result->type_id = type_desc->type_id;
    switch ((TypeID::Enum)result->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            // nothing to do for non-compound types
            break;

        case TypeID::Compound:
            result->members = clone(type_desc->members);
            break;
    }

    return result;
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


bool equal(const TypeMember *a, const TypeMember *b)
{
    return str_equal(a->name, b->name) && equal(a->type_desc, b->type_desc);
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b)
{   
    switch ((TypeID::Enum)a->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            return a->type_id == b->type_id;

        case TypeID::Compound:
            size_t a_mem_count = a->members.count;
            if (a_mem_count != b->members.count)
            {
                return false;
            }

            for (u32 i = 0; i < a_mem_count; ++i)
            {
                if (! equal(get(a->members, i),
                            get(b->members, i)))
                {
                    return false;
                }
            }
            return true;
    }
}


TypeMember *find_member(const TypeDescriptor &type_desc, const StrSlice name)
{
    size_t count = type_desc.members.count;
    for (u32 i = 0; i < count; ++i)
    {
        TypeMember *mem = get(type_desc.members, i);

        if (str_equal(mem->name, name))
        {
            return mem;
        }
    }

    return 0;
}

void add_member(TypeDescriptor *type_desc, const TypeMember &member)
{
    TypeMember *new_member = append(&type_desc->members);
    new_member->name = member.name;
    new_member->type_desc = clone(member.type_desc);
}

// TypeDescriptor *merge_type_descriptors(TypeDescriptor *typedesc_a, TypeDescriptor *typedesc_b)
// {
//     TypeDescriptor *result = clone(typedesc_a);

//     assert((typedesc_a->type_id != TypeID::Compound) ^ (typedesc_a->type_id == typedesc_b->type_id));

//     switch ((TypeID::Enum)typedesc_a->type_id)
//     {
//         case TypeID::None:
//         case TypeID::String:
//         case TypeID::Int:
//         case TypeID::Float:
//         case TypeID::Bool:
//             // do nothing
//             break;

//         case TypeID::Compound:
//             assert(false);
//             break;
//     }

//     return result;
// }


TypeDescriptor *merge_compound_types(const TypeDescriptor *typedesc_a, const TypeDescriptor *typedesc_b)
{
    assert(typedesc_a->type_id == TypeID::Compound);
    assert(typedesc_b->type_id == TypeID::Compound);
    TypeDescriptor *result = clone(typedesc_a);

    for (u32 ib = 0; ib < typedesc_b->members.count; ++ib)
    {
        TypeMember *b_member = get(typedesc_b->members, ib);
        TypeMember *a_member = find_member(*typedesc_a, str_slice(b_member->name));

        if (! (a_member && equal(a_member->type_desc, b_member->type_desc)))
        {
            add_member(result, *b_member);
        }
    }
    return result;
}

#define printf_indent(indent, fmt, ...) printf("%*s" fmt, (indent), "", __VA_ARGS__)
#define printf_ln(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#define printf_ln_indent(indent, fmt, ...) printf("%*s" fmt "\n", indent, "", __VA_ARGS__)
#define println(text) printf("%s\n", text)
#define println_indent(indent, text) printf_ln_indent(indent, "%s", text)


void pretty_print(TypeDescriptor *type_desc, int indent = 0)
{
    TypeID::Enum type_id = (TypeID::Enum)type_desc->type_id;
    switch (type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            printf_ln("%s", to_string(type_id));
            break;

        case TypeID::Compound:
            println("{");
            indent += 2;

            for (u32 i = 0; i < type_desc->members.count; ++i)
            {
                TypeMember *member = get(type_desc->members, i);
                printf_indent(indent, "%s: ", member->name.data);
                pretty_print(member->type_desc, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}

template<typename T>
bool basic_equality(const T &a, const T &b)
{
    return a == b;
}

bool str_key_equals(const char *const &a, const char *const &b)
{
    return str_equal(str_slice(a), str_slice(b));
}

u32 str_hash(const char *const &str)
{
    u32 sum = 0;
    StrSlice slice = str_slice(str);
    for (int i = 0; i < slice.length; ++i) {
        sum += (u32)slice.data[i];
    }
    return sum;
}

#include <unistd.h>
int main(int argc, char **argv)
{
    typedef OAHashtable<const char*, s32> NumMap;
    NumMap number_names;
// (OAHashtable<TKey, TValue> *ht, u32 bucket_count, typename OAHashtable<TKey, TValue>::HashFn hashfn)
    ht_init(&number_names, 256, &str_hash, &str_equal);

    ht_set(&number_names, "one", 1);
    ht_set(&number_names, "two", 2);
    ht_set(&number_names, "three", 3);
    ht_set(&number_names, "four", 4);
    
    int *intarray = MALLOC_ARRAY(int, 25);
    for (int i = 0; i < 25; ++i)
    {
        intarray[i] = i + 1;
    }

    printf_ln("intarray[0]=%i", intarray[0]);
    
    // auto *buckets = number_names.buckets;
    // auto *entries = number_names.entries;

    int one = *ht_find(&number_names, "one");
    printf_ln("%s: %i", "one", one);
    int two = *ht_find(&number_names, "two");
    printf_ln("%s: %i", "two", two);

    int three = *ht_find(&number_names, "three");
    printf_ln("%s: %i", "three", three);

    int four = *ht_find(&number_names, "four");
    printf_ln("%s: %i", "four", four);

    return 0;
    
    

    if (argc < 2)
    {
        printf("No file specified\n");
        return 0;
    }

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    TypeDescriptor *result_td = 0;

    for (int i = 1; i < argc; ++i) {
        const char *filename = argv[i];
        Str jsonstr = read_file(filename);
        assert(jsonstr.data);   

        json_parse_result_s jp_result = {};
        json_value_s *jv = json_parse_ex(jsonstr.data, jsonstr.length,
                                         jsonflags, &jp_result);

        if (!jv)
        {
            printf("Json parse error: %s\nAt %lu:%lu",
                   json_error_code_string(jp_result.error),
                   jp_result.error_line_no,
                   jp_result.error_row_no);
            return 1;
        }

        TypeDescriptor *type_desc = type_desc_from_json(jv);
        
        println("New type desciptor:");
        pretty_print(type_desc);

        if (!result_td)
        {
            result_td = type_desc;
        }
        else
        {
            result_td = merge_compound_types(result_td, type_desc);
        }
    }

    println("completed without errors");

    pretty_print(result_td);
}
