#include "typesys.h"
#include "programstate.h"


void load_base_type_descriptors(ProgramState *prgstate)
{
    ASSERT(prgstate);

    TypeDescriptor *none_type = add_typedescriptor(prgstate);
    none_type->type_id = TypeID::None;
    bind_typedesc_name(prgstate, TypeID::to_string(none_type->type_id),
                       none_type);
    prgstate->prim_none = none_type;

    TypeDescriptor *string_type = add_typedescriptor(prgstate);
    string_type->type_id = TypeID::String;
    bind_typedesc_name(prgstate, TypeID::to_string(string_type->type_id),
                       string_type);
    prgstate->prim_string = string_type;

    TypeDescriptor *int_type = add_typedescriptor(prgstate);
    int_type->type_id = TypeID::Int;
    bind_typedesc_name(prgstate, TypeID::to_string(int_type->type_id),
                       int_type);
    prgstate->prim_int = int_type;

    TypeDescriptor *float_type = add_typedescriptor(prgstate);
    float_type->type_id = TypeID::Float;
    bind_typedesc_name(prgstate, TypeID::to_string(float_type->type_id),
                       float_type);
    prgstate->prim_float = float_type;

    TypeDescriptor *bool_type = add_typedescriptor(prgstate);
    bool_type->type_id = TypeID::Bool;
    bind_typedesc_name(prgstate, TypeID::to_string(bool_type->type_id),
                       bool_type);
    prgstate->prim_bool = bool_type;
}


// TODO(mike): Guarantee no structural duplicates, and do it fast. Hashtable.
TypeDescriptor *find_equiv_typedesc(ProgramState *prgstate, TypeDescriptor *type_desc)
{
    TypeDescriptor *result = nullptr;
    BucketArray<TypeDescriptor> *typedesc_storage = &prgstate->type_descriptors;

    TYPESWITCH (type_desc->type_id)
    {
        // Builtins/primitives
        case TypeID::None:   result =  prgstate->prim_none;   break;
        case TypeID::String: result =  prgstate->prim_string; break;
        case TypeID::Int:    result =  prgstate->prim_int;    break;
        case TypeID::Float:  result =  prgstate->prim_float;  break;
        case TypeID::Bool:   result =  prgstate->prim_bool;   break;


        // Unique / user-defined
        case TypeID::Array:
        case TypeID::Compound:
        case TypeID::Union:
            for (BucketItemCount i = 0, e = prgstate->type_descriptors.capacity;
                 i < e; ++i)
            {
                TypeDescriptor *predefined;
                if (bucketarray::get_if_not_empty(&predefined, typedesc_storage, i))
                {
                    if (typedesc_equal(type_desc, predefined))
                    {
                        result = predefined;
                    }
                }
            }
            break;
    }

    return result;
}


TypeDescriptor *find_equiv_typedesc_or_add(ProgramState *prgstate, TypeDescriptor *typedesc, bool *new_type_added)
{
    TypeDescriptor *preexisting = find_equiv_typedesc(prgstate, typedesc);
    if (preexisting)
    {
        *new_type_added = false;
        return preexisting;
    }

    *new_type_added = true;
    return add_typedescriptor(prgstate, *typedesc);
}


TypeDescriptor *add_typedescriptor(ProgramState *prgstate, TypeDescriptor type_desc)
{
    TypeDescriptor *result = bucketarray::add(&prgstate->type_descriptors).elem;
    *result = type_desc;
    return result;
}


TypeDescriptor *add_typedescriptor(ProgramState *prgstate)
{
    TypeDescriptor td = {};
    return add_typedescriptor(prgstate, td);
}


void bind_typedesc_name(ProgramState *prgstate, NameRef name, TypeDescriptor *typedesc)
{
    ht_set(&prgstate->typedesc_bindings, name, typedesc);

    // ht_set(&prgstate->typedesc_reverse_bindings, typedesc, name);

    DynArray<NameRef> *names = nullptr;
    ht_set_if_unset(&names, &prgstate->typedesc_reverse_bindings, typedesc, dynarray::init<NameRef>(0));
    dynarray::append(names, name);
}


void bind_typedesc_name(ProgramState *prgstate, StrSlice name, TypeDescriptor *typedesc)
{
    NameRef nameref = nametable_find_or_add(&prgstate->names, name);
    bind_typedesc_name(prgstate, nameref, typedesc);
}


