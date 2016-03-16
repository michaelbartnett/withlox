#include "logging.h"

#include "pretty.h"
#include "types.h"
#include "hashtable.h"
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
#include <SDL.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

// Debugging
// http://stackoverflow.com/questions/312312/what-are-some-reasons-a-release-build-would-run-differently-than-a-debug-build
// http://www.codeproject.com/Articles/548/Surviving-the-Release-Version


// Type theory
// http://chris-taylor.github.io/blog/2013/02/10/the-algebra-of-algebraic-data-types/
// https://en.wikipedia.org/wiki/Algebraic_data_type
// https://en.wikipedia.org/wiki/Generalized_algebraic_data_type


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


#include "tokenizer.h"
#include <algorithm>

struct ProgramMemory;

#define CLI_COMMAND_FN_SIG(name) void name(ProgramMemory *prgmem, void *userdata, DynArray<Value> args)
typedef CLI_COMMAND_FN_SIG(CliCommandFn);


struct CliCommand
{
    CliCommandFn *fn;
    void *userdata;
};


typedef OAHashtable<StrSlice, Value, StrSliceEqual, StrSliceHash> StrToValueMap;

struct ProgramMemory
{
    NameTable names;
    DynArray<TypeDescriptor> type_descriptors;
    OAHashtable<NameRef, TypeDescriptorRef> typedesc_bindings;

    TypeDescriptorRef prim_string;
    TypeDescriptorRef prim_int;
    TypeDescriptorRef prim_float;
    TypeDescriptorRef prim_bool;
    TypeDescriptorRef prim_none;

    OAHashtable<StrSlice, CliCommand, StrSliceEqual, StrSliceHash> command_map;
    StrToValueMap value_map;

    u8 *perm_store;
    size_t perm_store_size;
    size_t perm_store_allocated;
};


void *alloc_perm(ProgramMemory *prgmem, size_t size)
{
    assert(size);
    assert(prgmem->perm_store_allocated < prgmem->perm_store_allocated + size);
    assert(prgmem->perm_store_size > prgmem->perm_store_allocated + size);

    void *result = prgmem->perm_store + prgmem->perm_store_allocated;
    prgmem->perm_store_allocated += size;
    return result;
}

TypeDescriptorRef add_typedescriptor(ProgramMemory *prgmem, TypeDescriptor **ptr);


void bind_typeref(ProgramMemory *prgmem, NameRef name, TypeDescriptorRef typeref)
{
    ht_set(&prgmem->typedesc_bindings, name, typeref);
}


void bind_typeref(ProgramMemory *prgmem, const char *name, TypeDescriptorRef typeref)
{
    NameRef nameref = nametable_find_or_add(&prgmem->names, str_slice(name));
    bind_typeref(prgmem, nameref, typeref);
}


void prgmem_init(ProgramMemory *prgmem)
{
    nametable_init(&prgmem->names, MEGABYTES(2));
    dynarray_init(&prgmem->type_descriptors, 0);
    append(&prgmem->type_descriptors);
    ht_init(&prgmem->typedesc_bindings);

    TypeDescriptor *none_type;
    prgmem->prim_none = add_typedescriptor(prgmem, &none_type);
    none_type->type_id = TypeID::None;
    bind_typeref(prgmem, TypeID::to_string(none_type->type_id),
                 prgmem->prim_none);

    TypeDescriptor *string_type;
    prgmem->prim_string = add_typedescriptor(prgmem, &string_type);
    string_type->type_id = TypeID::String;
    bind_typeref(prgmem, TypeID::to_string(string_type->type_id),
                 prgmem->prim_string);

    TypeDescriptor *int_type;
    prgmem->prim_int = add_typedescriptor(prgmem, &int_type);
    int_type->type_id = TypeID::Int;
    bind_typeref(prgmem, TypeID::to_string(int_type->type_id),
                 prgmem->prim_int);

    TypeDescriptor *float_type;
    prgmem->prim_float = add_typedescriptor(prgmem, &float_type);
    float_type->type_id = TypeID::Float;
    bind_typeref(prgmem, TypeID::to_string(float_type->type_id),
                 prgmem->prim_float);

    TypeDescriptor *bool_type;
    prgmem->prim_bool = add_typedescriptor(prgmem, &bool_type);
    bool_type->type_id = TypeID::Bool;
    bind_typeref(prgmem, TypeID::to_string(bool_type->type_id),
                 prgmem->prim_bool);

    ht_init(&prgmem->command_map);
    ht_init(&prgmem->value_map);
}


