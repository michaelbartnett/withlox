#include "types.h"
#include "programstate.h"


// TODO(mike): Guarantee no structural duplicates, and do it fast. Hashtable.
TypeRef find_equiv_typedescriptor(ProgramState *prgstate, TypeDescriptor *type_desc)
{
    TypeRef result = {};

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
            for (DynArrayCount i = 0,
                     count = prgstate->type_descriptors.count;
                 i < count;
                 ++i)
            {
                TypeRef ref = make_typeref(prgstate, i);
                TypeDescriptor *predefined = get_typedesc(ref);
                if (predefined && equal(type_desc, predefined))
                {
                    result = ref;
                    break;
                }
            }
            break;

        case TypeID::Union:
            for (DynArrayCount i = 0, count = prgstate->type_descriptors.count;
                 i < count;
                 ++i)
            {
                TypeRef ref = make_typeref(prgstate, i);
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


TypeRef find_equiv_type_or_add(ProgramState *prgstate, TypeDescriptor *type_desc, bool *new_type_added)
{
    TypeRef preexisting = find_equiv_typedescriptor(prgstate, type_desc);
    if (preexisting.index)
    {
        *new_type_added = false;
        return preexisting;
    }

    *new_type_added = true;
    return add_typedescriptor(prgstate, *type_desc, nullptr);
}



TypeRef add_typedescriptor(ProgramState *prgstate, TypeDescriptor type_desc, TypeDescriptor **ptr)
{
    TypeDescriptor *td = dynarray_append(&prgstate->type_descriptors, type_desc);

    if (ptr)
    {
        *ptr = td;
    }

    TypeRef result = make_typeref(prgstate, prgstate->type_descriptors.count - 1);
    return result;
}


TypeRef add_typedescriptor(ProgramState *prgstate, TypeDescriptor **ptr)
{
    TypeDescriptor td = {};
    return add_typedescriptor(prgstate, td, ptr);
}


TypeRef make_typeref(ProgramState *prgstate, u32 index)
{
    TypeRef result = {index, &prgstate->type_descriptors};
    return result;
}


TypeRef find_typeref_by_name(ProgramState *prgstate, NameRef name)
{
    TypeRef result = {};
    TypeRef *typeref = ht_find(&prgstate->typedesc_bindings, name);
    if (typeref)
    {
        result = *typeref;
    }
    return result;
}

TypeRef find_typeref_by_name(ProgramState *prgstate, StrSlice name)
{
    TypeRef result = {};
    NameRef nameref = nametable_find(&prgstate->names, name);
    if (nameref.offset)
    {
        result = find_typeref_by_name(prgstate, nameref);
    }
    return result;
}

TypeRef find_typeref_by_name(ProgramState *prgstate, Str name)
{
    return find_typeref_by_name(prgstate, str_slice(name));
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
    dynarray_init<CompoundTypeMember>(&result, src->count);

    for (DynArrayCount i = 0, e = src->count; i < e; ++i)
    {
        const CompoundTypeMember *src_member = &(*src)[i];
        CompoundTypeMember *dest_member = dynarray_append(&result);
        dest_member->name = src_member->name;
        dest_member->typeref = src_member->typeref;
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
            new_typedesc.union_type.type_cases = dynarray_clone(&src_typedesc->union_type.type_cases);
            break;
    }

    return new_typedesc;
}


// TODO(mike): FlatSet<T> container type or a dynarray_add_to_set template function
bool are_typerefs_unique(const DynArray<TypeRef> *types)
{
    for (DynArrayCount a = 0, count = types->count; a < count - 1; ++a)
    {
        for (DynArrayCount b = a + 1; b < count; ++b)
        {
            if (typeref_identical((*types)[a], (*types)[b]))
            {
                return false;
            }
        }
    }

    return true;
}


TypeRef make_union(ProgramState *prgstate, TypeRef a, TypeRef b)
{
    assert(!typeref_identical(a, b));
    TypeDescriptor new_typedesc = {};
    new_typedesc.type_id = TypeID::Union;

    TypeDescriptor *a_desc = get_typedesc(a);
    TypeDescriptor *b_desc = get_typedesc(b);

    bool a_is_union = a_desc->type_id == TypeID::Union;
    bool b_is_union = b_desc->type_id == TypeID::Union;
    bool both_unions = a_is_union && b_is_union;
    bool neither_unions = !a_is_union && !b_is_union;

    DynArray<TypeRef> *typecases = &new_typedesc.union_type.type_cases;

    if (neither_unions)
    {
        dynarray_init(typecases, 2);
        dynarray_append(typecases, a);
        dynarray_append(typecases, b);
    }
    else //if (both_unions)
    {
        // NOTE(mike): This whole fake array thing is kinda garbage, but I don't
        // have a good FlatSet container type, so all these set
        // operations have to be done manually at the moment, and this
        // seems to be the only way to avoid repeating the set
        // insertion loop 3-4 times.

        TypeRef spare_typeref = {};
        DynArray<TypeRef> spare_typecase_array = {&spare_typeref, 1, 1, nullptr};
        DynArray<TypeRef> *a_typecases;
        DynArray<TypeRef> *b_typecases;

        // In the case where only one of the types is a union, make a
        // fake typecase array that refers to stack instead of heap
        // memory
        if (! both_unions)
        {
            spare_typecase_array.data = &spare_typeref;
            spare_typecase_array.count = 1;
            spare_typecase_array.capacity = 1;

            if (! a_is_union)
            {
                assert(b_is_union);

                a_typecases = &spare_typecase_array;
                a_typecases->data[0] = a;

                b_typecases = &b_desc->union_type.type_cases;
            }
            else
            {
                assert(! b_is_union);

                a_typecases = &a_desc->union_type.type_cases;

                b_typecases = &spare_typecase_array;
                b_typecases->data[0] = b;
            }
        }
        else
        {
            a_typecases = &a_desc->union_type.type_cases;
            b_typecases = &b_desc->union_type.type_cases;
        }

        *typecases = dynarray_clone(a_typecases);
        DynArrayCount a_num_cases = a_typecases->count;
        DynArrayCount b_num_cases = b_typecases->count;
        dynarray_ensure_capacity(typecases, a_num_cases + b_num_cases);

        for (DynArrayCount i = 0; i < b_num_cases; ++i)
        {
            TypeRef b_case = b_typecases->at(i);
            bool already_contains_type = false;
            for (DynArrayCount j = 0; j < a_num_cases; ++j)
            {
                if (typeref_identical(b_case, a_typecases->at(j)))
                {
                    already_contains_type = true;
                    break;
                }
            }

            if (already_contains_type)
            {
                continue;
            }

            dynarray_append(typecases, b_case);
        }
    }

    bool new_type_added;
    TypeRef result = find_equiv_type_or_add(prgstate, &new_typedesc, &new_type_added);

    if (! new_type_added)
    {
        free_typedescriptor_components(&new_typedesc);
    }

    return result;
}


TypeRef merge_types(ProgramState *prgstate, TypeRef a, TypeRef b)
{
    const TypeDescriptor *a_desc = get_typedesc(a);
    const TypeDescriptor *b_desc = get_typedesc(b);

    bool a_is_compound = a_desc->type_id != TypeID::Compound;
    bool b_is_compound = b_desc->type_id != TypeID::Compound;
    bool neither_is_compound = ! (a_is_compound || b_is_compound);

    if (neither_is_compound || ( a_is_compound ^ b_is_compound ))
    {
        // If either A or B is a union, this function should merge them
        return make_union(prgstate, a, b);
    }

    assert(a_is_compound && b_is_compound);

    return compound_member_merge(prgstate, a, b);
}


TypeRef compound_member_merge(ProgramState *prgstate, TypeRef a, TypeRef b)
{
    const TypeDescriptor *a_desc = get_typedesc(a);
    const TypeDescriptor *b_desc = get_typedesc(b);

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
        else if (!typeref_identical(a_member->typeref, b_member->typeref))
        {
            // If the member exists, but is not the same type, then
            // replace that member's type with a type-merge
            a_member->typeref = merge_types(prgstate, a_member->typeref, b_member->typeref);
        }
        else
        {
            // If the member exists, and the types are identical, then do nothing
        }
    }

    TypeRef result = find_equiv_type_or_add(prgstate, &new_typedesc, nullptr);

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
            dynarray_deinit(&typedesc->compound_type.members);
            break;

        case TypeID::Union:
            dynarray_deinit(&typedesc->union_type.type_cases);
            break;
    }
}

