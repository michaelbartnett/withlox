#include "types.h"
#include "programstate.h"


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