TypeDescriptorRef make_typeref(ProgramMemory *prgmem, u32 index)
{
    TypeDescriptorRef result = {index, &prgmem->type_descriptors};
    return result;
}


TypeDescriptorRef add_typedescriptor(ProgramMemory *prgmem, TypeDescriptor type_desc, TypeDescriptor **ptr)
{
    TypeDescriptor *td = append(&prgmem->type_descriptors, type_desc);

    if (ptr)
    {
        *ptr = td;
    }

    TypeDescriptorRef result = make_typeref(prgmem,
                                            prgmem->type_descriptors.count - 1);
    return result;
}


TypeDescriptorRef add_typedescriptor(ProgramMemory *prgmem, TypeDescriptor **ptr)
{
    TypeDescriptor td = {};
    return add_typedescriptor(prgmem, td, ptr);
}


// TODO(mike): Guarantee no structural duplicates, and do it fast. Hashtable.
TypeDescriptorRef find_equiv_typedescriptor(ProgramMemory *prgmem, TypeDescriptor *type_desc)
{
    TypeDescriptorRef result = {};

    switch ((TypeID::Tag)type_desc->type_id)
    {
        // Builtins/primitives
        case TypeID::None:   result =  prgmem->prim_none;   break;
        case TypeID::String: result =  prgmem->prim_string; break;
        case TypeID::Int:    result =  prgmem->prim_int;    break;
        case TypeID::Float:  result =  prgmem->prim_float;  break;
        case TypeID::Bool:   result =  prgmem->prim_bool;   break;


        // Unique / user-defined
        case TypeID::Array:
        case TypeID::Compound:
            for (DynArrayCount i = 0,
                     count = prgmem->type_descriptors.count;
                 i < count;
                 ++i)
            {
                TypeDescriptorRef ref = make_typeref(prgmem, i);
                TypeDescriptor *predefined = get_typedesc(ref);
                if (predefined && equal(type_desc, predefined))
                {
                    result = ref;
                    break;
                }
            }
            break;
    }

    return result;
}


TypeDescriptorRef find_typeref_by_name(ProgramMemory *prgmem, NameRef name)
{
    TypeDescriptorRef result = {};
    TypeDescriptorRef *typeref = ht_find(&prgmem->typedesc_bindings, name);
    if (typeref)
    {
        result = *typeref;
    }
    return result;
}

TypeDescriptorRef find_typeref_by_name(ProgramMemory *prgmem, StrSlice name)
{
    TypeDescriptorRef result = {};
    NameRef nameref = nametable_find(&prgmem->names, name);
    if (nameref.offset)
    {
        result = find_typeref_by_name(prgmem, nameref);
    }
    return result;
}

TypeDescriptorRef find_typeref_by_name(ProgramMemory *prgmem, Str name)
{
    return find_typeref_by_name(prgmem, str_slice(name));
}


TypeDescriptor *find_typedesc_by_name(ProgramMemory *prgmem, NameRef name)
{
    return get_typedesc(find_typeref_by_name(prgmem, name));
}


TypeDescriptorRef type_desc_from_json(ProgramMemory *prgmem, json_value_s *jv);