void bind_typedesc_name(ProgramState *prgstate, const char *name, TypeDescriptor *typedesc)
{
    bind_typedesc_name(prgstate, str_slice(name), typedesc);
}


void bind_typedesc_name(ProgramState *prgstate, Str name, TypeDescriptor *typedesc)
{
    bind_typedesc_name(prgstate, str_slice(name), typedesc);
}


DynArray<NameRef> *find_names_of_typedesc(ProgramState *prgstate, TypeDescriptor *typedesc)
{
    return ht_find(&prgstate->typedesc_reverse_bindings, typedesc);
}


DynArrayCount find_names_of_typedesc(OUTPARAM DynArray<NameRef> *result_list, ProgramState *prgstate, TypeDescriptor *typedesc)
{
    DynArray<NameRef> *names = ht_find(&prgstate->typedesc_reverse_bindings, typedesc);
    dynarray::append_from(names, result_list);
    return names->count;
}


TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, NameRef name)
{
    TypeDescriptor *result = nullptr;
    TypeDescriptor **ptrptr_typedesc = ht_find(&prgstate->typedesc_bindings, name);
    if (ptrptr_typedesc)
    {
        result = *ptrptr_typedesc;
    }
    return result;
}

TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, StrSlice name)
{
    TypeDescriptor *result = nullptr;
    NameRef nameref = nametable_find(&prgstate->names, name);
    if (nameref.offset)
    {
        result = find_typedesc_by_name(prgstate, nameref);
    }
    return result;
}

TypeDescriptor *find_typedesc_by_name(ProgramState *prgstate, Str name)
{
    return find_typedesc_by_name(prgstate, str_slice(name));
}


CompoundTypeMember *find_member(const TypeDescriptor *type_desc, NameRef name)
{
    DynArrayCount count = type_desc->compound_type.members.count;
    for (u32 i = 0; i < count; ++i)
    {
        CompoundTypeMember *mem = &type_desc->compound_type.members[i];

        if (nameref_identical(mem->name, name))
        {
            return mem;
        }
    }

    return nullptr;
}


DynArray<CompoundTypeMember> copy_compound_member_array(const DynArray<CompoundTypeMember> *src)
{
    DynArray<CompoundTypeMember> result;
    dynarray::init<CompoundTypeMember>(&result, src->count);

    for (DynArrayCount i = 0, e = src->count; i < e; ++i)
    {
        const CompoundTypeMember *src_member = &(*src)[i];
        CompoundTypeMember *dest_member = dynarray::append(&result);
        dest_member->name = src_member->name;
        dest_member->typedesc = src_member->typedesc;
    }

    return result;
}


TypeDescriptor copy_typedesc(const TypeDescriptor *src_typedesc)
{
    TypeDescriptor new_typedesc = {};
    new_typedesc.type_id = src_typedesc->type_id;

    TYPESWITCH (new_typedesc.type_id)
    {
        case TypeID::None:
        case TypeID::String:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
            // nothing to do for non-compound types
            break;

        case TypeID::Array:
            new_typedesc.array_type = src_typedesc->array_type;
            break;

        case TypeID::Compound:
            new_typedesc.compound_type.members = copy_compound_member_array(&src_typedesc->compound_type.members);
            break;

        case TypeID::Union:
            new_typedesc.union_type.type_cases = dynarray::clone(&src_typedesc->union_type.type_cases);
            break;
    }

    return new_typedesc;
}


// TODO(mike): FlatSet<T> container type or a dynarray::add_to_set template function
bool are_typedescs_unique(const DynArray<TypeDescriptor *> *types)
{
    for (DynArrayCount a = 0, count = types->count; a < count - 1; ++a)
    {
        for (DynArrayCount b = a + 1; b < count; ++b)
        {
            if ((*types)[a] == (*types)[b])
            {
                return false;
            }
        }
    }

    return true;
}


