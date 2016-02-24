#include "types.h"

#include "hashtable.h"
#include "nametable.h"
#include "json_error.h"
#include "json.h"
#include "common.h"
#include "platform.h"



// void symboltable_init(SymbolTable *st, size_t storage_size)
// {
//     st->storage = CALLOC_ARRAY(char, storage_size);
//     // st->lookup = ht_init();
// }



struct ProgramMemory
{
    NameTable names;
    DynArray<TypeDescriptor> type_descriptors;
    OAHashtable<NameRef, TypeDescriptor*> typedesc_bindings;
};



TypeDescriptor *get_typedesc(TypeDescriptorRef ref)
{
    return ref.index && ref.owner ? get(ref.owner, ref.index) : 0;
}

TypeDescriptorRef add_typedescriptor(ProgramMemory *prgmem, TypeDescriptor **ptr)
{
    TypeDescriptor *td = append(&prgmem->type_descriptors);
    memset(td, 0, sizeof(*td));
    if (ptr)
    {
        *ptr = td;
    }

    TypeDescriptorRef result;
    result.index = prgmem->type_descriptors.count - 1;
    result.owner = &prgmem->type_descriptors;
    return result;
}


void bind_type_descriptor(ProgramMemory *prgmem, NameRef name, TypeDescriptor *type_desc)
{
    ht_set(&prgmem->typedesc_bindings, name, type_desc);
}


// TypeDescriptor *type_desc_from_json(ProgramMemory *prgmem, json_value_s *jv)
TypeDescriptorRef type_desc_from_json(ProgramMemory *prgmem, json_value_s *jv)
{
    // TypeDescriptor *result = MALLOC(TypeDescriptor);
    // TypeDescriptor *result = append(&prgmem->type_descriptors);
    // memset(result, 0, sizeof(*result));
    TypeDescriptor *type_desc;
    TypeDescriptorRef result = add_typedescriptor(prgmem, &type_desc);
 
    json_type_e jvtype = (json_type_e)jv->type;
    switch (jvtype)
    {
        case json_type_string:
            type_desc->type_id = TypeID::String;
            break;
        case json_type_number:
            type_desc->type_id = TypeID::Float;
            break;

        case json_type_object:
        {
            json_object_s *jso = (json_object_s *)jv->payload;
            assert(jso->length < UINT32_MAX);

            // Create members array and set type ID before adding members, because
            // adding members could create additional typedescriptors and invalidate
            // pointers into the type descriptor storage
            type_desc->members = dynarray_init<TypeMember>((u32)jso->length);
            type_desc->type_id = TypeID::Compound;

            for (json_object_element_s *elem = jso->start;
                 elem;
                 elem = elem->next)
            {
                type_desc = get_typedesc(result);
                TypeMember *member = append(&type_desc->members);
                assert(elem->name->string_size < UINT16_MAX);
                StrSlice nameslice = str_slice(elem->name->string, (u16)elem->name->string_size);
                member->name = nametable_find_or_add(&prgmem->names, nameslice);
                    // str_slice(elem->name->string, (u16)elem->name->string_size);
                // member->type_desc = type_desc_from_json(prgmem, elem->value);
                member->typedesc_ref = type_desc_from_json(prgmem, elem->value);
            }
            break;
        }

        case json_type_array:
            assert(0);
            break;
 
        case json_type_true:
        case json_type_false:
            type_desc->type_id = TypeID::Bool;
            break;

       case json_type_null:
            type_desc->type_id = TypeID::None;
            break   ;
    }

    return result;
}

TypeDescriptor *clone(const TypeDescriptor *type_desc);

DynArray<TypeMember> clone(const DynArray<TypeMember> &memberset)
{
    DynArray<TypeMember> result = dynarray_init<TypeMember>(memberset.count);
    
    // TypeMemberSet *result = type_member_set_alloc(memberset->count);

    for (u32 i = 0; i < memberset.count; ++i)
    {
        printf_ln("1 Members count... %i, srcdata address %px  destdata address %px",
                  memberset.count, memberset.data, result.data);
        TypeMember *src_member = get(memberset, i);
        
        printf_ln("2 Members count... %i, srcdata address %px  destdata address %px",
                  memberset.count, memberset.data, result.data);
        TypeMember *dest_member = append(&result);
        ZERO_PTR(dest_member);
        printf_ln("3 Members count... %i, srcdata address %px  destdata address %px",
                  memberset.count, memberset.data, result.data);
        NameRef newname = src_member->name;
        printf_ln("newname index %li, src_member address: %px", newname.offset, src_member);
        printf_ln("4 Members count... %i, srcdata address %px  destdata address %px",
                  memberset.count, memberset.data, result.data);
        dest_member->name = newname;
        printf_ln("5 Members count... %i, srcdata address %px  destdata address %px",
                  memberset.count, memberset.data, result.data);
        // dest_member->typedesc_ref = src_member->typedesc_ref;

        // dest_member->type_desc = clone(prgmem, src_member->type_desc);
    }

    return result;
}


