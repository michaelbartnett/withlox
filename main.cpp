#include "types.h"
#include "hashtable.h"
#include "json_error.h"
#include "json.h"
#include "common.h"
#include "platform.h"


TypeDescriptor *type_desc_from_json(json_value_s *jv)
{
    TypeDescriptor *result = MALLOC(TypeDescriptor);
    memset(result, 0, sizeof(*result));
 
    json_type_e jvtype = (json_type_e)jv->type;
    switch (jvtype)
    {
        case json_type_string:
            result->type_id = TypeID::String;
            break;
        case json_type_number:
            result->type_id = TypeID::Float;
            break;

        case json_type_object:
        {
            json_object_s *jso = (json_object_s *)jv->payload;
            assert(jso->length < UINT32_MAX);
            result->members = dynarray_init<TypeMember>((u32)jso->length);

            for (json_object_element_s *elem = jso->start;
                 elem;
                 elem = elem->next)
            {
                TypeMember *member = append(&result->members);
                assert(elem->name->string_size < UINT16_MAX);
                member->name = str(elem->name->string, (u16)elem->name->string_size);
                member->type_desc = type_desc_from_json(elem->value);
            }
            result->type_id = TypeID::Compound;
            break;
        }

        case json_type_array:
            assert(0);
            break;
 
        case json_type_true:
        case json_type_false:
            result->type_id = TypeID::Bool;
            break;

       case json_type_null:
            result->type_id = TypeID::None;
            break   ;
    }

    return result;
}

TypeDescriptor *clone(const TypeDescriptor *type_desc);

DynArray<TypeMember> clone(const DynArray<TypeMember> memberset)
{
    DynArray<TypeMember> result = dynarray_init<TypeMember>(memberset.count);
    
    // TypeMemberSet *result = type_member_set_alloc(memberset->count);

    for (u32 i = 0; i < memberset.count; ++i)
    {
        TypeMember *src_member = get(memberset, i);
        TypeMember *dest_member = append(&result);
        dest_member->name = str(src_member->name);
        dest_member->type_desc = clone(src_member->type_desc);
    }

    return result;
}


TypeDescriptor *clone(const TypeDescriptor *type_desc)
{
    TypeDescriptor *result = MALLOC(TypeDescriptor);
    
    result->type_id = type_desc->type_id;
    switch ((TypeID::Enum)result->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            // nothing to do for non-compound types
            break;

        case TypeID::Compound:
            result->members = clone(type_desc->members);
            break;
    }

    return result;
}


bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


bool equal(const TypeMember *a, const TypeMember *b)
{
    return str_equal(a->name, b->name) && equal(a->type_desc, b->type_desc);
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


TypeMember *find_member(const TypeDescriptor &type_desc, const StrSlice name)
{
    size_t count = type_desc.members.count;
    for (u32 i = 0; i < count; ++i)
    {
        TypeMember *mem = get(type_desc.members, i);

        if (str_equal(mem->name, name))
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
    new_member->type_desc = clone(member.type_desc);
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


TypeDescriptor *merge_compound_types(const TypeDescriptor *typedesc_a, const TypeDescriptor *typedesc_b)
{
    assert(typedesc_a->type_id == TypeID::Compound);
    assert(typedesc_b->type_id == TypeID::Compound);
    TypeDescriptor *result = clone(typedesc_a);

    for (u32 ib = 0; ib < typedesc_b->members.count; ++ib)
    {
        TypeMember *b_member = get(typedesc_b->members, ib);
        TypeMember *a_member = find_member(*typedesc_a, str_slice(b_member->name));

        if (! (a_member && equal(a_member->type_desc, b_member->type_desc)))
        {
            add_member(result, *b_member);
        }
    }
    return result;
}


void pretty_print(TypeDescriptor *type_desc, int indent = 0)
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
                printf_indent(indent, "%s: ", member->name.data);
                pretty_print(member->type_desc, indent);
            }

            indent -= 2;
            println_indent(indent, "}");
            break;
    }
}


#include <unistd.h>
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("No file specified\n");
        return 0;
    }

    size_t jsonflags = json_parse_flags_default
        | json_parse_flags_allow_trailing_comma
        | json_parse_flags_allow_c_style_comments
        ;

    TypeDescriptor *result_td = 0;

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

        TypeDescriptor *type_desc = type_desc_from_json(jv);
        
        println("New type desciptor:");
        pretty_print(type_desc);

        if (!result_td)
        {
            result_td = type_desc;
        }
        else
        {
            result_td = merge_compound_types(result_td, type_desc);
        }
    }

    println("completed without errors");

    pretty_print(result_td);
}
