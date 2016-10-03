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


inline void loadedrecord_free(LoadedRecord *lr)
{
    value_free(&lr->value);
    str_free(&lr->fullpath);
}


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
    StrToTypeMap type_map;

    Str collection_directory;
    // DynArray<Value> collection;
    // bool collection_loaded;

    DynArray<LoadedRecord> collection;

    bool colection_editor_active;
};


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
ParseResult load_json_dir(OUTPARAM DynArray<LoadedRecord> *destarray, ProgramState *prgstate, StrSlice path);
void drop_loaded_records(ProgramState *prgstate);

#define PROGRAMSTATE_H
#endif
