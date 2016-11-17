#include "programstate.h"


void collection_deinit(Collection *coll)
{
    ASSERT(coll->info.count == coll->value.array_value.elements.count);
    // ASSERT(coll->records.capacity = 0);

    for (DynArrayCount i = 0, e = coll->info.count; i < e; ++i)
    {
        recordinfo_deinit(&coll->info[i]);
    }
    str_free(&coll->load_path);

    value_free_components(&coll->value);
}


void prgstate_init(ProgramState *prgstate)
{
    nametable::init(&prgstate->names, MEGABYTES(2));
    bucketarray::init(&prgstate->type_descriptors);
    ht_init(&prgstate->typedesc_bindings);
    ht_init(&prgstate->typedesc_reverse_bindings);

    bucketarray::init(&prgstate->collections);

    ht_init(&prgstate->command_map);
    ht_init(&prgstate->value_map);

    dynarray::init(&prgstate->editing_collections, 0);
}


void drop_collection(ProgramState *prgstate, BucketIndex bucket_index)
{
    Collection *coll = &prgstate->collections[bucket_index];
    collection_assert_invariants(coll);
    collection_deinit(coll);

    DynArrayCount edit_index;
    if (dynarray::try_find_index(&edit_index, &prgstate->editing_collections, coll))
    {
        dynarray::swappop(&prgstate->editing_collections, edit_index);
    }

    bool removed = bucketarray::remove_at(&prgstate->collections, bucket_index);
    ASSERT(removed);
}
