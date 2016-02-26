#include "types.h"

#include "hashtable.h"
#include "nametable.h"
#include "json_error.h"
#include "json.h"
#include "common.h"
#include "platform.h"


// Debugging
// http://stackoverflow.com/questions/312312/what-are-some-reasons-a-release-build-would-run-differently-than-a-debug-build
// http://www.codeproject.com/Articles/548/Surviving-the-Release-Version


// void symboltable_init(SymbolTable *st, size_t storage_size)
// {
//     st->storage = CALLOC_ARRAY(char, storage_size);
//     // st->lookup = ht_init();
// }



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
    // TypeDescriptor *result = MALLOC(TypeDescriptor);
    // TypeDescriptor *result = append(&prgmem->type_descriptors);
    // memset(result, 0, sizeof(*result));
    // TypeDescriptor *type_desc;
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

            // Create members array and set type ID before adding members, because
                              // adding members could create additional typedescriptors and invalidate
                                        // pointers into the type descriptor storage

            DynArray<TypeMember> members;
            dynarray_init(&members, (u32)jso->length);

            for (json_object_element_s *elem = jso->start;
                 elem;
                 elem = elem->next)
            {
                TypeMember *member = append(&members);
                member->name = nametable_find_or_add(&prgmem->names,
                                                     elem->name->string, elem->name->string_size);
                    // str_slice(elem->name->string, (u16)elem->name->string_size);
                // member->type_desc = type_desc_from_json(prgmem, elem->value);
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

            // result = add_typedescriptor(prgmem, &type_desc);
            // type_desc = get_typedesc(result);
            // type_desc->members = members;
            // type_desc->type_id = TypeID::Compound;
            // TypeDescriptorRef preexisting = find_equiv_typedescriptor()
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
        IGNORE(printf_ln("[%i] 1 Members count... %i, srcdata address %px  destdata address %px memberset address %px",
                  i, memberset.count, memberset.data, result.data, &memberset));
        IGNORE(printf_ln("[%i] end of result.data == &memberset ? %i",
                  i, (char*)(result.data + result.count) == (char*)&memberset));
        TypeMember *src_member = get(memberset, i);
        
        IGNORE(printf_ln("[%i] 2 Members count... %i, srcdata address %px  destdata address %px memberset address %px",
                  i, memberset.count, memberset.data, result.data, &memberset));
        IGNORE(printf_ln("[%i] end of result.data == &memberset ? %i",
                  i, (char*)(result.data + result.count) == (char*)&memberset));
        TypeMember *dest_member = append(&result);
        ZERO_PTR(dest_member);
        IGNORE(printf_ln("[%i] 3 Members count... %i, srcdata address %px  destdata address %px memberset address %px"
                  "\n    dest_member == memberset ? %i",
                  i, memberset.count, memberset.data, result.data, &memberset,
                  (char *)dest_member == (char *)&memberset));
        NameRef newname = src_member->name;
        IGNORE(printf_ln("newname index %li, src_member address: %px", newname.offset, src_member));
        IGNORE(printf_ln("[%i] 4 Members count... %i, srcdata address %px  destdata address %px memberset address %px"
                  "\n    dest_member == memberset ? %i",
                  i, memberset.count, memberset.data, result.data, &memberset,
                  (char *)dest_member == (char *)&memberset));
        dest_member->name = newname;
        IGNORE(printf_ln("[%i] 5 Members count... %i, srcdata address %px  destdata address %px memberset address %px"
                  "\n    dest_member == memberset ? %i"
                  "\n--------------",
                  i, memberset.count, memberset.data, result.data, &memberset,
                  (char *)dest_member == (char *)&memberset));
        dest_member->typedesc_ref = src_member->typedesc_ref;
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
            IGNORE(printf_ln("Member count: %i", src_typedesc->members.count));
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
// void add_member(TypeDescriptor *typedesc_ref, const TypeMember &member)
{
    // TypeDescriptor *type_desc = get_typedesc(typedesc_ref);
    TypeMember *new_member = append(&type_desc->members);
    new_member->name = member.name;
    new_member->typedesc_ref = member.typedesc_ref;
    // new_member->typedesc = clone(member.type_desc);
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
    // auto r = result_ref.index;
    // printf_ln("ref index: %i", r);

    for (u32 ib = 0; ib < typedesc_b->members.count; ++ib)
    {
        TypeMember *b_member = get(typedesc_b->members, ib);
        TypeMember *a_member = find_member(*typedesc_a, b_member->name);

        // if (! (a_member && equal(a_member->type_desc, b_member->type_desc)))
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
                // pretty_print(member->type_desc, indent);
                pretty_print(member->typedesc_ref, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}


int main(int argc, char **argv)
{
     //void run_tests();
     //run_tests();
     //return 0;

    ProgramMemory prgmem;
    prgmem_init(&prgmem);

    if (argc < 2)
    {
        printf("No file specified\n");
		waitkey();
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
	waitkey();
}