TypeDescriptorRef type_desc_from_json_array(ProgramMemory *prgmem, json_value_s *jv)
{
    TypeDescriptorRef result;

    assert(jv->type == json_type_array);
    json_array_s *jarray = (json_array_s *)jv->payload;

    DynArray<TypeDescriptorRef> element_types;
    assert(jarray->length < DYNARRAY_COUNT_MAX);
    dynarray_init(&element_types, (DynArrayCount)jarray->length);

    for (json_array_element_s *elem = jarray->start;
         elem;
         elem = elem->next)
    {
        TypeDescriptorRef typeref = type_desc_from_json(prgmem, elem->value);

        // This is a set insertion
        bool found = false;
        for (DynArrayCount i = 0; i < element_types.count; ++i)
        {
            TypeDescriptorRef *existing = get(element_types, i);
            if (typedesc_ref_identical(typeref, *existing))
            {
                found = true;
                break;
            }
        }

        if (! found)
        {
            append(&element_types, typeref);
        }
    }

    if (element_types.count > 1)
    {
        logln("Ecountered a heterogeneous json array. "
              "It will be reported as an Array of None type since this is not yet supported");

        set(&element_types, 0, prgmem->prim_none);
    }
    // else
    {
        TypeDescriptor constructed_typedesc;
        constructed_typedesc.type_id = TypeID::Array;

        if (element_types.count == 0)
        {
            // empty,  untyped array...?
            logln("Got an empty json array. This defaults to type [None], but I don't like it!");
            constructed_typedesc.array_type.elem_typedesc_ref = prgmem->prim_none;
        }
        else
        {
            constructed_typedesc.array_type.elem_typedesc_ref = *get(element_types, 0);
        }


        // If it's the empty array, bound to find a preexisting. Will that be common?
        TypeDescriptorRef preexisting = find_equiv_typedescriptor(prgmem, &constructed_typedesc);
        if (preexisting.index)
        {
            result = preexisting;
        }
        else
        {
            result = add_typedescriptor(prgmem, constructed_typedesc, 0);
            logln("Added array type descriptor");
        }
    }

    dynarray_deinit(&element_types);

    return result;
}


TypeDescriptorRef type_desc_from_json_object(ProgramMemory *prgmem, json_value_s *jv)
{
    TypeDescriptorRef result;

    assert(jv->type == json_type_object);
    json_object_s *jobj = (json_object_s *)jv->payload;
    assert(jobj->length < UINT32_MAX);

    DynArray<TypeMember> members;
    dynarray_init(&members, (u32)jobj->length);

    for (json_object_element_s *elem = jobj->start;
         elem;
         elem = elem->next)
    {
        TypeMember *member = append(&members);
        member->name = nametable_find_or_add(&prgmem->names,
                                             elem->name->string, elem->name->string_size);
        member->typedesc_ref = type_desc_from_json(prgmem, elem->value);
    }

    TypeDescriptor constructed_typedesc;
    constructed_typedesc.type_id = TypeID::Compound;
    constructed_typedesc.members = members;

    TypeDescriptorRef preexisting = find_equiv_typedescriptor(prgmem, &constructed_typedesc);
    if (preexisting.index)
    {
        result = preexisting;
    }
    else
    {
        result = add_typedescriptor(prgmem, constructed_typedesc, 0);
    }

    return result;
}


TypeDescriptorRef type_desc_from_json(ProgramMemory *prgmem, json_value_s *jv)
{
    TypeDescriptorRef result;

    json_type_e jvtype = (json_type_e)jv->type;
    switch (jvtype)
    {
        case json_type_string:
            result = prgmem->prim_string;
            break;

        case json_type_number:
        {
            json_number_s *jnum = (json_number_s *)jv->payload;
            tokenizer::Token number_token = tokenizer::read_number(jnum->number, jnum->number_size);
            result = number_token.type == tokenizer::TokenType::Int ? prgmem->prim_int : prgmem->prim_float;
            break;
        }

        case json_type_object:
            result = type_desc_from_json_object(prgmem, jv);
            break;

        case json_type_array:
            result = type_desc_from_json_array(prgmem, jv);
            break;

        case json_type_true:
        case json_type_false:
            result = prgmem->prim_bool;
            break;

       case json_type_null:
           result = prgmem->prim_none;
           break;
    }

    return result;
}


TypeDescriptor *clone(const TypeDescriptor *type_desc);


