#include "logging.h"

#include "pretty.h"
#include "types.h"
#include "hashtable.h"
#include "bucketarray.h"
#include "nametable.h"
#include "json_error.h"
#include "str.h"
#include "common.h"
#include "platform.h"
#include "json.h"
#include "linenoise.h"
#include "imgui_helpers.h"
// #include "dearimgui/imgui_internal.h"
#include "dearimgui/imgui.h"
#include "dearimgui/imgui_impl_sdl.h"
#include "memory.h"
#include <SDL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

#include <signal.h>
#include <unistd.h>

#include "tokenizer.h"
#include "clicommands.h"
#include "programstate.h"
#include <algorithm>


/*

Memory Management:

http://www.ibm.com/developerworks/aix/tutorials/au-memorymanager/

https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9?at=default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/memory.h?at=default&fileviewer=file-view-default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/memory.cpp?at=default&fileviewer=file-view-default
https://bitbucket.org/bitsquid/foundation/src/7f896236dbafd2cb842655004b1c7bf6e76dcef9/temp_allocator.h?at=default&fileviewer=file-view-default
https://www.google.com/search?q=bitsquid+blog&oq=bitsquid+blog&aqs=chrome..69i57j0j69i59.2205j0j7&sourceid=chrome&ie=UTF-8
http://bitsquid.blogspot.com/2016/04/the-poolroom-figure-seq-figure-arabic-1.html
http://bitsquid.blogspot.com/2012/11/bitsquid-foundation-library.html
http://bitsquid.blogspot.com/2012/09/a-new-way-of-organizing-header-files.html
http://gamesfromwithin.com/the-always-evolving-coding-style
http://gamesfromwithin.com/opengl-and-uikit-demo
https://twitter.com/GeorgeSealy/status/15800523038
http://gamesfromwithin.com/backwards-is-forward-making-better-games-with-test-driven-development
http://gamesfromwithin.com/simple-is-beautiful
http://bitsquid.blogspot.com/2012/01/sensible-error-handling-part-1.html
http://bitsquid.blogspot.com/2012/02/sensible-error-handling-part-2.html
http://bitsquid.blogspot.com/2012/02/sensible-error-handling-part-3.html
http://bitsquid.blogspot.com/2010/12/bitsquid-c-coding-style.html
http://bitsquid.blogspot.com/2011/12/platform-specific-resources.html
http://bitsquid.blogspot.com/2011/12/pragmatic-approach-to-performance.html
http://bitsquid.blogspot.com/2011/08/idea-for-better-watch-windows.html
http://bitsquid.blogspot.com/2011/05/monitoring-your-game.html
http://web.archive.org/web/20120419004126/http://www.altdevblogaday.com/2011/05/17/a-birds-eye-view-of-your-memory-map/
http://bitsquid.blogspot.com/2011/01/managing-coupling.html
http://bitsquid.blogspot.com/2011/02/managing-decoupling-part-2-polling.html
http://bitsquid.blogspot.com/2011/02/some-systems-need-to-manipulate-objects.html
http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html
http://bitsquid.blogspot.com/2011/11/example-in-data-oriented-design-sound.html


 */

/* JSON Libraries

http://www.json.org/
http://lloyd.github.io/yajl/
http://www.json.org/JSON_checker/
https://github.com/udp/json-parser
https://github.com/udp/json-builder
https://github.com/zserge/jsmn
https://docs.google.com/spreadsheets/d/1L8XrSas9_PS5RKduSgXUiHDmH50VQxsvwjYDoRHu_9I/edit#gid=0
https://github.com/kgabis/parson
https://github.com/esnme/ujson4c/
https://github.com/esnme/ultrajson
https://bitbucket.org/yarosla/nxjson/src
https://github.com/cesanta/frozen

https://github.com/nothings/stb/blob/master/docs/other_libs.md
https://github.com/kazuho/picojson
https://github.com/sheredom/json.h
https://github.com/Zguy/Jzon/blob/master/Jzon.h
https://github.com/kgabis/parson
https://github.com/miloyip/nativejson-benchmark
https://github.com/open-source-parsers/jsoncpp
https://github.com/giacomodrago/minijson_writer
https://www.quora.com/What-is-the-best-C-JSON-library
https://github.com/esnme/ujson4c/blob/master/src/ujdecode.c
https://www.google.com/search?q=c%2B%2B+json&oq=c%2B%2B+json&aqs=chrome.0.0l2j69i60j0j69i61l2.1031j0j7&sourceid=chrome&ie=UTF-8#q=c+json

 */

// Debugging
// http://stackoverflow.com/questions/312312/what-are-some-reasons-a-release-build-would-run-differently-than-a-debug-build
// http://www.codeproject.com/Articles/548/Surviving-the-Release-Version


