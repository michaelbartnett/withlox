#ifndef PROGRAMSTATE_H

#include "str.h"
#include "hashtable.h"
#include "dynarray.h"
#include "nametable.h"
#include "types.h"
#include "clicommands.h"


typedef OAHashtable<StrSlice, Value, StrSliceEqual, StrSliceHash> StrToValueMap;
typedef OAHashtable<StrSlice, TypeRef, StrSliceEqual, StrSliceHash> StrToTypeMap;


struct ProgramState
{
    NameTable names;
    DynArray<TypeDescriptor> type_descriptors;
    OAHashtable<NameRef, TypeRef> typedesc_bindings;

    TypeRef prim_string;
    TypeRef prim_int;
    TypeRef prim_float;
    TypeRef prim_bool;
    TypeRef prim_none;

    OAHashtable<StrSlice, CliCommand, StrSliceEqual, StrSliceHash> command_map;
    StrToValueMap value_map;
    StrToTypeMap type_map;

    Str collection_directory;
    DynArray<Value> collection;
    bool collection_loaded;
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


ParseResult load_json_dir(OUTPARAM DynArray<Value> *destarray, ProgramState *prgstate, StrSlice path);

#define PROGRAMSTATE_H
#endif
