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


void typedescriptor_init(TypeDescriptor *type_desc, TypeID::Tag type_id)
{
    type_desc->type_id = type_id;
    dynarray_init(&type_desc->members, 0);
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
        case TypeID::None:   result =  prgmem->prim_none;   break;
        case TypeID::String: result =  prgmem->prim_string; break;
        case TypeID::Int:    result =  prgmem->prim_int;    break;
        case TypeID::Float:  result =  prgmem->prim_float;  break;
        case TypeID::Bool:   result =  prgmem->prim_bool;   break;

        case TypeID::Compound:
            for (u32 i = 0,
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

// TypeDescriptor *type_desc_from_json(ProgramMemory *prgmem, json_value_s *jv)
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
            result = prgmem->prim_float;
            break;

        case json_type_object:
        {
            json_object_s *jso = (json_object_s *)jv->payload;
            assert(jso->length < UINT32_MAX);

            DynArray<TypeMember> members;
            dynarray_init(&members, (u32)jso->length);

            for (json_object_element_s *elem = jso->start;
                 elem;
                 elem = elem->next)
            {
                TypeMember *member = append(&members);
                member->name = nametable_find_or_add(&prgmem->names,
                                                     elem->name->string, elem->name->string_size);
                member->typedesc_ref = type_desc_from_json(prgmem, elem->value);
            }

            TypeDescriptor constructed_typedesc;
            typedescriptor_init(&constructed_typedesc, TypeID::Compound);
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
            break;
        }

        case json_type_array:
            assert(0);
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

    for (u32 ib = 0; ib < typedesc_b->members.count; ++ib)
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


void register_command(ProgramMemory *prgmem, StrSlice name, CliCommandFn *fnptr, void *userdata)
{
    assert(fnptr);
    CliCommand cmd = {fnptr, userdata};
    Str allocated_name = str(name);
    if (ht_set(&prgmem->command_map, str_slice(allocated_name), cmd))
    {
        printf_ln("Warning, overriding command: %s", allocated_name.data);
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
        printf_ln("Command '%s' not found", name.data);
        return;
    }

    cmd->fn(prgmem, cmd->userdata, args);
}


CLI_COMMAND_FN_SIG(say_hello)
{
    UNUSED(prgmem);
    UNUSED(userdata);
    UNUSED(args);

    println("You are calling the say_hello command");
}


CLI_COMMAND_FN_SIG(list_args)
{
    UNUSED(prgmem);
    UNUSED(userdata);

    println("list_args:");
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
        printf_ln("Argument must be a string, got a %s intead",
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
        printf_ln("Type not found: %s", arg->str_val.data);
    }
}


CLI_COMMAND_FN_SIG(set_value)
{
    UNUSED(userdata);

    if (args.count < 2)
    {
        printf_ln("Error: expected 2 arguments, got %i instead", args.count);
        return;
    }

    Value *name_arg = get(args, 0);

    TypeDescriptor *type_desc = get_typedesc(name_arg->typedesc_ref);
    if (type_desc->type_id != TypeID::String)
    {
        printf_ln("Error: first argument must be a string, got a %s instead",
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
}


CLI_COMMAND_FN_SIG(get_value)
{
    UNUSED(userdata);

    if (args.count < 1)
    {
        println("Error: expected 1 argument");
        return;
    }

    Value *name_arg = get(args, 0);
    TypeDescriptor *type_desc = get_typedesc(name_arg->typedesc_ref);
    if (type_desc->type_id != TypeID::String)
    {
        printf_ln("Error: first argument must be a string, got a %s instead",
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
        printf_ln("No value bound to name: '%s'", name_arg->str_val.data);
    }
}


int main(int argc, char **argv)
{
    // void run_tests();
    // run_tests();
    // return 0;

    ProgramMemory prgmem;
    prgmem_init(&prgmem);

    if (argc < 2)
    {
        printf("No file specified\n");
		end_of_program();
        return 0;
    }

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    TypeDescriptorRef result_ref = {};

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

        TypeDescriptorRef typedesc_ref = type_desc_from_json(&prgmem, jv);
        NameRef bound_name = nametable_find_or_add(&prgmem.names, filename);
        bind_typeref(&prgmem, bound_name, typedesc_ref);

        println("New type desciptor:");
        pretty_print(typedesc_ref);

        if (!result_ref.index)
        {
            result_ref = typedesc_ref;
        }
        else
        {
            TypeDescriptor *result_ptr = get_typedesc(result_ref);
            TypeDescriptor *type_desc = get_typedesc(typedesc_ref);
            result_ref = merge_compound_types(&prgmem, result_ptr, type_desc, 0);
        }
    }

    println("completed without errors");

    pretty_print(result_ref);

    REGISTER_COMMAND(&prgmem, say_hello, NULL);
    REGISTER_COMMAND(&prgmem, list_args, NULL);
    REGISTER_COMMAND(&prgmem, find_type, NULL);
    REGISTER_COMMAND(&prgmem, set_value, NULL);
    REGISTER_COMMAND(&prgmem, get_value, NULL);

    for (;;)
    {
        char *input = linenoise(">> ");

        if (!input)
        {
            break;
        }

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

            Value value = create_value_from_token(&prgmem, token);
            // pretty_print(value.typedesc_ref);

            append(&cmd_args, value);
        }

        exec_command(&prgmem, first_token.text, cmd_args);

        dynarray_deinit(&cmd_args);
        std::free(input);
    }

	end_of_program();
}