// Type theory
// http://chris-taylor.github.io/blog/2013/02/10/the-algebra-of-algebraic-data-types/
// https://en.wikipedia.org/wiki/Algebraic_data_type
// https://en.wikipedia.org/wiki/Generalized_algebraic_data_type

// GraphQL has a type system. Interesting?
// https://facebook.github.io/graphql/

// class MemStack
// {
// public:
//     MemoryStack(size_t size)
//         : size(size)
//         , top(0)
//     {
//         data = malloc(size);
//     }

//     size_t size;
//     u8 *data;
//     u8 *top;
// /*


// ----
// size_prev:32
// size_this:30
// flags:2  ALLOC/FREE (meaning, this block)
// ----
// payload
// ----
// next header...
// ----



// HEADER

// to push, add header
// to pop, lookup header, remove by size

// FOOTER




//  */
// }


void prgstate_init(ProgramState *prgstate)
{
    nametable_init(&prgstate->names, MEGABYTES(2));
    bucketarray_init(&prgstate->type_descriptors);
    // dynarray_init(&prgstate->type_descriptors, 0);
    // dynarray_append(&prgstate->type_descriptors);
    ht_init(&prgstate->typedesc_bindings);

    dynarray_init(&prgstate->collection, 0);

    TypeDescriptor *none_type = add_typedescriptor(prgstate);
    none_type->type_id = TypeID::None;
    bind_typedesc_name(prgstate, TypeID::to_string(none_type->type_id),
                       none_type);
    prgstate->prim_none = none_type;

    TypeDescriptor *string_type = add_typedescriptor(prgstate);
    string_type->type_id = TypeID::String;
    bind_typedesc_name(prgstate, TypeID::to_string(string_type->type_id),
                       string_type);
    prgstate->prim_string = string_type;

    TypeDescriptor *int_type = add_typedescriptor(prgstate);
    int_type->type_id = TypeID::Int;
    bind_typedesc_name(prgstate, TypeID::to_string(int_type->type_id),
                       int_type);
    prgstate->prim_int = int_type;

    TypeDescriptor *float_type = add_typedescriptor(prgstate);
    float_type->type_id = TypeID::Float;
    bind_typedesc_name(prgstate, TypeID::to_string(float_type->type_id),
                       float_type);
    prgstate->prim_float = float_type;

    TypeDescriptor *bool_type = add_typedescriptor(prgstate);
    bool_type->type_id = TypeID::Bool;
    bind_typedesc_name(prgstate, TypeID::to_string(bool_type->type_id),
                       bool_type);
    prgstate->prim_bool = bool_type;

    ht_init(&prgstate->command_map);
    ht_init(&prgstate->value_map);
    ht_init(&prgstate->type_map);
}


TypeDescriptor *typedesc_from_json(ProgramState *prgstate, json_value_s *jv);


TypeDescriptor *typedesc_from_json_array(ProgramState *prgstate, json_value_s *jv)
{
    TypeDescriptor *result;

    assert(jv->type == json_type_array);
    json_array_s *jarray = (json_array_s *)jv->payload;

    DynArray<TypeDescriptor *> element_types;
    dynarray_init(&element_types, DYNARRAY_COUNT(jarray->length));

    for (json_array_element_s *elem = jarray->start;
         elem;
         elem = elem->next)
    {
        TypeDescriptor *typedesc = typedesc_from_json(prgstate, elem->value);

        // This is a set insertion
        bool found = false;
        for (DynArrayCount i = 0; i < element_types.count; ++i)
        {
            TypeDescriptor *existing = element_types[i];
            if (typedesc == existing)
            {
                found = true;
                break;
            }
        }

        if (! found)
        {
            dynarray_append(&element_types, typedesc);
        }
    }

    TypeDescriptor constructed_typedesc;
    constructed_typedesc.type_id = TypeID::Array;

    if (element_types.count > 1)
    {
        // Element type is Union

        // logln("Ecountered a heterogeneous json array. "
        //       "It will be reported as an Array of None type since this is not yet supported");



// WARNING(mike): DID NOT SORTING BREAK THIS???
        // // Sort the typeref array so that equality checks for union types can be linear
        // dynarray_sort_unstable<SortTypeRef>(&element_types);


        UnionType union_type;
        union_type.type_cases = dynarray_clone(&element_types);

        TypeDescriptor element_union_type;
        element_union_type.type_id = TypeID::Union;
        element_union_type.union_type = union_type;

        bool new_type_added;

        constructed_typedesc.array_type.elem_type =
            find_equiv_typedesc_or_add(prgstate, &element_union_type, &new_type_added);
        if (! new_type_added)
        {
            free_typedescriptor_components(&element_union_type);
        }
    }
    else
    {
        // Element type is non-Union
        ArrayType array_type;

        if (element_types.count == 0)
        {
            // empty,  untyped array...?
            // maybe use an Any type here, if that ever becomes a thing.
            logln("Got an empty json array. This defaults to type [None], but I don't like it!");
            array_type.elem_type = prgstate->prim_none;
            constructed_typedesc.array_type = array_type;
        }
        else
        {
            constructed_typedesc.array_type.elem_type = element_types[0];
        }
    }

    // If it's the empty array, bound to find a preexisting. Will that be common?
    bool new_type_added;
    result = find_equiv_typedesc_or_add(prgstate, &constructed_typedesc, &new_type_added);
    if (! new_type_added)
    {
        free_typedescriptor_components(&constructed_typedesc);
    }

    dynarray_deinit(&element_types);

    return result;
}