DynArray<TypeMember> clone(const DynArray<TypeMember> &memberset)
{
    DynArray<TypeMember> result = dynarray_init<TypeMember>(memberset.count);

    for (u32 i = 0; i < memberset.count; ++i)
    {
        TypeMember *src_member = get(memberset, i);
        TypeMember *dest_member = append(&result);
        ZERO_PTR(dest_member);
        NameRef newname = src_member->name;
        dest_member->name = newname;
        dest_member->typedesc_ref = src_member->typedesc_ref;
    }

    return result;
}


TypeDescriptorRef clone(ProgramMemory *prgmem, const TypeDescriptor *src_typedesc, TypeDescriptor **ptr)
{
    TypeDescriptor *dest_typedesc;
    TypeDescriptorRef result = add_typedescriptor(prgmem, &dest_typedesc);
    if (ptr)
    {
        *ptr = dest_typedesc;
    }

    dest_typedesc->type_id = src_typedesc->type_id;
    switch ((TypeID::Tag)dest_typedesc->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            // nothing to do for non-compound types
            break;

        case TypeID::Array:
            dest_typedesc->array_type = src_typedesc->array_type;
            break;

        case TypeID::Compound:
            dest_typedesc->members = clone(src_typedesc->members);
            break;
    }

    return result;
}


