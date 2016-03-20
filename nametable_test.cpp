#include "nametable.h"
#include "str.h"
#include "common.h"
#include "dynarray.h"



static const char *test_add_names[] = {
    "Animation",
    "Atten",
    "BoneCountArray",
    "BoneIndexArray",
    "BoneNode",
    "BoneRefArray",
    "BoneWeightArray",
    "CameraNode",
    "CameraObject",
    "Clip",
    "Color",
    "Extension",
    "GeometryNode",
    "GeometryObject",
    "IndexArray",
    "Key",
    "LightNode",
    "LightObject",
    "Material",
    "MaterialRef",
    "Metric",
    "Morph",
    "MorphWeight",
    "Name",
    "Node",
    "ObjectRef",
    "Param",
    "Rotation",
    "Scale",
    "Skeleton",
    "Skin",
    "Texture",
    "Time",
    "Track",
    "Transform",
    "Translation",
    "Value",
    "VertexArray",
    "Vector3f",
};

// NOTE(mike): These should be the same only shuffled
static const char * test_find_names[] = {
    "Node",
    "VertexArray",
    "Translation",
    "CameraNode",
    "GeometryObject",
    "ObjectRef",
    "Color",
    "BoneCountArray",
    "BoneIndexArray",
    "Texture",
    "Skin",
    "Scale",
    "LightNode",
    "GeometryNode",
    "Clip",
    "Atten",
    "Name",
    "Extension",
    "BoneRefArray",
    "Transform",
    "MaterialRef",
    "Material",
    "Rotation",
    "Time",
    "Metric",
    "Vector3f",
    "Value",
    "Skeleton",
    "Track",
    "BoneWeightArray",
    "BoneNode",
    "MorphWeight",
    "LightObject",
    "Morph",
    "CameraObject",
    "IndexArray",
    "Param",
    "Key",
    "Animation",        
};


struct NameTableTest
{
    StrSlice name;
    NameRef ref;
};


s32 run_nametable_tests()
{
    NameTable nt;
    nametable_init(&nt, 1024);

    // NameRef nametable_find_or_add(str_slice(""))
    DynArrayCount test_count = ARRAY_DIM(test_add_names);

    s32 add_fails = 0;
    s32 find_fails = 0;

    {
        DynArray<NameTableTest> tests;
        dynarray_init(&tests, test_count);
        for (DynArrayCount i = 0; i < test_count; ++i)
        {
            NameTableTest *test = dynarray_append(&tests);
            test->name = str_slice(test_find_names[i]);
        }    

        for (DynArrayCount i = 0; i < test_count; ++i)
        {
            NameTableTest *test = dynarray_get(tests, i);
            test->ref = nametable_find_or_add(&nt, test->name);

            if (!test->ref.offset)
            {
                printf_ln("Failed to add '%s' to NameTable", test->name.data);
                ++add_fails;                
            }
        }

        if (add_fails != 0)
        {
            printf_ln("There were %i NameTable test failures while adding, exiting test early", add_fails);

            // add test count, because failing here implies all subsequent tests fail too
            return add_fails + (s32)test_count;
        }
        else
        {
            println("No failures adding entries to NameTable");
        }

    }

    {
        DynArray<NameTableTest> tests;
        dynarray_init(&tests, test_count);
        for (DynArrayCount i = 0; i < test_count; ++i)
        {
            NameTableTest *test = dynarray_append(&tests);
            test->name = str_slice(test_find_names[i]);
        }

        for (DynArrayCount i = 0; i < test_count; ++i)
        {
            NameTableTest *test = dynarray_get(tests, i);

            test->ref = nametable_find(&nt, test->name);

            if (!test->ref.offset)
            {
                printf_ln("Test %i Failed to find '%s' in NameTable", i, test->name.data);
                ++find_fails;
            }
            else
            {
                StrSlice refslice = str_slice(test->ref);
                if (!str_equal(refslice, test->name))
                {
                    printf_ln("Test %i Failed to create valid slice: '%s' vs '%s'", i, refslice.data, test->name.data);
                }
            }
        }

        if (find_fails != 0)
        {
            printf_ln("There were %i failures while looking up entries in NameTable", find_fails);
        }
        else
        {
            println("No failures looking up entries in NameTable");
        }
    }

    return add_fails + find_fails;
}