TypeDescriptor *typedesc_from_json_object(ProgramState *prgstate, json_value_s *jv)
{
    TypeDescriptor *result;

    assert(jv->type == json_type_object);
    json_object_s *jobj = (json_object_s *)jv->payload;

    DynArray<CompoundTypeMember> members;
    dynarray_init(&members, DYNARRAY_COUNT(jobj->length));

    for (json_object_element_s *elem = jobj->start;
         elem;
         elem = elem->next)
    {
        CompoundTypeMember *member = dynarray_append(&members);
        member->name = nametable_find_or_add(&prgstate->names,
                                             elem->name->string, elem->name->string_size);
        member->typedesc = typedesc_from_json(prgstate, elem->value);
    }

    TypeDescriptor constructed_typedesc;
    constructed_typedesc.type_id = TypeID::Compound;
    constructed_typedesc.compound_type.members = members;

    TypeDescriptor *preexisting = find_equiv_typedesc(prgstate, &constructed_typedesc);
    if (preexisting)
    {
        result = preexisting;
    }
    else
    {
        result = add_typedescriptor(prgstate, constructed_typedesc);
    }

    return result;
}


TypeDescriptor *typedesc_from_json(ProgramState *prgstate, json_value_s *jv)
{
    TypeDescriptor *result;

    json_type_e jvtype = (json_type_e)jv->type;
    switch (jvtype)
    {
        case json_type_string:
            result = prgstate->prim_string;
            break;

        case json_type_number:
        {
            json_number_s *jnum = (json_number_s *)jv->payload;
            tokenizer::Token number_token = tokenizer::read_number(jnum->number, jnum->number_size);
            result = number_token.type == tokenizer::TokenType::Int ? prgstate->prim_int : prgstate->prim_float;
            break;
        }

        case json_type_object:
            result = typedesc_from_json_object(prgstate, jv);
            break;

        case json_type_array:
            result = typedesc_from_json_array(prgstate, jv);
            break;

        case json_type_true:
        case json_type_false:
            result = prgstate->prim_bool;
            break;

       case json_type_null:
           result = prgstate->prim_none;
           break;
    }

    return result;
}


DynArray<CompoundTypeMember> clone(const DynArray<CompoundTypeMember> &memberset)
{
    DynArray<CompoundTypeMember> result = dynarray_init<CompoundTypeMember>(memberset.count);

    for (u32 i = 0; i < memberset.count; ++i)
    {
        CompoundTypeMember *src_member = &memberset[i];
        CompoundTypeMember *dest_member = dynarray_append(&result);
        ZERO_PTR(dest_member);
        NameRef newname = src_member->name;
        dest_member->name = newname;
        dest_member->typedesc = src_member->typedesc;
    }

    return result;
}


Value create_value_from_token(ProgramState *prgstate, tokenizer::Token token)
{
    // good test: {"tulsi": "gabbard", "julienne": { "fries": 42.2, "asponge": {}, "hillarymoney": 9001}}
    // bindinfer "Candidate" {"tulsi": "gabbard", "julienne": { "fries": 42.2, "asponge": {}, "hillarymoney": 9001}}
    // checktype "Candidate" {"tulsi":"poopoo","julienne":{"fries":9999999.0,"asponge":{},"hillarymoney":250000}}
    Str token_copy = str(token.text);
    Value result = {};

    switch (token.type)
    {
        case tokenizer::TokenType::Eof:
        case tokenizer::TokenType::Unknown:
            result.typedesc = prgstate->prim_none;
            break;

        case tokenizer::TokenType::String:
            result.str_val = token_copy;
            result.typedesc = prgstate->prim_string;
            // indicate no need to free
            token_copy.data = 0;
            break;

        case tokenizer::TokenType::Int:
            result.s32_val = atoi(token_copy.data);
            result.typedesc = prgstate->prim_int;
            break;

        case tokenizer::TokenType::Float:
            result.f32_val = (float)atof(token_copy.data);
            result.typedesc = prgstate->prim_float;
            break;

        case tokenizer::TokenType::True:
            result.typedesc = prgstate->prim_bool;
            result.bool_val = true;
            break;

        case tokenizer::TokenType::False:
            result.typedesc = prgstate->prim_bool;
            result.bool_val = false;
            break;
    }

    if (token_copy.data)
    {
        str_free(&token_copy);
    }
    return result;
}


