#include "types.h"
#include "programstate.h"


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
                if (bucketarray::get_if_not_empty(typedesc_storage, i, &predefined))
                {
                    if (equal(type_desc, predefined))
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
}


void bind_typedesc_name(ProgramState *prgstate, const char *name, TypeDescriptor *typedesc)
{
    NameRef nameref = nametable_find_or_add(&prgstate->names, str_slice(name));
    bind_typedesc_name(prgstate, nameref, typedesc);
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

    bool a_is_union = a_desc->type_id == TypeID::Union;
    bool b_is_union = b_desc->type_id == TypeID::Union;
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
    assert(a_desc->type_id == TypeID::Compound);
    assert(b_desc->type_id == TypeID::Compound);

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
                    // TypeRef typeref = validator->union_type.type_cases[i];
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
