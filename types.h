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

// struct TypeRef
// {
//     u32 index;
//     DynArray<TypeDescriptor> *owner;
// };

// struct SortTypeRef
// {
//     bool operator() (const TypeRef &a, const TypeRef &b)
//     {
//         return (a.owner == b.owner && a.index < b.index)
//             || (intptr_t)a.owner < (intptr_t)b.owner;
//     }
// };

// inline bool typeref_identical(const TypeRef &lhs, const TypeRef &rhs)
// {
//     return lhs.index == rhs.index
//         && lhs.owner == rhs.owner;
// }


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


#define ASSERT_UNION(typedesc) assert((typedesc)->type_id == TypeID::Union)


inline DynArrayCount union_num_cases(TypeDescriptor *typedesc)
{
    ASSERT_UNION(typedesc);
    return typedesc->union_type.type_cases.count;
}


inline TypeDescriptor *union_getcase(TypeDescriptor *typedesc, DynArrayCount i)
{
    ASSERT_UNION(typedesc);
    return typedesc->union_type.type_cases[i];
}


bool are_typedescs_unique(const DynArray<TypeDescriptor *> *types);

CompoundTypeMember *find_member(const TypeDescriptor *type_desc, NameRef name);

HEADERFN bool equal(const TypeDescriptor *a, const TypeDescriptor *b);


HEADERFN bool equal(const CompoundTypeMember *a, const CompoundTypeMember *b)
{
    return nameref_identical(a->name, b->name) && (a->typedesc == b->typedesc);
}


HEADERFN bool equal(const TypeDescriptor *a, const TypeDescriptor *b)
{
    assert(a);
    assert(b);

    if (a->type_id != b->type_id)
    {
        return false;
    }

    TYPESWITCH (a->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            return true;

        case TypeID::Array:
            return a->array_type.elem_type == b->array_type.elem_type;

        case TypeID::Compound:
        {
            DynArrayCount a_mem_count = a->compound_type.members.count;
            if (a_mem_count != b->compound_type.members.count)
            {
                return false;
            }

            for (DynArrayCount ia = 0; ia < a_mem_count; ++ia)
            {
                CompoundTypeMember *a_member = &a->compound_type.members[ia];
                CompoundTypeMember *b_member = find_member(b, a->compound_type.members[ia].name);
                if (!b_member || !equal(a_member, b_member))
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

            #if SANITY_CHECK
            assert(are_typedescs_unique(&a->union_type.type_cases));
            assert(are_typedescs_unique(&b->union_type.type_cases));
            #endif

            for (DynArrayCount ia = 0, count = a->union_type.type_cases.count;
                 ia < count;
                 ++ia)
            {
                TypeDescriptor *a_case = a->union_type.type_cases[ia];

                bool found = false;

                for (DynArrayCount ib = 0;
                     ib < count;
                     ++ib)
                {
                    if (a_case == b->union_type.type_cases[ib])
                    {
                        found = true;
                        break;
                    }
                }

                if (!found)
                {
                    return false;
                }
            }
            return true;
    }
}


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

HEADERFN void value_assertions(const Value &value)
{
    assert(value.typedesc->type_id != TypeID::Union);
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


struct CompoundValueMember
{
    // Assertion on save: member_desc->type_desc == value->type_desc
    // Maybe could exist in a being-edited state where the type is not satisfied?
    NameRef name;
    Value value;
};


HEADERFN CompoundValueMember *find_member(const Value *value, NameRef name)
{
    for (u32 i = 0; i < value->compound_value.members.count; ++i)
    {
        CompoundValueMember *member = &value->compound_value.members[i];
        if (nameref_identical(member->name, name))
        {
            return member;
        }
    }

    return nullptr;
}


HEADERFN Value clone(const Value *src)
{
    value_assertions(src);

    Value result;
    result.typedesc = src->typedesc;

    TypeDescriptor *type_desc = src->typedesc;

    TYPESWITCH (type_desc->type_id)
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
            dynarray::init(&result.array_value.elements, src->array_value.elements.count);
            for (DynArrayCount i = 0; i < src->array_value.elements.count; ++i)
            {
                Value *src_element = &src->array_value.elements[i];
                Value *dest_element = dynarray::append(&result.array_value.elements);
                *dest_element = clone(src_element);
            }
            break;

        case TypeID::Compound:
            dynarray::init(&result.compound_value.members, src->compound_value.members.count);
            for (DynArrayCount i = 0; i < src->compound_value.members.count; ++i)
            {
                CompoundValueMember *src_member = &src->compound_value.members[i];
                CompoundValueMember *dest_member = dynarray::append(&result.compound_value.members);
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

    if (lhs.typedesc != rhs.typedesc)
    {
        return false;
    }

    TypeDescriptor *type_desc = lhs.typedesc;

    TYPESWITCH (type_desc->type_id)
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
                Value *l_elem = &lhs.array_value.elements[i];
                Value *r_elem = &rhs.array_value.elements[i];
                if (!value_equal(*l_elem, *r_elem))
                {
                    return false;
                }
            }
            break;
        }

        case TypeID::Compound:
            for (u32 i = 0; i < type_desc->compound_type.members.count; ++i)
            {
                CompoundTypeMember *type_member = &type_desc->compound_type.members[i];
                CompoundValueMember *lh_member = find_member(&lhs, type_member->name);
                CompoundValueMember *rh_member = find_member(&rhs, type_member->name);

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
    TypeDescriptor *type_desc = value->typedesc;

    TYPESWITCH (type_desc->type_id)
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
                Value *element = &value->array_value.elements[i];
                value_free(element);
            }
            break;

        case TypeID::Compound:            
            for (u32 i = 0; i < value->compound_value.members.count; ++i)
            {
                CompoundValueMember *member = &value->compound_value.members[i];
                value_free(&member->value);
            }
            dynarray::deinit(&value->compound_value.members);
            break;

        case TypeID::Union:
            ASSERT_MSG("There must never be a value of type Union");
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