Value create_value_from_json(ProgramState *prgstate, json_value_s *jv);

// Value create_object_with_type_from_json(ProgramState *prgstate, json_object_s *jobj, TypeRef typeref);
Value create_object_with_type_from_json(ProgramState *prgstate, json_object_s *jobj, TypeDescriptor *typedesc);


Value create_array_with_type_from_json(ProgramState *prgstate, json_array_s *jarray, TypeDescriptor *typedesc)
{
    Value result;
    result.typedesc = typedesc;

    dynarray_init(&result.array_value.elements, DYNARRAY_COUNT(jarray->length));
    TypeDescriptor *elem_typedesc = typedesc->array_type.elem_type;

    DynArrayCount member_idx = 0;
    for (json_array_element_s *jelem = jarray->start;
         jelem;
         jelem = jelem->next)
    {
        Value *value_element = dynarray_append(&result.array_value.elements);

        TYPESWITCH (elem_typedesc->type_id)
        {
            case TypeID::None:
            case TypeID::String:
            case TypeID::Int:
            case TypeID::Float:
            case TypeID::Bool:
            case TypeID::Union:
                *value_element = create_value_from_json(prgstate, jelem->value);
                break;

            case TypeID::Array:
                assert(jelem->value->type == json_type_array);
                *value_element =
                    create_array_with_type_from_json(prgstate,
                                                     (json_array_s *)jelem->value->payload,
                                                     elem_typedesc);
                break;

            case TypeID::Compound:
                assert(jelem->value->type == json_type_object);
                *value_element =
                    create_object_with_type_from_json(prgstate,
                                                      (json_object_s *)jelem->value->payload,
                                                      elem_typedesc);
                break;
        }

        ++member_idx;
    }

    return result;
}


Value create_object_with_type_from_json(ProgramState *prgstate, json_object_s *jobj, TypeDescriptor *typedesc)
{
    Value result;

    result.typedesc = typedesc;
    dynarray_init(&result.compound_value.members, typedesc->compound_type.members.count);

    DynArrayCount member_idx = 0;
    for (json_object_element_s *jelem = jobj->start;
         jelem;
         jelem = jelem->next)
    {
        CompoundTypeMember *type_member = &typedesc->compound_type.members[member_idx];
        CompoundValueMember *value_member = dynarray_append(&result.compound_value.members);
        value_member->name = type_member->name;

        TypeDescriptor *type_member_desc = type_member->typedesc;

        TYPESWITCH (type_member_desc->type_id)
        {
            case TypeID::None:
            case TypeID::String:
            case TypeID::Int:
            case TypeID::Float:
            case TypeID::Bool:
                value_member->value = create_value_from_json(prgstate, jelem->value);
                break;

            case TypeID::Array:
                value_member->value =
                    create_array_with_type_from_json(prgstate,
                                                     (json_array_s *)jelem->value->payload,
                                                     type_member->typedesc);
                break;

            case TypeID::Compound:
                assert(jelem->value->type == json_type_object);
                value_member->value =
                    create_object_with_type_from_json(prgstate,
                                                      (json_object_s *)jelem->value->payload,
                                                      type_member->typedesc);
                break;

            case TypeID::Union:
                assert(!(bool)"Should never happen");
                break;
        }

        ++member_idx;
    }

    return result;
}


Value create_value_from_json(ProgramState *prgstate, json_value_s *jv)
{
    Value result;

    switch ((json_type_e)jv->type)
    {
        case json_type_string:
        {
            json_string_s *jstr = (json_string_s *)jv->payload;
            result.typedesc = prgstate->prim_string;
            StrLen length = STRLEN(jstr->string_size);
            result.str_val = str(jstr->string, length);
            break;
        }

        case json_type_number:
        {
            json_number_s *jnum = (json_number_s *)jv->payload;
            tokenizer::Token number_token = tokenizer::read_number(jnum->number, jnum->number_size);

            result = create_value_from_token(prgstate, number_token);
            break;
        }

        case json_type_object:
        {
            TypeDescriptor *typedesc = typedesc_from_json_object(prgstate, jv);
            json_object_s *jobj = (json_object_s *)jv->payload;
            result = create_object_with_type_from_json(prgstate, jobj, typedesc);
            break;
        }

        case json_type_array:
        {
            TypeDescriptor *typedesc = typedesc_from_json_array(prgstate, jv);
            assert(typedesc->type_id == TypeID::Array);
            json_array_s *jarray = (json_array_s *)jv->payload;
            result = create_array_with_type_from_json(prgstate, jarray, typedesc);
            break;
        }

        case json_type_true:
            result.typedesc = prgstate->prim_bool;
            result.bool_val = true;
            break;

        case json_type_false:
            result.typedesc = prgstate->prim_bool;
            result.bool_val = false;
            break;

        case json_type_null:
            result.typedesc = prgstate->prim_none;
            break;
    }

    return result;
}