TypeDescriptor *make_union(ProgramState *prgstate, TypeDescriptor *a_desc, TypeDescriptor *b_desc)
{
    assert(a_desc != b_desc);
    TypeDescriptor new_typedesc = {};
    new_typedesc.type_id = TypeID::Union;

    bool a_is_union = tIS_UNION(a_desc);
    bool b_is_union = tIS_UNION(b_desc);
    bool both_unions = a_is_union && b_is_union;
    bool neither_unions = !a_is_union && !b_is_union;

    DynArray<TypeDescriptor *> *typecases = &new_typedesc.union_type.type_cases;

    if (neither_unions)
    {
        dynarray::init(typecases, 2);
        dynarray::append(typecases, a_desc);
        dynarray::append(typecases, b_desc);
    }
    else //if (both_unions)
    {
        // NOTE(mike): This whole fake array thing is kinda garbage, but I don't
        // have a good FlatSet container type, so all these set
        // operations have to be done manually at the moment, and this
        // seems to be the only way to avoid repeating the set
        // insertion loop 3-4 times.

        TypeDescriptor *spare_typedesc = nullptr;
        DynArray<TypeDescriptor *> spare_typecase_array = {&spare_typedesc, 1, 1, nullptr};
        DynArray<TypeDescriptor *> *a_typecases;
        DynArray<TypeDescriptor *> *b_typecases;

        // In the case where only one of the types is a union, make a
        // fake typecase array that refers to stack instead of heap
        // memory
        if (! both_unions)
        {
            spare_typecase_array.data = &spare_typedesc;
            spare_typecase_array.count = 1;
            spare_typecase_array.capacity = 1;

            if (! a_is_union)
            {
                assert(b_is_union);

                a_typecases = &spare_typecase_array;
                a_typecases->data[0] = a_desc;

                b_typecases = &b_desc->union_type.type_cases;
            }
            else
            {
                assert(! b_is_union);

                a_typecases = &a_desc->union_type.type_cases;

                b_typecases = &spare_typecase_array;
                b_typecases->data[0] = b_desc;
            }
        }
        else
        {
            a_typecases = &a_desc->union_type.type_cases;
            b_typecases = &b_desc->union_type.type_cases;
        }

        *typecases = dynarray::clone(a_typecases);
        DynArrayCount a_num_cases = a_typecases->count;
        DynArrayCount b_num_cases = b_typecases->count;
        dynarray::ensure_capacity(typecases, a_num_cases + b_num_cases);

        for (DynArrayCount i = 0; i < b_num_cases; ++i)
        {
            TypeDescriptor *b_case = b_typecases->at(i);
            bool already_contains_type = false;
            for (DynArrayCount j = 0; j < a_num_cases; ++j)
            {
                if (b_case != (*a_typecases)[j])
                {
                    already_contains_type = true;
                    break;
                }
            }

            if (already_contains_type)
            {
                continue;
            }

            dynarray::append(typecases, b_case);
        }
    }

    bool new_type_added;
    TypeDescriptor *result = find_equiv_typedesc_or_add(prgstate, &new_typedesc, &new_type_added);

    if (! new_type_added)
    {
        free_typedescriptor_components(&new_typedesc);
    }

    return result;
}


TypeDescriptor *merge_types(ProgramState *prgstate, /*const*/ TypeDescriptor *a_desc, /*const*/ TypeDescriptor *b_desc)
{
    bool a_is_compound = a_desc->type_id != TypeID::Compound;
    bool b_is_compound = b_desc->type_id != TypeID::Compound;
    bool neither_is_compound = ! (a_is_compound || b_is_compound);

    if (neither_is_compound || ( a_is_compound ^ b_is_compound ))
    {
        // If either A or B is a union, this function should merge them
        return make_union(prgstate, a_desc, b_desc);
    }

    assert(a_is_compound && b_is_compound);

    return compound_member_merge(prgstate, a_desc, b_desc);
}


TypeDescriptor *compound_member_merge(ProgramState *prgstate, TypeDescriptor *a_desc, TypeDescriptor *b_desc)
{
    assert(a_desc);
    assert(b_desc);
    ASSERT(tIS_COMPOUND(a_desc));
    ASSERT(tIS_COMPOUND(b_desc));

    TypeDescriptor new_typedesc = copy_typedesc(a_desc);

    for (DynArrayCount ib = 0; ib < b_desc->compound_type.members.count; ++ib)
    {
        CompoundTypeMember *b_member = &b_desc->compound_type.members[ib];
        CompoundTypeMember *a_member = find_member(a_desc, b_member->name);

        if (!a_member)
        {
            // If no member with matching name exists, add the member directly
            add_member(&new_typedesc, *b_member);
        }
        else if (a_member->typedesc != b_member->typedesc)
        {
            // If the member exists, but is not the same type, then
            // replace that member's type with a type-merge
            a_member->typedesc = merge_types(prgstate, a_member->typedesc, b_member->typedesc);
        }
        else
        {
            // If the member exists, and the types are identical, then do nothing
        }
    }

    TypeDescriptor *result = find_equiv_typedesc_or_add(prgstate, &new_typedesc, nullptr);

    return result;
}