TypeMember *find_member(const TypeDescriptor &type_desc, NameRef name)
{
    size_t count = type_desc.members.count;
    for (u32 i = 0; i < count; ++i)
    {
        TypeMember *mem = get(type_desc.members, i);

        if (nameref_identical(mem->name, name))
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
    new_member->typedesc_ref = member.typedesc_ref;
}


// NOTE(mike): Merging *any* type descriptor only makes sense if we have sum types
// That would be cool, but might be a bit much. We'll see.
// TypeDescriptor *merge_type_descriptors(TypeDescriptor *typedesc_a, TypeDescriptor *typedesc_b)

TypeDescriptorRef merge_compound_types(ProgramMemory *prgmem,
                                       const TypeDescriptor *typedesc_a,
                                       const TypeDescriptor *typedesc_b,
                                       TypeDescriptor **ptr)
{
    assert(typedesc_a);
    assert(typedesc_b);
    assert(typedesc_a->type_id == TypeID::Compound);
    assert(typedesc_b->type_id == TypeID::Compound);

    TypeDescriptor *result_ptr;
    TypeDescriptorRef result_ref = clone(prgmem, typedesc_a, &result_ptr);

    for (DynArrayCount ib = 0; ib < typedesc_b->members.count; ++ib)
    {
        TypeMember *b_member = get(typedesc_b->members, ib);
        TypeMember *a_member = find_member(*typedesc_a, b_member->name);

        if (! (a_member && typedesc_ref_identical(a_member->typedesc_ref, b_member->typedesc_ref)))
        {
            add_member(result_ptr, *b_member);
        }
    }

    if (ptr)
    {
        *ptr = result_ptr;
    }
    return result_ref;
}


Value create_value_from_token(ProgramMemory *prgmem, tokenizer::Token token)
{
    // good test: {"tulsi": "gabbard", "julienne": { "fries": 42.2, "asponge": {}, "hillarymoney": 9001}}
    Str token_copy = str(token.text);
    Value result = {};

    switch (token.type)
    {
        case tokenizer::TokenType::Eof:
        case tokenizer::TokenType::Unknown:
            result.typedesc_ref = prgmem->prim_none;
            break;

        case tokenizer::TokenType::String:
            result.str_val = token_copy;
            result.typedesc_ref = prgmem->prim_string;
            // indicate no need to free
            token_copy.data = 0;
            break;

        case tokenizer::TokenType::Int:
            result.s32_val = atoi(token_copy.data);
            result.typedesc_ref = prgmem->prim_int;
            break;

        case tokenizer::TokenType::Float:
            result.f32_val = (float)atof(token_copy.data);
            result.typedesc_ref = prgmem->prim_float;
            break;

        case tokenizer::TokenType::True:
            result.typedesc_ref = prgmem->prim_bool;
            result.bool_val = true;
            break;

        case tokenizer::TokenType::False:
            result.typedesc_ref = prgmem->prim_bool;
            result.bool_val = false;
            break;
    }

    if (token_copy.data)
    {
        str_free(&token_copy);
    }
    return result;
}


Value create_value_from_json(ProgramMemory *prgmem, json_value_s *jv);

Value create_object_with_type_from_json(ProgramMemory *prgmem, json_object_s *jobj, TypeDescriptorRef typeref);


Value create_array_with_type_from_json(ProgramMemory *prgmem, json_array_s *jarray, TypeDescriptorRef typeref)
{
    Value result;
    result.typedesc_ref = typeref;

    TypeDescriptor *type_desc = get_typedesc(typeref);
    assert(jarray->length < DYNARRAY_COUNT_MAX);
    dynarray_init(&result.array_value.elements, (DynArrayCount)jarray->length);
    TypeDescriptorRef elem_typeref = type_desc->array_type.elem_typedesc_ref;
    TypeDescriptor *elem_typedesc = get_typedesc(elem_typeref);

    DynArrayCount member_idx = 0;
    for (json_array_element_s *jelem = jarray->start;
         jelem;
         jelem = jelem->next)
    {
        Value *value_element = append(&result.array_value.elements);

        switch ((TypeID::Tag)elem_typedesc->type_id)
        {
            case TypeID::None:
            case TypeID::String:
            case TypeID::Int:
            case TypeID::Float:
            case TypeID::Bool:
                *value_element = create_value_from_json(prgmem, jelem->value);
                break;

            case TypeID::Array:
                assert(jelem->value->type == json_type_array);
                *value_element =
                    create_array_with_type_from_json(prgmem,
                                                     (json_array_s *)jelem->value->payload,
                                                     elem_typeref);
                break;

            case TypeID::Compound:
                assert(jelem->value->type == json_type_object);
                *value_element =
                    create_object_with_type_from_json(prgmem,
                                                      (json_object_s *)jelem->value->payload,
                                                      elem_typeref);
                break;
        }

        ++member_idx;
    }

    return result;
}


Value create_object_with_type_from_json(ProgramMemory *prgmem, json_object_s *jobj, TypeDescriptorRef typeref)
{
    Value result;

    result.typedesc_ref = typeref;
    TypeDescriptor *type_desc = get_typedesc(typeref);
    dynarray_init(&result.members, type_desc->members.count);

    DynArrayCount member_idx = 0;
    for (json_object_element_s *jelem = jobj->start;
         jelem;
         jelem = jelem->next)
    {
        TypeMember *type_member = get(&type_desc->members, member_idx);
        ValueMember *value_member = append(&result.members);
        value_member->name = type_member->name;

        TypeDescriptor *type_member_desc = get_typedesc(type_member->typedesc_ref);

        switch ((TypeID::Tag)type_member_desc->type_id)
        {
            case TypeID::None:
            case TypeID::String:
            case TypeID::Int:
            case TypeID::Float:
            case TypeID::Bool:
                value_member->value = create_value_from_json(prgmem, jelem->value);
                break;

            case TypeID::Array:
                value_member->value =
                    create_array_with_type_from_json(prgmem,
                                                     (json_array_s *)jelem->value->payload,
                                                     type_member->typedesc_ref);
                break;

            case TypeID::Compound:
                assert(jelem->value->type == json_type_object);
                value_member->value =
                    create_object_with_type_from_json(prgmem,
                                                      (json_object_s *)jelem->value->payload,
                                                      type_member->typedesc_ref);
                break;
        }

        ++member_idx;
    }

    return result;
}


Value create_value_from_json(ProgramMemory *prgmem, json_value_s *jv)
{
    Value result;

    switch ((json_type_e)jv->type)
    {
        case json_type_string:
        {
            json_string_s *jstr = (json_string_s *)jv->payload;
            result.typedesc_ref = prgmem->prim_string;
            assert(jstr->string_size < STR_LENGTH_MAX);
            StrLen length = (StrLen)jstr->string_size;
            result.str_val = str(jstr->string, length);
            break;
        }

        case json_type_number:
        {
            json_number_s *jnum = (json_number_s *)jv->payload;
            tokenizer::Token number_token = tokenizer::read_number(jnum->number, jnum->number_size);

            result = create_value_from_token(prgmem, number_token);
            break;
        }

        case json_type_object:
        {
            // result.typedesc_ref = prgmem->
            TypeDescriptorRef typeref = type_desc_from_json_object(prgmem, jv);
            json_object_s *jobj = (json_object_s *)jv->payload;
            result = create_object_with_type_from_json(prgmem, jobj, typeref);
            break;
        }

        case json_type_array:
        {
            TypeDescriptorRef typeref = type_desc_from_json_array(prgmem, jv);
            assert(get_typedesc(typeref)->type_id == TypeID::Array);
            json_array_s *jarray = (json_array_s *)jv->payload;
            result = create_array_with_type_from_json(prgmem, jarray, typeref);
            break;
        }

        case json_type_true:
            result.typedesc_ref = prgmem->prim_bool;
            result.bool_val = true;
            break;

        case json_type_false:
            result.typedesc_ref = prgmem->prim_bool;
            result.bool_val = false;
            break;

        case json_type_null:
            result.typedesc_ref = prgmem->prim_none;
            break;
    }

    return result;
}


void register_command(ProgramMemory *prgmem, StrSlice name, CliCommandFn *fnptr, void *userdata)
{
    assert(fnptr);
    CliCommand cmd = {fnptr, userdata};
    Str allocated_name = str(name);
    if (ht_set(&prgmem->command_map, str_slice(allocated_name), cmd))
    {
        logf_ln("Warning, overriding command: %s", allocated_name.data);
        str_free(&allocated_name);
    }
}


void register_command(ProgramMemory *prgmem, const char *name, CliCommandFn *fnptr, void *userdata)
{
    register_command(prgmem, str_slice(name), fnptr, userdata);
}

#define REGISTER_COMMAND(prgmem, fn_ident, userdata) register_command((prgmem), # fn_ident, &fn_ident, userdata)


void exec_command(ProgramMemory *prgmem, StrSlice name, DynArray<Value> args)
{
    CliCommand *cmd = ht_find(&prgmem->command_map, name);

    if (!cmd)
    {
        logf_ln("Command '%s' not found", name.data);
        return;
    }

    cmd->fn(prgmem, cmd->userdata, args);
}


CLI_COMMAND_FN_SIG(say_hello)
{
    UNUSED(prgmem);
    UNUSED(userdata);
    UNUSED(args);

    logln("You are calling the say_hello command");
}


CLI_COMMAND_FN_SIG(list_args)
{
    UNUSED(prgmem);
    UNUSED(userdata);

    logln("list_args:");
    for (u32 i = 0; i < args.count; ++i)
    {
        Value *val = get(&args, i);
        pretty_print(val->typedesc_ref);
        pretty_print(val);
    }
}


CLI_COMMAND_FN_SIG(find_type)
{
    UNUSED(userdata);

    Value *arg = get(args, 0);

    if (! typedesc_ref_identical(arg->typedesc_ref, prgmem->prim_string))
    {
        TypeDescriptor *argtype = get_typedesc(arg->typedesc_ref);
        logf_ln("Argument must be a string, got a %s intead",
                  TypeID::to_string(argtype->type_id));
        return;
    }

    TypeDescriptorRef typeref = find_typeref_by_name(prgmem, arg->str_val);
    if (typeref.index)
    {
        pretty_print(typeref);
    }
    else
    {
        logf_ln("Type not found: %s", arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(set_value)
{
    UNUSED(userdata);

    if (args.count < 2)
    {
        logf_ln("Error: expected 2 arguments, got %i instead", args.count);
        return;
    }

    Value *name_arg = get(args, 0);

    TypeDescriptor *type_desc = get_typedesc(name_arg->typedesc_ref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    // This is maybe a bit too manual, but I want everything to be POD, no destructors
    StrToValueMap::Entry *entry;
    bool was_occupied = ht_find_or_add_entry(&entry, &prgmem->value_map, str_slice(name_arg->str_val));
    if (!was_occupied)
    {
        // allocate dedicated space
        Str allocated = str(entry->key);
        entry->key = str_slice(allocated);
    }

    entry->value = clone(get(args, 1));

    FormatBuffer fbuf;
    fbuf.flush_on_destruct();
    fbuf.writeln("Storing value:");
    pretty_print(&entry->value, &fbuf);
}


CLI_COMMAND_FN_SIG(get_value)
{
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("Error: expected 1 argument");
        return;
    }

    Value *name_arg = get(args, 0);
    TypeDescriptor *type_desc = get_typedesc(name_arg->typedesc_ref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    Value *value = ht_find(&prgmem->value_map, str_slice(name_arg->str_val));
    if (value)
    {
        pretty_print(value);
    }
    else
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(get_value_type)
{
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("Error: expected 1 argument");
        return;
    }

    Value *name_arg = get(args, 0);
    TypeDescriptor *type_desc = get_typedesc(name_arg->typedesc_ref);
    if (type_desc->type_id != TypeID::String)
    {
        logf_ln("Error: first argument must be a string, got a %s instead",
                  TypeID::to_string(type_desc->type_id));
        return;
    }

    Value *value = ht_find(&prgmem->value_map, str_slice(name_arg->str_val));
    if (value)
    {
        pretty_print(value->typedesc_ref);
    }
    else
    {
        logf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }


}


CLI_COMMAND_FN_SIG(print_values)
{
    UNUSED(prgmem);
    UNUSED(userdata);

    if (args.count < 1)
    {
        logln("No arguments");
        return;
    }

    FormatBuffer fmt_buf;
    fmt_buf.flush_on_destruct();

    for (u32 i = 0; i < args.count; ++i)
    {
        fmt_buf.writeln("");
        Value *value = get(args, i);
        fmt_buf.write("Value: ");
        pretty_print(value, &fmt_buf);
        fmt_buf.writef("Parsed value's type: %i ", value->typedesc_ref.index);
        pretty_print(value->typedesc_ref, &fmt_buf);
    }
}


void test_json_import(ProgramMemory *prgmem, int filename_count, char **filenames)
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

    TypeDescriptorRef result_ref = {};

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

        TypeDescriptorRef typedesc_ref = type_desc_from_json(prgmem, jv);
        NameRef bound_name = nametable_find_or_add(&prgmem->names, filename);
        bind_typeref(prgmem, bound_name, typedesc_ref);

        logln("New type desciptor:");
        pretty_print(typedesc_ref);

        if (!result_ref.index)
        {
            result_ref = typedesc_ref;
        }
        else
        {
            TypeDescriptor *result_ptr = get_typedesc(result_ref);
            TypeDescriptor *type_desc = get_typedesc(typedesc_ref);
            result_ref = merge_compound_types(prgmem, result_ptr, type_desc, 0);
        }
    }

    logln("completed without errors");

    pretty_print(result_ref);
}


void init_cli_commands(ProgramMemory *prgmem)
{
    REGISTER_COMMAND(prgmem, say_hello, NULL);
    REGISTER_COMMAND(prgmem, list_args, NULL);
    REGISTER_COMMAND(prgmem, find_type, NULL);
    REGISTER_COMMAND(prgmem, set_value, NULL);
    REGISTER_COMMAND(prgmem, get_value, NULL);
    REGISTER_COMMAND(prgmem, get_value_type, NULL);
    REGISTER_COMMAND(prgmem, print_values, NULL);
}


void process_console_input(ProgramMemory *prgmem, StrSlice input_buf)
{
    tokenizer::State tokstate;
    tokenizer::init(&tokstate, input_buf.data);

    tokenizer::Token first_token = tokenizer::read_string(&tokstate);

    DynArray<Value> cmd_args;
    dynarray_init(&cmd_args, 10);

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    bool error = false;

    for (;;) {
        size_t offset_from_input = (size_t)(tokstate.current - input_buf.data);
        json_parse_result_s jp_result = {};
        json_value_s *jv = json_parse_ex(tokstate.current, (size_t)(tokstate.end - tokstate.current),
                                         jsonflags, &jp_result);


        if (jp_result.error != json_parse_error_none)
        {
            FormatBuffer fmt_buf;
            fmt_buf.flush_on_destruct();

            // json.h doesn't seem to be consistent in whether it point to errors before or after the relevant character
            bool invalid_value = jp_result.error == json_parse_error_invalid_value;
            bool invalid_number = jp_result.error == json_parse_error_invalid_number_format;

            fmt_buf.writef("Json parse error: %s\nAt %lu:%lu\n%s\n",
                           json_error_code_string(jp_result.error),
                           jp_result.error_line_no,
                           jp_result.error_row_no + (jp_result.error_line_no > 1 ? 0 : offset_from_input) - (invalid_number ? 1 : 0),
                           input_buf.data);

            size_t buffer_offset = jp_result.error_offset + offset_from_input + (invalid_value ? 1 : 0);
            for (size_t i = 0; i < buffer_offset - 1; ++i) fmt_buf.write("~");
            fmt_buf.write("^");
            error =  true;
            break;
        }
        else if (jv)
        {
            Value parsed_value = create_value_from_json(prgmem, jv);
            append(&cmd_args, parsed_value);
        }
        else
        {
            // if jv was null, that means end of input
            break;
        }

        tokstate.current += jp_result.parse_offset;
    }

    if (!error)
    {
        exec_command(prgmem, first_token.text, cmd_args);
    }

    dynarray_deinit(&cmd_args);
}


void run_terminal_cli(ProgramMemory *prgmem)
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

        DynArray<Value> cmd_args;
        dynarray_init(&cmd_args, 10);

        for (;;)
        {
            tokenizer::Token token = tokenizer::read_token(&tokstate);
            if (token.type == tokenizer::TokenType::Eof)
            {
                break;
            }

            Value value = create_value_from_token(prgmem, token);

            append(&cmd_args, value);
        }

        exec_command(prgmem, first_token.text, cmd_args);

        dynarray_deinit(&cmd_args);
        std::free(input);
    }
}


void run_terminal_json_cli(ProgramMemory *prgmem)
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

        process_console_input(prgmem, str_slice(input));

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
    append(&this->input_entries, str(input));
    this->to_front();
}


