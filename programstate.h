// -*- c++ -*-
#ifndef PROGRAMSTATE_H

#include "str.h"
#include "bucketarray.h"
#include "hashtable.h"
#include "dynarray.h"
#include "nametable.h"
#include "types.h"
#include "clicommands.h"


typedef OAHashtable<StrSlice, Value, StrSliceEqual, StrSliceHash> StrToValueMap;
typedef OAHashtable<StrSlice, TypeDescriptor *, StrSliceEqual, StrSliceHash> StrToTypeMap;


struct LoadedRecord
{
    Str fullpath;
    Value value;
};


struct Collection
{
    Str load_path;
    DynArray<LoadedRecord *> records;
};


struct ProgramState
{
    NameTable names;
    BucketArray<TypeDescriptor> type_descriptors;
    OAHashtable<NameRef, TypeDescriptor *> typedesc_bindings;

    TypeDescriptor *prim_string;
    TypeDescriptor *prim_int;
    TypeDescriptor *prim_float;
    TypeDescriptor *prim_bool;
    TypeDescriptor *prim_none;

    OAHashtable<StrSlice, CliCommand, StrSliceEqual, StrSliceHash> command_map;
    StrToValueMap value_map;

    Str collection_directory;
    // DynArray<Value> collection;

    BucketArray<Collection> collections;
    BucketArray<LoadedRecord> loaded_records;

    bool colection_editor_active;
    DynArray<Collection *> editing_collections;
};


HEADERFN void prgstate_init(ProgramState *prgstate)
{
    nametable_init(&prgstate->names, MEGABYTES(2));
    bucketarray::init(&prgstate->type_descriptors);
    ht_init(&prgstate->typedesc_bindings);

    bucketarray::init(&prgstate->collections);
    bucketarray::init(&prgstate->loaded_records);

    ht_init(&prgstate->command_map);
    ht_init(&prgstate->value_map);

    dynarray::init(&prgstate->editing_collections, 0);
}



struct ParseResult
{
    enum Status
    {
        Eof,
        Failed,
        Succeeded
    };

    Status status;
    Str error_desc;
    size_t parse_offset;
    size_t error_offset;
    size_t error_line;
    size_t error_column;
};


// ParseResult load_json_dir(OUTPARAM DynArray<Value> *destarray, ProgramState *prgstate, StrSlice path);
ParseResult load_json_dir(OUTPARAM Collection **pcollection, ProgramState *prgstate, StrSlice path);
void drop_collection(ProgramState *prgstate, BucketIndex bucket_index);


#define PROGRAMSTATE_H
#endif