// TypeDescriptor *clone(ProgramMemory *prgmem, const TypeDescriptor *type_desc)
TypeDescriptorRef clone(ProgramMemory *prgmem, const TypeDescriptor *src_typedesc, TypeDescriptor **ptr)
{
    // TypeDescriptor *result = MALLOC(TypeDescriptor);
    TypeDescriptor *dest_typedesc;
    TypeDescriptorRef result = add_typedescriptor(prgmem, &dest_typedesc);
    if (ptr)
    {
        *ptr = dest_typedesc;
    }
    
    dest_typedesc->type_id = src_typedesc->type_id;
    switch ((TypeID::Enum)dest_typedesc->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            // nothing to do for non-compound types
            break;

        case TypeID::Compound:
            printf_ln("Member count: %i", src_typedesc->members.count);
            dest_typedesc->members = clone(src_typedesc->members);
            break;
    }

    return result;
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


bool equal(const TypeMember *a, const TypeMember *b)
{
    return nameref_identical(a->name, b->name)
        && typedesc_ref_identical(a->typedesc_ref, b->typedesc_ref);
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b)
{   
    switch ((TypeID::Enum)a->type_id)
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
// void add_member(TypeDescriptor *typedesc_ref, const TypeMember &member)
{
    // TypeDescriptor *type_desc = get_typedesc(typedesc_ref);
    TypeMember *new_member = append(&type_desc->members);
    new_member->name = member.name;
    new_member->typedesc_ref = member.typedesc_ref;
    // new_member->typedesc = clone(member.type_desc);
}

// TypeDescriptor *merge_type_descriptors(TypeDescriptor *typedesc_a, TypeDescriptor *typedesc_b)
// {
//     TypeDescriptor *result = clone(typedesc_a);

//     assert((typedesc_a->type_id != TypeID::Compound) ^ (typedesc_a->type_id == typedesc_b->type_id));

//     switch ((TypeID::Enum)typedesc_a->type_id)
//     {
//         case TypeID::None:
//         case TypeID::String:
//         case TypeID::Int:
//         case TypeID::Float:
//         case TypeID::Bool:
//             // do nothing
//             break;

//         case TypeID::Compound:
//             assert(false);
//             break;
//     }

//     return result;
// }


// TypeDescriptorRef merge_compound_types(const TypeDescriptorRef typedescref_a, const TypeDescriptorRef typedescref_b)
// {
    
// }

TypeDescriptorRef merge_compound_types(ProgramMemory *prgmem,
                                       const TypeDescriptor *typedesc_a,
                                       const TypeDescriptor *typedesc_b,
                                       TypeDescriptor **ptr)
{
    assert(typedesc_a->type_id == TypeID::Compound);
    assert(typedesc_b->type_id == TypeID::Compound);

    TypeDescriptor *result_ptr;
    TypeDescriptorRef result_ref = clone(prgmem, typedesc_a, &result_ptr);

    // for (u32 ib = 0; ib < typedesc_b->members.count; ++ib)
    // {
    //     TypeMember *b_member = get(typedesc_b->members, ib);
    //     TypeMember *a_member = find_member(*typedesc_a, b_member->name);

    //     // if (! (a_member && equal(a_member->type_desc, b_member->type_desc)))
    //     if (! (a_member && typedesc_ref_identical(a_member->typedesc_ref, b_member->typedesc_ref)))
    //     {
    //         add_member(result_ptr, *b_member);
    //     }
    // }

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
    TypeID::Enum type_id = (TypeID::Enum)type_desc->type_id;
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
                // pretty_print(member->type_desc, indent);
                pretty_print(member->typedesc_ref, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}


void prgmem_init(ProgramMemory *prgmem)
{
    nametable_init(&prgmem->names, MEGABYTES(2));
    dynarray_init(&prgmem->type_descriptors, 0);
    append(&prgmem->type_descriptors);
    ht_init(&prgmem->typedesc_bindings);
}

#include <unistd.h>
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
}