ParseResult try_parse_json_as_value(OUTPARAM Value *output, ProgramState *prgstate, const StrSlice input)
{
    ParseResult result = {};

    if (input.length == 0)
    {
        result.status = ParseResult::Eof;
        return result;
    }

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    json_parse_result_s jp_result = {};
    json_value_s *jv = json_parse_ex(input.data, input.length,
                                     jsonflags, &jp_result);

    result.parse_offset = jp_result.parse_offset;

    if (jp_result.error != json_parse_error_none)
    {
        // json.h doesn't seem to be consistent in whether it point to
        // errors before or after the relevant character
        bool invalid_value = jp_result.error == json_parse_error_invalid_value;
        bool invalid_number = jp_result.error == json_parse_error_invalid_number_format;

        result.error_offset = jp_result.error_offset;
        result.error_line = jp_result.error_line_no;
        result.error_column = jp_result.error_row_no;
        result.error_desc = str(json_error_code_string(jp_result.error));

        // offset for json.h error reporting weirdness
        result.error_column += (invalid_number ? 1 : 0);
        result.error_offset += (invalid_value ? 1 : 0);

        result.status = ParseResult::Failed;
    }
    else if (! jv)
    {
        // if jv was null, that means end of input,
        // but need to check for error first
        result.status = ParseResult::Eof;
    }
    else
    {
        *output = create_value_from_json(prgstate, jv);
        result.status = ParseResult::Succeeded;
    }

    return result;
}


// TODO(mike): Should probably be its own return type instead
// of piggybacking on the ParseResult
ParseResult load_json_dir(OUTPARAM DynArray<LoadedRecord> *destarray, ProgramState *prgstate, StrSlice path)
{
    assert(destarray);
    ParseResult result = {};
    result.status = ParseResult::Succeeded;

    DirLister dirlist(path);

    bool error = false;

    DynArrayCount num_values_read = 0;

    while (dirlist.next())
    {
        if (dirlist.current.is_file && str_endswith_ignorecase(dirlist.current.name, ".json"))
        {
            Str filecontents = {};

            FileReadResult read_result = read_text_file(&filecontents, dirlist.current.access_path.data);

            if (read_result.error_kind != FileReadResult::NoError)
            {
                FormatBuffer errorfmt;
                errorfmt.writef("Error reading json file '%s': ", dirlist.current.name.data);
                errorfmt.writeln(read_result.platform_error.message.data);
                result.error_desc = str(errorfmt.buffer, STRLEN(errorfmt.cursor));
                error = true;
            }

            read_result.release();

            if (error) break;

            Value parsed_value;
            ParseResult parse_result = try_parse_json_as_value(
                OUTPARAM &parsed_value, prgstate, str_slice(filecontents));

            switch (parse_result.status)
            {
                case ParseResult::Eof:
                    break;

                case ParseResult::Failed:
                {
                    result = parse_result;
                    FormatBuffer errorfmt;
                    errorfmt.writef("error parsing json file %s: %s", dirlist.current.name.data, result.error_desc.data);
                    result.error_desc = str(errorfmt.buffer, STRLEN(errorfmt.cursor));
                    error = true;
                    break;
                }
                case ParseResult::Succeeded:
                {
                    Str fullpath = {};
                    PlatformError abspath_err = resolve_path(&fullpath, dirlist.current.access_path.data);
                    if (abspath_err.is_error())
                    {
                        FormatBuffer errorfmt;
                        errorfmt.writef("error resolving absolute path for '%s': %s",
                                        dirlist.current.access_path.data, abspath_err.message.data);
                        result.error_desc = str(errorfmt.buffer, STRLEN(errorfmt.cursor));
                        error = true;
                    }
                    else
                    {
                        LoadedRecord lr = {fullpath, parsed_value};
                        dynarray_append(destarray, lr);
                        ++num_values_read;
                    }
                    break;
                }
            }
        }
    }

    if (error)
    {
        // expect caller to deallocate
        dynarray_popnum(destarray, num_values_read);
    }

    return result;
}


void drop_loaded_records(ProgramState *prgstate)
{
    for (DynArrayCount i = 0; i < prgstate->collection.count; ++i)
    {
        value_free(&prgstate->collection[i].value);
        str_free(&prgstate->collection[i].fullpath);
    }
    dynarray_clear(&prgstate->collection);
}


