// -*- c++ -*-
#ifndef TYPESYS_JSON_H

#include "typesys.h"

struct ProgramState;
struct Collection;

// Uses the json.h library
struct json_value_s;
struct json_object_s;
struct json_array_s;


struct JsonParseResult
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

    void release();
};


struct LoadJsonDirResult
{
    enum ErrorKind
    {
        NoError,
        FileError,
        ParseError
    };

    Collection *collection;
    ErrorKind error_kind;

    union
    {
        JsonParseResult parse_error;
        PlatformError file_error;
    };


    // bool is_error()
    // {
    //     return fs_error.is_error() || last_parse_result.status != JsonParseResult::Succeeded;
    // }

    // bool is_fs_error()
    // {
    //     return fs_error.is_error();
    // }

    void release();

    static LoadJsonDirResult from_fs_error(PlatformError platform_error)
    {
        LoadJsonDirResult result = {};
        result.error_kind = FileError;
        result.file_error = platform_error;
        return result;
    }

    static LoadJsonDirResult from_parse_error(JsonParseResult parse_result)
    {
        LoadJsonDirResult result = {};
        result.error_kind = ParseError;
        result.parse_error = parse_result;
        return result;
    }
};


// Infer type descriptors from JSON

TypeDescriptor *typedesc_from_json(ProgramState *prgstate, json_value_s *jv);

TypeDescriptor *typedesc_from_json_array(ProgramState *prgstate, json_value_s *jv);

TypeDescriptor *typedesc_from_json_object(ProgramState *prgstate, json_value_s *jv);


// Crate values from JSON (includes inferring type descriptors)

Value create_value_from_json(ProgramState *prgstate, json_value_s *jv);

Value create_object_with_type_from_json(ProgramState *prgstate, json_object_s *jobj, TypeDescriptor *typedesc);

Value create_array_with_type_from_json(ProgramState *prgstate, json_array_s *jarray, TypeDescriptor *typedesc);

JsonParseResult try_parse_json_as_value(OUTPARAM Value *output, ProgramState *prgstate,
                                        const char *input, size_t input_length);

// JsonParseResult load_json_dir(OUTPARAM Collection **pcollection, ProgramState *prgstate,
LoadJsonDirResult load_json_dir(ProgramState *prgstate, const char *path, size_t path_length);

#define TYPESYS_JSON_H
#endif
