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

#define CLI_COMMAND_FN_SIG(name) void name(void *userdata, DynArray<Value> args)
typedef CLI_COMMAND_FN_SIG(CliCommandFn);


struct CliCommand
{
    CliCommandFn *fn;
    void *userdata;
};


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

    OAHashtable<NameRef, CliCommand> command_map;
};


TypeDescriptorRef add_typedescriptor(ProgramMemory *prgmem, TypeDescriptor **ptr);


void prgmem_init(ProgramMemory *prgmem)
{
    nametable_init(&prgmem->names, MEGABYTES(2));
    dynarray_init(&prgmem->type_descriptors, 0);
    append(&prgmem->type_descriptors);
    ht_init(&prgmem->typedesc_bindings);

    TypeDescriptor *none_type;
    prgmem->prim_none = add_typedescriptor(prgmem, &none_type);
    none_type->type_id = TypeID::None;

    TypeDescriptor *string_type;
    prgmem->prim_string = add_typedescriptor(prgmem, &string_type);
    string_type->type_id = TypeID::String;

    TypeDescriptor *int_type;
    prgmem->prim_int = add_typedescriptor(prgmem, &int_type);
    int_type->type_id = TypeID::Int;
    
    TypeDescriptor *float_type;
    prgmem->prim_float = add_typedescriptor(prgmem, &float_type);
    float_type->type_id = TypeID::Float;
    
    TypeDescriptor *bool_type;
    prgmem->prim_bool = add_typedescriptor(prgmem, &bool_type);
    bool_type->type_id = TypeID::Bool;

    ht_init(&prgmem->command_map);
}


TypeDescriptorRef make_typeref(ProgramMemory *prgmem, u32 index)
{
    TypeDescriptorRef result = {index, &prgmem->type_descriptors};
    return result;
}


TypeDescriptor *get_typedesc(TypeDescriptorRef ref)
{
    return ref.index && ref.owner ? get(ref.owner, ref.index) : 0;
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


bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


bool equal(const TypeMember *a, const TypeMember *b)
{
    return nameref_identical(a->name, b->name)
        && typedesc_ref_identical(a->typedesc_ref, b->typedesc_ref);
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b)
{
    assert(a);
    assert(b);
    switch ((TypeID::Tag)a->type_id)
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


void bind_typeref(ProgramMemory *prgmem, NameRef name, TypeDescriptorRef typeref)
{
    ht_set(&prgmem->typedesc_bindings, name, typeref);
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


void pretty_print(TypeDescriptor *type_desc, int indent = 0);

void pretty_print(TypeDescriptorRef typedesc_ref, int indent = 0)
{
    TypeDescriptor *td = get_typedesc(typedesc_ref);
    pretty_print(td, indent);
}


void pretty_print(TypeDescriptor *type_desc, int indent)
{
    TypeID::Tag type_id = (TypeID::Tag)type_desc->type_id;
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
                printf_indent(indent, "%s: ", str_slice(member->name).data);
                pretty_print(member->typedesc_ref, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}


void pretty_print(Value *value)
{
    TypeDescriptor *type_desc = get_typedesc(value->typedesc_ref);

    switch ((TypeID::Tag)type_desc->type_id)
    {
        case TypeID::None:
            println("NONE");
            break;

        case TypeID::String:
            printf_ln("%s", value->str_val.data);
            break;

        case TypeID::Int:
            printf_ln("%i", value->s32_val);
            break;

        case TypeID::Float:
            printf_ln("%f", value->f32_val);
            break;

        case TypeID::Bool:
            printf_ln("%s", (value->bool_val ? "True" : "False"));
            break;

        case TypeID::Compound:
            println("Printing compounds not supported yet");
    }
}


void pretty_print(tokenizer::Token token)
{
    char token_output[256];
    size_t len = std::min((size_t)token.text.length, sizeof(token_output) - 1);
    if (len == 0)
    {
        token_output[0] = 0;
    }
    else
    {
        std::strncpy(token_output, token.text.data, len);
        token_output[len] = 0;
    }

    printf_ln("Token(%s, \"%s\")", tokenizer::to_string(token.type), token_output);
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


void register_command(ProgramMemory *prgmem, NameRef name, CliCommandFn *fnptr, void *userdata)
{
    assert(fnptr);
    CliCommand cmd = {fnptr, userdata};
    if (ht_set(&prgmem->command_map, name, cmd))
    {
        printf_ln("Warning, overriding command: %s", str_slice(name).data);
    }
}

void register_command(ProgramMemory *prgmem, StrSlice name, CliCommandFn *fnptr, void *userdata)
{
    assert(fnptr);
    NameRef nameref = nametable_find_or_add(&prgmem->names, name);
    register_command(prgmem, nameref, fnptr, userdata);
}

void register_command(ProgramMemory *prgmem, const char *name, CliCommandFn *fnptr, void *userdata)
{
    register_command(prgmem, str_slice(name), fnptr, userdata);
}

#define REGISTER_COMMAND(prgmem, fn_ident, userdata) register_command((prgmem), # fn_ident, &fn_ident, userdata)


void exec_command(ProgramMemory *prgmem, NameRef name, DynArray<Value> args)
{
    CliCommand *cmd = ht_find(&prgmem->command_map, name);

    if (!cmd)
    {
        printf_ln("Command '%s' not found", str_slice(name).data);
        return;
    }

    cmd->fn(cmd->userdata, args);
}


void exec_command(ProgramMemory *prgmem, StrSlice name, DynArray<Value> args)
{
    NameRef nameref = nametable_find(&prgmem->names, name);
    if (!nameref.offset)
    {
        Str namecopy = str(name);
        printf_ln("Command '%s' not found", namecopy.data);
        str_free(&namecopy);
    }
    else
    {
        exec_command(prgmem, nameref, args);
    }
}


CLI_COMMAND_FN_SIG(say_hello)
{
    UNUSED(userdata);
    UNUSED(args);

    println("You are calling the say_hello command");
}


CLI_COMMAND_FN_SIG(list_args)
{
    UNUSED(userdata);
    println("list_args:");
    for (u32 i = 0; i < args.count; ++i)
    {
        Value *val = get(&args, i);
        pretty_print(val->typedesc_ref);
        pretty_print(val);
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

        // TypeDescriptor *type_desc = type_desc_from_json(&prgmem, jv);
        TypeDescriptorRef typedesc_ref = type_desc_from_json(&prgmem, jv);
        NameRef bound_name = nametable_find_or_add(&prgmem.names, filename);
        bind_typeref(&prgmem, bound_name, typedesc_ref);

        println("New type desciptor:");
        // pretty_print(type_desc);
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