void test_json_import(ProgramState *prgstate, int filename_count, char **filenames)
{
    if (filename_count < 1)
    {
        logf("No files specified\n");
        return;
    }


    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    TypeDescriptor *result = nullptr;

    for (int i = 0; i < filename_count; ++i) {
        const char *filename = filenames[i];
        Str jsonstr = read_file(filename);
        assert(jsonstr.data);

        json_parse_result_s jp_result = {};
        json_value_s *jv = json_parse_ex(jsonstr.data, jsonstr.length,
                                         jsonflags, &jp_result);

        if (!jv)
        {
            logf("Json parse error: %s\nAt %lu:%lu",
                   json_error_code_string(jp_result.error),
                   jp_result.error_line_no,
                   jp_result.error_row_no);
            return;
        }

        TypeDescriptor *typedesc = typedesc_from_json(prgstate, jv);
        NameRef bound_name = nametable_find_or_add(&prgstate->names, filename);
        bind_typedesc_name(prgstate, bound_name, typedesc);

        logln("New type desciptor:");
        pretty_print(typedesc);

        if (!result)
        {
            result = typedesc;
        }
        else
        {
            result = merge_types(prgstate, result, typedesc);
        }
    }

    logln("completed without errors");

    pretty_print(result);
}


void log_parse_error(ParseResult pr, StrSlice input, size_t input_offset_from_src)
{
    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();

    fmt_buf.writef("Json parse error: %s\nAt %lu:%lu\n%s\n",
                   pr.error_desc.data,
                   pr.error_line,
                   pr.error_column + (pr.error_line > 1 ? 0 : input_offset_from_src),
                   input.data);

    size_t buffer_offset = pr.error_offset + input_offset_from_src;
    for (size_t i = 0; i < buffer_offset - 1; ++i) fmt_buf.write("~");
    fmt_buf.write("^");
}


void process_console_input(ProgramState *prgstate, StrSlice input_buf)
{
    tokenizer::State tokstate;
    tokenizer::init(&tokstate, input_buf.data);

    tokenizer::Token first_token = tokenizer::read_string(&tokstate);

    DynArray<Value> cmd_args;
    dynarray_init(&cmd_args, 10);

    bool error = false;

    for (;;) {
        size_t offset_from_input = (size_t)(tokstate.current - input_buf.data);

        Value parsed_value;
        StrSlice parse_slice = str_slice(tokstate);
        ParseResult parse_result = try_parse_json_as_value(OUTPARAM &parsed_value, prgstate, parse_slice);

        switch (parse_result.status)
        {
            case ParseResult::Failed:
                log_parse_error(parse_result, parse_slice, offset_from_input);
                error = true;
                goto AfterArgParseLoop;
            case ParseResult::Succeeded:
                dynarray_append(&cmd_args, parsed_value);
                break;
            case ParseResult::Eof:
                goto AfterArgParseLoop;
        }

        tokstate.current += parse_result.parse_offset;
    }
AfterArgParseLoop:

    if (!error)
    {
        exec_command(prgstate, first_token.text, cmd_args);
    }

    dynarray_deinit(&cmd_args);
}


void run_terminal_json_cli(ProgramState *prgstate)
{
    for (;;)
    {
        char *input = linenoise(">> ");

        if (!input)
        {
            break;
        }

        append_log(input);

        tokenizer::State tokstate;
        tokenizer::init(&tokstate, input);

        tokenizer::Token first_token = tokenizer::read_string(&tokstate);

        if (first_token.type == tokenizer::TokenType::Eof)
        {
            std::free(input);
            continue;
        }

        process_console_input(prgstate, str_slice(input));

        std::free(input);
    }
}




class CliHistory
{
public:

    CliHistory()
        : input_entries()
        , saved_input_buf()
        , pos(0)
        , saved_cursor_pos(0)
        , saved_selection_start(0)
        , saved_selection_end(0)
        {
        }

    void to_front();
    void add(StrSlice input);
    void restore(s64 position, ImGuiTextEditCallbackData *data);
    void backward(ImGuiTextEditCallbackData *data);
    void forward(ImGuiTextEditCallbackData *data);



    DynArray<Str> input_entries;
    Str saved_input_buf;
    s64 pos;
    s32 saved_cursor_pos;
    s32 saved_selection_start;
    s32 saved_selection_end;
};


void CliHistory::to_front()
{
    if (this->saved_input_buf.capacity)
    {
        str_free(&this->saved_input_buf);
    }
    this->pos = -1;
}


void CliHistory::add(StrSlice input)
{
    if (!this->input_entries.data)
    {
        dynarray_init(&this->input_entries, 64);
    }
    dynarray_append(&this->input_entries, str(input));
    this->to_front();
}