void free_typedescriptor_components(TypeDescriptor *typedesc)
{
    TYPESWITCH (typedesc->type_id)
    {
        case TypeID::None:
        case TypeID::Int:
        case TypeID::Float:
        case TypeID::Bool:
        case TypeID::String:
        case TypeID::Array:
            // Nothing to do here
            break;

        case TypeID::Compound:
            dynarray::deinit(&typedesc->compound_type.members);
            break;

        case TypeID::Union:
            dynarray::deinit(&typedesc->union_type.type_cases);
            break;
    }
}


TypeCheckInfo check_type_compatible(TypeDescriptor *input, TypeDescriptor *validator)
{
    TypeCheckInfo result = {};
    if (input == validator)
    {
        result.result = TypeCheckResult_Ok;
        result.passed = true;
        return result;
    }

    if (validator->type_id != TypeID::Union && input->type_id != validator->type_id)
    {
        result.result = TypeCheckResult_MismatchedTypeID;
        result.passed = false;
        return result;
    }

    TYPESWITCH (validator->type_id)
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
            TypeCheckInfo elemcheck = check_type_compatible(input->array_type.elem_type,
                                                            validator->array_type.elem_type);

            result.passed = elemcheck.passed;
            if (!result.passed)
            {
                result.result = TypeCheckResult_ArrayElementTypeMismatch;
            }
            break;
        }

        case TypeID::Compound:
        {
            if (input->compound_type.members.count != validator->compound_type.members.count)
            {
                result.passed = false;
                result.result = TypeCheckResult_MemberCountMismatch;
            }
            else
            {
                for (DynArrayCount i = 0,
                                   icount = validator->compound_type.members.count;
                     i < icount;
                    ++i)
                {
                    CompoundTypeMember *validator_member = &validator->compound_type.members[i];
                    CompoundTypeMember *input_member = nullptr;
                    for (DynArrayCount j = 0,
                                       jcount = input->compound_type.members.count;
                         j < jcount;
                         ++j)
                    {
                        CompoundTypeMember *mem = &input->compound_type.members[j];
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

                    TypeCheckInfo membercheck = check_type_compatible(input_member->typedesc,
                                                                      validator_member->typedesc);
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
                    TypeDescriptor *typedesc = validator->union_type.type_cases[i];
                    TypeCheckInfo check = check_type_compatible(input, typedesc);
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
                    TypeDescriptor *input_case = input->union_type.type_cases[i];
                    bool found_match = false;
                    for (DynArrayCount j = 0,
                             jcount = validator->union_type.type_cases.count;
                         j < jcount;
                         ++j)
                    {
                        TypeDescriptor *validator_case = validator->union_type.type_cases[j];
                        TypeCheckInfo check = check_type_compatible(input_case, validator_case);
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


bool typemember_equal(const CompoundTypeMember *a, const CompoundTypeMember *b)
{
    return nameref_identical(a->name, b->name) && (a->typedesc == b->typedesc);
}


bool typedesc_equal(const TypeDescriptor *a, const TypeDescriptor *b)
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
                if (!b_member || !typemember_equal(a_member, b_member))
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


CompoundValueMember *find_member(const Value *value, NameRef name)
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


Value clone(const Value *src)
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


bool value_equal(const Value &lhs, const Value &rhs)
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


void value_free_components(Value *value)
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
                value_free_components(element);
            }
            break;

        case TypeID::Compound:            
            for (u32 i = 0; i < value->compound_value.members.count; ++i)
            {
                CompoundValueMember *member = &value->compound_value.members[i];
                value_free_components(&member->value);
            }
            dynarray::deinit(&value->compound_value.members);
            break;

        case TypeID::Union:
            ASSERT_MSG("There must never be a value of type Union");
    }

    ZERO_PTR(value);
}


// struct ValueEqual
// {
//     bool operator()(const Value &lhs, const Value &rhs)
//     {
//         return value_equal(lhs, rhs);
//     }
// };

