// -*- c++ -*-
#ifndef PROGRAMSTATE_H

#include "str.h"
#include "bucketarray.h"
#include "hashtable.h"
#include "dynarray.h"
#include "nametable.h"
#include "typesys.h"
#include "clicommands.h"
#include "typesys_json.h"


typedef OAHashtable<StrSlice, Value, StrSliceEqual, StrSliceHash> StrToValueMap;


struct RecordInfo
{
    Str fullpath;
};

inline void recordinfo_deinit(RecordInfo *ri)
{
    str_free(&ri->fullpath);
}


struct Collection
{
    Str load_path;
    TypeDescriptor *top_typedesc;

    // Will be an array of top_typedesc
    Value value;
    DynArray<RecordInfo> info;
};


struct CollectionEditor
{
    Collection *collection;
    DynArray<Value *> breadcrumbs;
};


struct ProgramState
{
    NameTable names;
    BucketArray<TypeDescriptor> type_descriptors;
    OAHashtable<NameRef, TypeDescriptor *> typedesc_bindings;
    OAHashtable<TypeDescriptor *, DynArray<NameRef> > typedesc_reverse_bindings;

    TypeDescriptor *prim_string;
    TypeDescriptor *prim_int;
    TypeDescriptor *prim_float;
    TypeDescriptor *prim_bool;
    TypeDescriptor *prim_none;

    OAHashtable<StrSlice, CliCommand, StrSliceEqual, StrSliceHash> command_map;
    StrToValueMap value_map;

    BucketArray<Collection> collections;

    bool colection_editor_active;
    DynArray<Collection *> editing_collections;
};


inline void collection_assert_invariants(Collection *coll)
{
    ASSERT(coll->info.count == coll->value.array_value.elements.count);
    ASSERT(vIS_ARRAY(&coll->value));
}


void collection_deinit(Collection *coll);

void prgstate_init(ProgramState *prgstate);

void drop_collection(ProgramState *prgstate, BucketIndex bucket_index);

#define PROGRAMSTATE_H
#endif
