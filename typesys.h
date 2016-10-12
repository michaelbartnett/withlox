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
#include "common.h"


#define TYPESWITCH(type_id) switch ((TypeID::Tag)(type_id))

#define tIS_INT(typedesc)      ((typedesc)->type_id == TypeID::Int)
#define tIS_STRING(typedesc)   ((typedesc)->type_id == TypeID::String)
#define tIS_BOOL(typedesc)     ((typedesc)->type_id == TypeID::Bool)
#define tIS_ARRAY(typedesc)    ((typedesc)->type_id == TypeID::Array)
#define tIS_COMPOUND(typedesc) ((typedesc)->type_id == TypeID::Compound)
#define tIS_UNION(typedesc)    ((typedesc)->type_id == TypeID::Union)

#define vIS_INT(val)           tIS_INT((val)->typedesc)
#define vIS_STRING(val)        tIS_STRING((val)->typedesc)
#define vIS_BOOL(val)          tIS_BOOL((val)->typedesc)
#define vIS_ARRAY(val)         tIS_ARRAY((val)->typedesc)
#define vIS_COMPOUND(val)      tIS_COMPOUND((val)->typedesc)
#define vIS_UNION(val)         tIS_UNION((val)->typedesc)



struct ProgramState;
void load_base_type_descriptors(ProgramState *prgstate);


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
    TYPESWITCH (type_id)
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


struct CompoundTypeMember
{
    NameRef name;
    TypeDescriptor *typedesc;
};

struct ArrayType
{
    TypeDescriptor *elem_type;
};


struct UnionType
{
    DynArray<TypeDescriptor *> type_cases;
};


struct CompoundType
{
    DynArray<CompoundTypeMember> members;
};

struct TypeDescriptor
{
    u32 type_id;
    union
    {
        // DynArray<CompoundTypeMember> members;
        CompoundType compound_type;
        // these could each just be a TypeSet the way it works now, but good to keep them separate
        ArrayType array_type;
        UnionType union_type; 
    };
};


inline DynArrayCount union_num_cases(TypeDescriptor *typedesc)
{
    ASSERT(tIS_UNION(typedesc));
    return typedesc->union_type.type_cases.count;
}


inline TypeDescriptor *union_getcase(TypeDescriptor *typedesc, DynArrayCount i)
{
    ASSERT(tIS_UNION(typedesc));
    return typedesc->union_type.type_cases[i];
}


bool are_typedescs_unique(const DynArray<TypeDescriptor *> *types);

CompoundTypeMember *find_member(const TypeDescriptor *type_desc, NameRef name);

bool typedesc_equal(const TypeDescriptor *a, const TypeDescriptor *b);


struct ProgramState;

TypeDescriptor *add_typedescriptor(ProgramState *prgstate);
TypeDescriptor *add_typedescriptor(ProgramState *prgstate, TypeDescriptor type_desc);

TypeDescriptor *find_equiv_typedesc(ProgramState *prgstate, TypeDescriptor *type_desc);
TypeDescriptor *find_equiv_typedesc_or_add(ProgramState *prgstate, TypeDescriptor *type_desc, bool *new_type_added);

void bind_typedesc_name(ProgramState *prgstate, NameRef name, TypeDescriptor *typedesc);
void bind_typedesc_name(ProgramState *prgstate, const char *name, TypeDescriptor *typedesc);

TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, NameRef name);
TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, StrSlice name);
TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, Str name);

CompoundTypeMember *find_member(const TypeDescriptor &type_desc, NameRef name);

TypeDescriptor copy_typedesc(const TypeDescriptor *src_typedesc);

TypeDescriptor *compound_member_merge(ProgramState *prgstate, TypeDescriptor *a_desc, TypeDescriptor *b_desc);

TypeDescriptor *merge_types(ProgramState *prgstate, /*const*/ TypeDescriptor *a_desc, /*const*/ TypeDescriptor *b_desc);

void free_typedescriptor_components(TypeDescriptor *typedesc);



HEADERFN void add_member(TypeDescriptor *typedesc, const CompoundTypeMember &member)
{
    CompoundTypeMember *new_member = dynarray::append(&typedesc->compound_type.members);
    new_member->name = member.name;
    new_member->typedesc = member.typedesc;
}


enum TypeCheckResult
{
    TypeCheckResult_Invalid,
    TypeCheckResult_Ok,
    TypeCheckResult_MemberCountMismatch,
    TypeCheckResult_ArrayElementTypeMismatch,
    TypeCheckResult_MismatchedTypeID,
    TypeCheckResult_MissingMember,
    TypeCheckResult_MemberMismatch,
    TypeCheckResult_NoMatchForUnionCase,
    TypeCheckResult_MismatchedUnions
};

HEADERFN const char *to_string(TypeCheckResult e)
{
    switch (e)
    {
        case TypeCheckResult_Invalid:                  return "Invalid";
        case TypeCheckResult_Ok:                       return "Ok";
        case TypeCheckResult_MemberCountMismatch:      return "MemberCountMismatch";
        case TypeCheckResult_ArrayElementTypeMismatch: return "ArrayElementTypeMismatch";
        case TypeCheckResult_MismatchedTypeID:         return "MismatchedTypeID";
        case TypeCheckResult_MissingMember:            return "MissingMember";
        case TypeCheckResult_MemberMismatch:           return "MemberMismatch";
        case TypeCheckResult_NoMatchForUnionCase:      return "NoMatchForUnionCase";
        case TypeCheckResult_MismatchedUnions:         return "MismatchedUnions";
    }
}


struct TypeCheckInfo
{
    TypeCheckResult result;
    bool passed;
};

/*
tests:

bindinfer "TestType" {"test":[1, "two", 3.0], "test2":["hello", "goodbye", []]}
(should pass) checktype "TestType" {"test":[9], "test2":["awkward"]}
(should pass) checktype "TestType" {"test":[9], "test2":["awkward", [null, null]]}
(should fail) checktype "TestType" {"test":[9], "test2":["awkward", null]} 
(should fail) checktype "TestType" {"test":[9, "hi", null], "test2":["awkward"]}

 */

// TODO(mike): Better failure-reason reporting
TypeCheckInfo check_type_compatible(TypeDescriptor *input, TypeDescriptor *validator);


struct CompoundValueMember;
struct Value;


struct ArrayValue
{
    DynArray<Value> elements;
};


struct CompoundValue
{
    DynArray<CompoundValueMember> members;
};


struct Value
{
    TypeDescriptor *typedesc;
    union
    {
        Str str_val;
        f32 f32_val;
        s32 s32_val;
        bool bool_val;
        CompoundValue compound_value;
        ArrayValue array_value;
    };
};


struct CompoundValueMember
{
    // Assertion on save: member_desc->type_desc == value->type_desc
    // Maybe could exist in a being-edited state where the type is not satisfied?
    NameRef name;
    Value value;
};


inline void value_assertions(const Value &value)
{
    assert(value.typedesc->type_id != TypeID::Union);
}
inline void value_assertions(const Value *value)
{
    assert(value->typedesc->type_id != TypeID::Union);
}

// struct ValueHash
// {
//     u32 operator()(const Value &value)
//     {
//         TypeDescriptor
//     }
// };


CompoundValueMember *find_member(const Value *value, NameRef name);

Value clone(const Value *src);

bool value_equal(const Value &lhs, const Value &rhs);

void value_free_components(Value *value);

#define TYPES_H
#endif