void CliHistory::restore(s64 position, ImGuiTextEditCallbackData* data)
{
    if (this->input_entries.count == 0)
    {
        return;
    }

    Str *new_input = &this->input_entries[DYNARRAY_COUNT(position)];
    memcpy(data->Buf, new_input->data, new_input->length);
    data->Buf[new_input->length] = '\0';
    data->BufTextLen = data->SelectionStart = data->SelectionEnd = data->CursorPos = new_input->length;
    data->BufDirty = true;
}


void CliHistory::backward(ImGuiTextEditCallbackData* data)
{
    s64 prev_pos = this->pos;

    if (this->pos == -1)
    {
        this->pos = this->input_entries.count - 1;
        // str_overwrite(this->saved_input_buf, data->Buf);
        this->saved_input_buf = str(data->Buf);
        this->saved_cursor_pos = data->CursorPos;
        this->saved_selection_start = data->SelectionStart;
        this->saved_selection_end = data->SelectionEnd;
    }
    else if (this->pos > 0)
    {
        --this->pos;
    }

    if (prev_pos != this->pos)
    {
        this->restore(this->pos, data);
    }
}


void CliHistory::forward(ImGuiTextEditCallbackData* data)
{
    s64 prev_pos = this->pos;

    if (this->pos >= 0)
    {
        ++this->pos;
    }

    if (prev_pos == this->input_entries.count - 1)
    {
        memcpy(data->Buf, this->saved_input_buf.data, this->saved_input_buf.length);
        data->Buf[this->saved_input_buf.length] = '\0';
        data->BufTextLen = this->saved_input_buf.length;
        data->CursorPos = this->saved_cursor_pos;
        data->SelectionStart = this->saved_selection_start;
        data->SelectionEnd = this->saved_selection_end;
        data->BufDirty = true;

        this->to_front();
    }
    else if (prev_pos != this->pos)
    {
        this->restore(this->pos, data);
    }
}


int imgui_cli_history_callback(ImGuiTextEditCallbackData *data)
{
    CliHistory *hist = (CliHistory *)data->UserData;

    if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory)
    {
        return 0;
    }

    if (data->EventKey == ImGuiKey_UpArrow)
    {
        hist->backward(data);
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
        hist->forward(data);
    }

    return 0;
}


