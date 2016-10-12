#include "typesys_json.h"
#include "typesys.h"
#include "json.h"
#include "json_error.h"
#include "logging.h"
#include "programstate.h"
#include "tokenizer.h"
#include "formatbuffer.h"

TypeDescriptor *typedesc_from_json_array(ProgramState *prgstate, json_value_s *jv)
{
    TypeDescriptor *result;

    assert(jv->type == json_type_array);
    json_array_s *jarray = (json_array_s *)jv->payload;

    DynArray<TypeDescriptor *> element_types;
    dynarray::init(&element_types, DYNARRAY_COUNT(jarray->length));

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
            dynarray::append(&element_types, typedesc);
        }
    }

    TypeDescriptor constructed_typedesc;
    constructed_typedesc.type_id = TypeID::Array;

    if (element_types.count > 1)
    {
        // Element type is Union

        UnionType union_type;
        union_type.type_cases = dynarray::clone(&element_types);

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
    if (!new_type_added)
    {
        free_typedescriptor_components(&constructed_typedesc);
    }

    dynarray::deinit(&element_types);

    return result;
}


TypeDescriptor *typedesc_from_json_object(ProgramState *prgstate, json_value_s *jv)
{
    TypeDescriptor *result;

    assert(jv->type == json_type_object);
    json_object_s *jobj = (json_object_s *)jv->payload;

    DynArray<CompoundTypeMember> members;
    dynarray::init(&members, DYNARRAY_COUNT(jobj->length));

    for (json_object_element_s *elem = jobj->start;
         elem;
         elem = elem->next)
    {
        CompoundTypeMember *member = dynarray::append(&members);
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


Value create_array_with_type_from_json(ProgramState *prgstate, json_array_s *jarray, TypeDescriptor *typedesc)
{
    Value result;
    result.typedesc = typedesc;

    dynarray::init(&result.array_value.elements, DYNARRAY_COUNT(jarray->length));
    TypeDescriptor *elem_typedesc = typedesc->array_type.elem_type;

    DynArrayCount member_idx = 0;
    for (json_array_element_s *jelem = jarray->start;
         jelem;
         jelem = jelem->next)
    {
        Value *value_element = dynarray::append(&result.array_value.elements);

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
    dynarray::init(&result.compound_value.members, typedesc->compound_type.members.count);

    DynArrayCount member_idx = 0;
    for (json_object_element_s *jelem = jobj->start;
         jelem;
         jelem = jelem->next)
    {
        CompoundTypeMember *type_member = &typedesc->compound_type.members[member_idx];
        CompoundValueMember *value_member = dynarray::append(&result.compound_value.members);
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
            ASSERT(tIS_ARRAY(typedesc));
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



JsonParseResult try_parse_json_as_value(OUTPARAM Value *output, ProgramState *prgstate,
                                        const char *input, size_t input_length)
{
    JsonParseResult result = {};

    if (input_length == 0)
    {
        result.status = JsonParseResult::Eof;
        return result;
    }

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    json_parse_result_s jp_result = {};
    json_value_s *jv = json_parse_ex(input, input_length,
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

        result.status = JsonParseResult::Failed;
    }
    else if (! jv)
    {
        // if jv was null, that means end of input,
        // but need to check for error first
        result.status = JsonParseResult::Eof;
    }
    else
    {
        *output = create_value_from_json(prgstate, jv);
        result.status = JsonParseResult::Succeeded;
    }

    return result;
}


// TODO(mike): Should probably be its own return type instead
// of piggybacking on the JsonParseResult
JsonParseResult load_json_dir(OUTPARAM Collection **pcollection, ProgramState *prgstate,
                              const char *path, size_t path_length)
{
    ASSERT(pcollection);
    JsonParseResult result = {};
    result.status = JsonParseResult::Succeeded;

    Collection *collection = bucketarray::add(&prgstate->collections).elem;
    collection->load_path = str(path, STRLEN(path_length));
    dynarray::init(&collection->records, 0);
    *pcollection = collection;

    DirLister dirlist(path, path_length);

    bool error = false;

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
            JsonParseResult parse_result = try_parse_json_as_value(OUTPARAM &parsed_value, prgstate,
                                                                   filecontents.data, filecontents.length);

            switch (parse_result.status)
            {
                case JsonParseResult::Eof:
                    break;

                case JsonParseResult::Failed:
                {
                    result = parse_result;
                    FormatBuffer errorfmt;
                    errorfmt.writef("error parsing json file %s: %s", dirlist.current.name.data, result.error_desc.data);
                    result.error_desc = str(errorfmt.buffer, STRLEN(errorfmt.cursor));
                    error = true;
                    break;
                }
                case JsonParseResult::Succeeded:
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
                        LoadedRecord *lr = bucketarray::add(&prgstate->loaded_records).elem;
                        lr->fullpath = fullpath;
                        lr->value = parsed_value;
                        dynarray::append(&collection->records, lr);
                    }
                    break;
                }
            }
        }
    }

    return result;
}


