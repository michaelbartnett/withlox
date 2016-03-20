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
checktype "TestType" {"test":[9], "test2":["awkward"]}
checktype "TestType" {"test":[9], "test2":["awkward", [null, null]]}
checktype "TestType" {"test":[9], "test2":["awkward", null]}
checktype "TestType" {"test":[9, "hi", null], "test2":["awkward"]}

 */
HEADERFN TypeCheckInfo check_type_compatible(TypeDescriptorRef input_typeref, TypeDescriptorRef validator_typeref)
{
    TypeCheckInfo result = {};
    if (typedesc_ref_identical(input_typeref, validator_typeref))
    {
        result.result = TypeCheckResult_Ok;
        result.passed = true;
        return result;
    }

    TypeDescriptor *input = get_typedesc(input_typeref);
    TypeDescriptor *validator = get_typedesc(validator_typeref);

    if (validator->type_id != TypeID::Union && input->type_id != validator->type_id)
    {
        result.result = TypeCheckResult_MismatchedTypeID;
        result.passed = false;
        return result;
    }

    switch ((TypeID::Tag)validator->type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            assert(!(bool)"Should have early'd out by now");
            break;

        case TypeID::Array:
        {
            TypeCheckInfo elemcheck = check_type_compatible(input->array_type.elem_typedesc_ref,
                                                            validator->array_type.elem_typedesc_ref);

            result.passed = elemcheck.passed;
            if (!result.passed)
            {
                result.result = TypeCheckResult_ArrayElementTypeMismatch;
            }
            break;
        }

        case TypeID::Compound:
        {
            if (input->members.count != validator->members.count)
            {
                result.passed = false;
                result.result = TypeCheckResult_MemberCountMismatch;
            }
            else
            {
                for (DynArrayCount i = 0,
                                   icount = validator->members.count;
                     i < icount;
                    ++i)
                {
                    TypeMember *validator_member = dynarray_get(&validator->members, i);
                    TypeMember *input_member = nullptr;
                    for (DynArrayCount j = 0,
                                       jcount = input->members.count;
                         j < jcount;
                         ++j)
                    {
                        TypeMember *mem = dynarray_get(input->members, j);
                        if (nameref_identical(mem->name, validator_member->name))
                        {
                            input_member = mem;
                            break;
                        }
                    }
                    if (!input_member)
                    {
                        result.result = TypeCheckResult_MissingMember;
                        result.passed = false;
                        goto CompoundCheckExit;
                    }

                    TypeCheckInfo membercheck = check_type_compatible(input_member->typedesc_ref,
                                                                      validator_member->typedesc_ref);
                    if (!membercheck.passed)
                    {
                        result.result = TypeCheckResult_MemberMismatch;
                        result.passed = false;
                        goto CompoundCheckExit;
                    }
                }

                { // Check pased
                    result.result = TypeCheckResult_Ok;
                    result.passed = true;
                }
            CompoundCheckExit:
                {}
            }
            break;
        }

        case TypeID::Union:
            if (input->type_id != TypeID::Union)
            {
                // If input is not a union, just need to find if input
                // type matches any of the union's type cases
                bool match_found = false;
                for (DynArrayCount i = 0,
                                   count = validator->union_type.type_cases.count;
                     i < count && !match_found;
                    ++i)
                {
                    TypeDescriptorRef typeref = *dynarray_get(validator->union_type.type_cases, i);
                    TypeCheckInfo check = check_type_compatible(input_typeref, typeref);
                    if (check.passed)
                    {
                        match_found = true;
                        break;
                    }
                }

                result.passed = match_found;
                result.result = match_found
                    ? TypeCheckResult_Ok
                    : TypeCheckResult_NoMatchForUnionCase;
            }
            else
            {
                // Need to see if the input union's type cases are
                // compatible with a subset of the validator union's
                // type cases

                for (DynArrayCount i = 0,
                                   icount = input->union_type.type_cases.count;
                     i < icount;
                     ++i)
                {
                    TypeDescriptorRef input_case_typeref = *dynarray_get(input->union_type.type_cases, i);
                    bool found_match = false;
                    for (DynArrayCount j = 0,
                             jcount = validator->union_type.type_cases.count;
                         j < jcount;
                         ++j)
                    {
                        TypeDescriptorRef validator_case_typeref = *dynarray_get(validator->union_type.type_cases, j);
                        TypeCheckInfo check = check_type_compatible(input_case_typeref, validator_case_typeref);
                        if (check.passed)
                        {
                            found_match = true;
                            break;
                        }
                    }

                    if (!found_match)
                    {
                        result.result = TypeCheckResult_MismatchedUnions;
                        result.passed = false;
                        goto UnionSubsetCheckExit;
                    }
                }

                result.result = TypeCheckResult_Ok;
                result.passed= true;

            UnionSubsetCheckExit:
                {}
            }
            break;
    }

    return result;
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