void CliHistory::restore(s64 position, ImGuiTextEditCallbackData* data)
{
    if (this->input_entries.count == 0)
    {
        return;
    }

    assert(position < DYNARRAY_COUNT_MAX);
    Str *new_input = get(&this->input_entries, (u32)position);
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



void draw_imgui_json_cli(ProgramMemory *prgmem, SDL_Window *window)
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
        | ImGuiWindowFlags_NoTitleBar
        // | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        ;

    ImGui::SetNextWindowSize(ImGui_SDLWindowSize(window), ImGuiSetCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("ConsoleWindow###jsoneditor-console", NULL, console_wndflags);

    ImGui::TextUnformatted("This is the console.");
    ImGui::Separator();

    ImGui::BeginChild("Output",
                      ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()),
                      false,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_ForceHorizontalScrollbar);


    ImGuiInputTextFlags output_flags = ImGuiInputTextFlags_ReadOnly;
    Str biglog = concatenated_log();

    if (highlight_output_mode)
    {
        ImGui::InputTextMultiline("##log-output", biglog.data, biglog.length, ImVec2(-1, -1), output_flags);

        if ((! (ImGui::IsWindowHovered() || ImGui::IsItemHovered()) && ImGui::IsMouseReleased(0))
            || ImGui::IsKeyReleased(SDLK_TAB))
        {
            highlight_output_mode = false;
        }
    }
    else
    {
        ImGui::TextUnformatted(biglog.data, biglog.data + biglog.length);
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

            process_console_input(prgmem, input_slice);

            input_buf[0] = '\0';
        }
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    first_draw = false;
}


int main(int argc, char **argv)
{
    // void run_tests();
    // run_tests();
    // return 0;

    ProgramMemory prgmem;
    prgmem_init(&prgmem);
    init_cli_commands(&prgmem);

    test_json_import(&prgmem, argc - 1, argv + 1);
    // run_terminal_cli(&prgmem);
    // run_terminal_json_cli(&prgmem);

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

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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

        draw_imgui_json_cli(&prgmem, window);

        ImGui::ShowTestWindow();


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
