// -*- c++ -*-


/*

parse_type <type descriptor syntax>

<type descriptor syntax> := <bound type name>
                          | { <type member descriptor>* }
<type member descriptor>


 */


#ifndef TYPES_H

#include "numeric_types.h"
#include "str.h"
#include "dynarray.h"
#include "nametable.h"

// enum class TypeFlgas : u32
// {
//     None = 0,
//     Nullable = (1 << 0),
//     Optional = (1 << 1),
// };

namespace TypeID
{

enum Tag
{
    None,
    String,
    Int,
    Float,
    Bool,
    // Enum,
    // Array,
    Compound
};


inline const char *to_string(TypeID::Tag type_id)
{
    switch (type_id)
    {
        case TypeID::None:     return "None";
        case TypeID::String:   return "String";
        case TypeID::Int:      return "Int";
        case TypeID::Float:    return "Float";
        case TypeID::Bool:     return "Bool";
        case TypeID::Compound: return "Compound";
    }
}

inline const char *to_string(u32 type_id)
{
    // stupid C++03 enums
    return to_string((Tag)type_id);
}

}



struct TypeDescriptor;

struct TypeDescriptorRef
{
    u32 index;
    DynArray<TypeDescriptor> *owner;
};


inline bool typedesc_ref_identical(const TypeDescriptorRef &lhs, const TypeDescriptorRef &rhs)
{
    return lhs.index == rhs.index
        && lhs.owner == rhs.owner;
}


struct TypeMember
{
    NameRef name;
    TypeDescriptorRef typedesc_ref;
    // TypeDescriptor *type_desc;
};


struct TypeDescriptor
{
    u32 type_id;
    DynArray<TypeMember> members;
};


HEADERFN bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


HEADERFN bool equal(const TypeMember *a, const TypeMember *b)
{
    return nameref_identical(a->name, b->name)
        && typedesc_ref_identical(a->typedesc_ref, b->typedesc_ref);
}


HEADERFN bool equal(const TypeDescriptor *a, const TypeDescriptor *b)
{
    assert(a);
    assert(b);

    if (a->type_id != b->type_id)
    {
        return false;
    }

    switch ((TypeID::Tag)a->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            return true;

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


inline TypeDescriptor *get_typedesc(TypeDescriptorRef ref)
{
    return ref.index && ref.owner ? get(ref.owner, ref.index) : 0;
}


struct ValueMember;

struct Value
{
    TypeDescriptorRef typedesc_ref;
    union
    {
        Str str_val;
        f32 f32_val;
        s32 s32_val;
        bool bool_val;
        DynArray<ValueMember> members;
    };
};

// struct ValueHash
// {
//     u32 operator()(const Value &value)
//     {
//         TypeDescriptor
//     }
// };


struct ValueMember
{
    // Assertion on save: member_desc->type_desc == value->type_desc
    // Maybe could exist in a being-edited state where the type is not satisfied?
    NameRef name;
    Value value;
};


HEADERFN ValueMember *find_member(const Value *value, NameRef name)
{
    for (u32 i = 0; i < value->members.count; ++i)
    {
        ValueMember *member = get(value->members, i);
        if (nameref_identical(member->name, name))
        {
            return member;
        }
    }

    return NULL;
}


HEADERFN Value clone(const Value *src)
{
    Value result;
    result.typedesc_ref = src->typedesc_ref;

    TypeDescriptor *type_desc = get_typedesc(src->typedesc_ref);
    
    switch ((TypeID::Tag)type_desc->type_id)
    {
        case TypeID::None:
            break;

        case TypeID::String:
            result.str_val = str(src->str_val);
            break;

        case TypeID::Int:
            result.s32_val = src->s32_val;
            break;

        case TypeID::Float:
            result.f32_val = src->f32_val;
            break;

        case TypeID::Bool:
            result.bool_val = src->bool_val;
            break;

        case TypeID::Compound:
            dynarray_init(&result.members, src->members.count);
            for (u32 i = 0; i < src->members.count; ++i)
            {
                ValueMember *src_member = get(src->members, i);
                ValueMember *dest_member = append(&result.members);
                dest_member->name = src_member->name;
                dest_member->value = clone(&src_member->value);
            }
    }

    return result;
}


HEADERFN bool value_equal(const Value &lhs, const Value &rhs)
{
    if (!typedesc_ref_identical(lhs.typedesc_ref, rhs.typedesc_ref))
    {
        return false;
    }

    TypeDescriptor *type_desc = get_typedesc(lhs.typedesc_ref);

    switch ((TypeID::Tag)type_desc->type_id)
    {
        case TypeID::None:     return true;
        case TypeID::String:   return str_equal(lhs.str_val, rhs.str_val);
        case TypeID::Int:      return lhs.s32_val == rhs.s32_val;
        case TypeID::Float:    return 0.0f == (lhs.f32_val - rhs.f32_val);
        case TypeID::Bool:     return lhs.bool_val == rhs.bool_val;
        case TypeID::Compound:
            for (u32 i = 0; i < type_desc->members.count; ++i)
            {
                TypeMember *type_member = get(type_desc->members, i);
                ValueMember *lh_member = find_member(&lhs, type_member->name);
                ValueMember *rh_member = find_member(&rhs, type_member->name);

                if (!value_equal(lh_member->value, rh_member->value))
                {
                    return false;
                }
            }
    }

    return true;
}


HEADERFN void value_free(Value *value)
{
    TypeDescriptor *type_desc = get_typedesc(value->typedesc_ref);
    
    switch ((TypeID::Tag)type_desc->type_id)
    {
        case TypeID::None:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            break;

        case TypeID::String:
            str_free(&value->str_val);
            break;

        case TypeID::Compound:            
            for (u32 i = 0; i < value->members.count; ++i)
            {
                ValueMember *member = get(value->members, i);
                value_free(&member->value);
            }
            dynarray_deinit(&value->members);
            break;
    }

    ZERO_PTR(value);
}


struct ValueEqual
{
    bool operator()(const Value &lhs, const Value &rhs)
    {
        return value_equal(lhs, rhs);
    }
};

#define TYPES_H
#endif
