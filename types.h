// -*- c++ -*-


/*
possible grammar for type definitions

parse_type <type descriptor syntax>

<type descriptor syntax> := <bound type name>
                          | { <type member descriptor>* }
<type member descriptor>


[1, "two", 3.0]

 */


#ifndef TYPES_H

#include "numeric_types.h"
#include "str.h"
#include "dynarray.h"
#include "nametable.h"


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
    Array,
    Compound,
    Union
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
        case TypeID::Array:    return "Array";
        case TypeID::Compound: return "Compound";
        case TypeID::Union:    return "Union";
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

struct SortTypeRef
{
    bool operator() (const TypeDescriptorRef &a, const TypeDescriptorRef &b)
    {
        return (a.owner == b.owner && a.index < b.index)
            || (intptr_t)a.owner < (intptr_t)b.owner;
    }
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

// TODO(mike): Wrap the compound members array in a struct
// struct CompountType
// {
//     DynArray<TypeMember> members;
// };


struct ArrayType
{
    TypeDescriptorRef elem_typedesc_ref;
    // TypeDescriptorRef element_type;
};


struct UnionType
{
    DynArray<TypeDescriptorRef> type_cases;
};


struct TypeDescriptor
{
    u32 type_id;
    union
    {
        DynArray<TypeMember> members;

        // these could each just be a TypeSet the way it works now, but good to keep them separate
        ArrayType array_type;
        UnionType union_type; 
    };
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

        case TypeID::Array:
            return typedesc_ref_identical(a->array_type.elem_typedesc_ref,
                                          b->array_type.elem_typedesc_ref);
            break;

        case TypeID::Compound:
        {
            size_t a_mem_count = a->members.count;
            if (a_mem_count != b->members.count)
            {
                return false;
            }

            for (u32 i = 0; i < a_mem_count; ++i)
            {
                if (! equal(dynarray_get(a->members, i),
                            dynarray_get(b->members, i)))
                {
                    return false;
                }
            }
            return true;
        }

        case TypeID::Union:
            if (a->union_type.type_cases.count != b->union_type.type_cases.count)
            {
                return false;
            }

            for (DynArrayCount i = 0,
                     count = a->union_type.type_cases.count;
                 i < count;
                 ++i)
            {
                TypeDescriptorRef *a_case = dynarray_get(a->union_type.type_cases, i);
                TypeDescriptorRef *b_case = dynarray_get(b->union_type.type_cases, i);

                if (!typedesc_ref_identical(*a_case, *b_case))
                {
                    return false;
                }
            }
            return true;
    }
}


inline TypeDescriptor *get_typedesc(TypeDescriptorRef ref)
{
    return ref.index && ref.owner ? dynarray_get(ref.owner, ref.index) : 0;
}


struct ValueMember;
struct Value;


struct ArrayValue
{
    // TypeDescriptorRef elem_typedesc_ref;
    DynArray<Value> elements;
};


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
        ArrayValue array_value;
    };
};

HEADERFN void value_assertions(const Value &value)
{
    assert(get_typedesc(value.typedesc_ref)->type_id != TypeID::Union);
}
HEADERFN void value_assertions(const Value *value)
{
    value_assertions(*value);
}

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
        ValueMember *member = dynarray_get(value->members, i);
        if (nameref_identical(member->name, name))
        {
            return member;
        }
    }

    return NULL;
}


HEADERFN Value clone(const Value *src)
{
    value_assertions(src);

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

        case TypeID::Array:
            dynarray_init(&result.array_value.elements, src->array_value.elements.count);
            for (DynArrayCount i = 0; i < src->array_value.elements.count; ++i)
            {
                Value *src_element = dynarray_get(src->array_value.elements, i);
                Value *dest_element = dynarray_append(&result.array_value.elements);
                *dest_element = clone(src_element);
            }
            break;

        case TypeID::Compound:
            dynarray_init(&result.members, src->members.count);
            for (DynArrayCount i = 0; i < src->members.count; ++i)
            {
                ValueMember *src_member = dynarray_get(src->members, i);
                ValueMember *dest_member = dynarray_append(&result.members);
                dest_member->name = src_member->name;
                dest_member->value = clone(&src_member->value);
            }
            break;

        case TypeID::Union:
            assert(!(bool)"There should never be a value with type_id Union");
            break;
    }

    return result;
}


HEADERFN bool value_equal(const Value &lhs, const Value &rhs)
{
    value_assertions(lhs);
    value_assertions(rhs);

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

        case TypeID::Array:
        {
            DynArrayCount num_elems = lhs.array_value.elements.count; 
            if (num_elems != lhs.array_value.elements.count)
            {
                return false;
            }
            for (DynArrayCount i = 0; i < num_elems; ++i)
            {
                Value *l_elem = dynarray_get(lhs.array_value.elements, i);
                Value *r_elem = dynarray_get(rhs.array_value.elements, i);
                if (!value_equal(*l_elem, *r_elem))
                {
                    return false;
                }
            }
            break;
        }

        case TypeID::Compound:
            for (u32 i = 0; i < type_desc->members.count; ++i)
            {
                TypeMember *type_member = dynarray_get(type_desc->members, i);
                ValueMember *lh_member = find_member(&lhs, type_member->name);
                ValueMember *rh_member = find_member(&rhs, type_member->name);

                if (!value_equal(lh_member->value, rh_member->value))
                {
                    return false;
                }
            }

        case TypeID::Union:
            assert(!(bool)"There must never be a value of type Union");
            break;
    }

    return true;
}


HEADERFN void value_free(Value *value)
{
    value_assertions(value);
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

        case TypeID::Array:
            for (DynArrayCount i = 0; i < value->array_value.elements.count; ++i)
            {
                Value *element = dynarray_get(value->array_value.elements, i);
                value_free(element);
            }
            break;

        case TypeID::Compound:            
            for (u32 i = 0; i < value->members.count; ++i)
            {
                ValueMember *member = dynarray_get(value->members, i);
                value_free(&member->value);
            }
            dynarray_deinit(&value->members);
            break;

        case TypeID::Union:
            assert(!(bool)"There must never be a value of type Union");
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
