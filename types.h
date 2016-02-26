// -*- c++ -*-

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


// const char *to_string(u32 type_id)
// {
//     return to_string((TypeID::Enum)type_id);
// }


const char *to_string(TypeID::Tag type_id)
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

}



struct TypeDescriptor;

struct TypeDescriptorRef
{
    u32 index;
    DynArray<TypeDescriptor> *owner;
};


bool typedesc_ref_identical(const TypeDescriptorRef &lhs, const TypeDescriptorRef &rhs)
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


struct ValueMember;


struct Value
{
    TypeDescriptor *type_desc;
    union
    {
        Str str_val;
        f32 f32_val;
        s32 s32_val;
        DynArray<ValueMember> members;
    };
};


struct ValueMember
{
    // Assertion on save: member_desc->type_desc == value->type_desc
    // Maybe could exist in a being-edited state where the type is not satisfied?
    TypeMember *member_desc;
    Value value;
};

#define TYPES_H
#endif