void draw_imgui_json_cli(ProgramState *prgstate, SDL_Window *window)
{
    static float scroll_y = 0;
    static bool highlight_output_mode = false;
    static bool first_draw = true;
    static CliHistory history;
    // if (first_draw)
    // {
        // clihistory_init(&history);
    // }

    ImGuiWindowFlags console_wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoTitleBar
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImGui_SDLWindowSize(window).scaled(0.5), ImGuiSetCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("ConsoleWindow###jsoneditor-console", nullptr, console_wndflags);

    ImGui::TextUnformatted("This is the console.");
    ImGui::Separator();

    ImGui::BeginChild("Output",
                      ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()),
                      false,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);


    ImGuiInputTextFlags output_flags = ImGuiInputTextFlags_ReadOnly;
    Str *biglog = concatenated_log();

    if (highlight_output_mode)
    {
        ImGui::InputTextMultiline("##log-output", biglog->data, biglog->length, ImVec2(-1, -1), output_flags);

        if ((! (ImGui::IsWindowHovered() || ImGui::IsItemHovered()) && ImGui::IsMouseReleased(0))
            || ImGui::IsKeyReleased(SDLK_TAB))
        {
            highlight_output_mode = false;
        }
    }
    else
    {
        ImGui::TextUnformatted(biglog->data, biglog->data + biglog->length);
        scroll_y = ImGui::GetItemRectSize().y;
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
        {
            highlight_output_mode = true;
        }
        ImGui::SetScrollY(scroll_y);
    }



    ImGui::EndChild();

    static char input_buf[1024];
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (first_draw)
    {
        ImGui::SetKeyboardFocusHere();
    }
    if (ImGui::InputText("##input", input_buf, sizeof(input_buf), input_flags, imgui_cli_history_callback, &history))
    {
        StrSlice input_slice = str_slice(input_buf);
        if (input_slice.length > 0)
        {
            history.add(input_slice);
            log(input_buf);

            process_console_input(prgstate, input_slice);

            input_buf[0] = '\0';
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    first_draw = false;
}


void draw_collection_editor(ProgramState *prgstate)
{
    UNUSED(prgstate);

    ImGuiWindowFlags wndflags = 0
        | ImGuiWindowFlags_NoSavedSettings
        // | ImGuiWindowFlags_NoMove
        // | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiSetCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("Collection Editor", nullptr, wndflags);

    if (prgstate->collection.count > 0)
    {
        // collect and render column names
        DynArrayCount column_count = 0;
        {
            Value *value = &prgstate->collection[0].value;

            column_count = value->compound_value.members.count;

            ImGui::Columns((int)column_count, "Data Yay");
            ImGui::Separator();

            Str col_label = {};
            for (DynArrayCount i = 0; i < column_count; ++i)
            {
                str_overwrite(&col_label, str_slice(value->compound_value.members[i].name));
                ImGui::Text("%s", col_label.data);
                ImGui::NextColumn();
            }
            str_free(&col_label);
            ImGui::Separator();
        }

        for (DynArrayCount i = 0; i < prgstate->collection.count; ++i)
        {
            ImGui::PushID((int)i);

            Value *value = &prgstate->collection[i].value;
            TypeDescriptor *value_type = value->typedesc;
            assert(value_type->type_id == TypeID::Compound);

            for (DynArrayCount j = 0, memcount = value_type->compound_type.members.count;
                 j < memcount;
                 ++j)
            {
                ImGui::PushID((int)j);

                Value *memval = &value->compound_value.members[j].value;
                TypeDescriptor *memtype = memval->typedesc;
                TYPESWITCH (memtype->type_id)
                {
                    case TypeID::String:
                        ImGui::InputText("##field", memval->str_val.data, memval->str_val.capacity);
                        break;
                    case TypeID::Int:
                        ImGui::InputInt("##field", &memval->s32_val);
                        break;
                    case TypeID::Float:
                        ImGui::InputFloat("##field", &memval->f32_val);
                        break;
                    case TypeID::Bool:
                        ImGui::Checkbox("##field", &memval->bool_val);
                        break;
                    default:
                        ImGui::Text("TODO: %s", TypeID::to_string(memtype->type_id));
                        break;
                }

                ImGui::PopID();
                ImGui::NextColumn();
            }

            ImGui::PopID();
        }


        // // get max columns
        // for (DynArrayCount j = 0; i < prgstate->collection.count; ++i)
        // {
        //     Value *value = &prgstate->collection[i].value;
        //     TypeDescriptor *typedesc = get_typedesc(value->typeref);

        //     if (typedesc->type_id == TypeID::Compount)
        //     {
        //         column_count = max(column_count, value->compound_value.members.count);
        //     }
        // }

        // ImGui::Columns(column_count);

        ImGui::Columns(1);
        ImGui::Separator();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}


void log_display_info()
{
    int num_displays = SDL_GetNumVideoDisplays();
    logf_ln("Num displays: %i", num_displays);

    for (int i = 0; i < num_displays; ++i) {
        SDL_DisplayMode display_mode;
        SDL_GetCurrentDisplayMode(i, &display_mode);
        const char *display_name = SDL_GetDisplayName(i);
        logf_ln("Display[%i]: %s (%i x %i @ %i hz)",
                i, display_name, display_mode.w, display_mode.h, display_mode.refresh_rate);
    }
}


int main(int argc, char **argv)
{
    mem::memory_init(logf_with_userdata, nullptr);
    FormatBuffer::set_default_flush_fn(log_write_with_userdata, nullptr);

    ProgramState prgstate;
    prgstate_init(&prgstate);
    init_cli_commands(&prgstate);
    mem::default_allocator()->log_allocations();
    test_json_import(&prgstate, argc - 1, argv + 1);

    // TTY console
    // run_terminal_cli(&prgstate);
    run_terminal_json_cli(&prgstate);

    if (0 != SDL_Init(SDL_INIT_VIDEO))
    {
        logf_ln("Failed to initialize SDL: %s", SDL_GetError());
        end_of_program();
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    log_display_info();
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(0, &display_mode);
    SDL_Window *window = SDL_CreateWindow("jsoneditor",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          (int)(display_mode.w * 0.75), (int)(display_mode.h * 0.75),
                                          SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    u32 window_id = SDL_GetWindowID(window);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImVec4 clear_color = ImColor(114, 144, 154);
    ImGui_ImplSdl_Init(window);

    bool show_imgui_testwindow = false;
    bool running = true;
    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (!ImGui_ImplSdl_ProcessEvent(&event))
            {
                // event not handled by imgui
                switch (event.type)
                {
                    case SDL_QUIT:
                        running = false;
                        break;

                    case SDL_WINDOWEVENT:
                        if (event.window.windowID == window_id &&
                            event.window.event == SDL_WINDOWEVENT_CLOSE)
                        {
                            running = false;
                        }
                        break;
                }
            }
        }


        ImGui_ImplSdl_NewFrame(window);

        draw_imgui_json_cli(&prgstate, window);

        draw_collection_editor(&prgstate);

        ImGui::ShowTestWindow(&show_imgui_testwindow);


        glViewport(0, 0,
                   (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_Delay(1);
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    end_of_program();
    return 0;
}
