#include "dynarray.h"
#include "hashtable.h"
#include "common.h"
#include "platform.h"


/*
Hash tables!


http://www.reedbeta.com/blog/2015/01/12/data-oriented-hash-table/
http://burtleburtle.net/bob/hash/spooky.html
http://www.sebastiansylvan.com/post/robin-hood-hashing-should-be-your-default-hash-table-implementation/
https://cs.uwaterloo.ca/research/tr/1986/CS-86-14.pdf

https://gist.github.com/tonious/1377667
https://www.quora.com/How-would-you-implement-a-hash-table-in-C-from-scratch
http://stackoverflow.com/questions/1138742/looking-for-a-good-hash-table-implementation-in-c
http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)HashTables.html
https://www.cs.cmu.edu/~fp/courses/15122-f10/lectures/22-mem/hashtable.c
https://www.cs.princeton.edu/courses/archive/fall06/cos217/lectures/09HashTables-2x2.pdf
http://www.cprogramming.com/tutorial/computersciencetheory/hash-table.html
http://www.ks.uiuc.edu/Research/vmd/doxygen/hash_8c-source.html

http://see-programming.blogspot.com/2013/05/closed-hashing-linear-probing.html
http://www.austincc.edu/comer/ds14s/hashtabl.htm
http://codereview.stackexchange.com/questions/18883/hash-table-using-open-addressing-with-linear-probing
https://github.com/fuzxxl/yz
https://en.wikipedia.org/wiki/List_of_hash_functions
http://www.cse.yorku.ca/~oz/hash.html
http://cstheory.stackexchange.com/questions/9639/how-did-knuth-derive-a
http://www.burtleburtle.net/bob/hash/doobs.html
https://github.com/aappleby/smhasher
http://www.burtleburtle.net/bob/c/lookup2.c
http://www.burtleburtle.net/bob/c/lookup3.c
https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

http://www.gamedev.net/topic/634740-help-with-string-hashing/
http://www.isthe.com/chongo/tech/comp/fnv/
https://en.wikipedia.org/wiki/MurmurHash

 */

/*
Strings and memory!

https://www.google.com/webhp?sourceid=chrome-instant&ion=1&espv=2&ie=UTF-8#q=managing+strings+in+C89+best+practices
https://en.wikibooks.org/wiki/C_Programming/Strings
http://www.drdobbs.com/managed-string-library-for-c/184402023?pgno=2
https://en.wikipedia.org/wiki/C_string_handling
http://stackoverflow.com/questions/17071460/best-practice-for-returning-a-variable-length-string-in-c
http://stackoverflow.com/questions/4580329/string-handling-practices-in-c
http://www.informit.com/articles/article.aspx?p=2036582&seqNum=4



 */


/*
Some bitsquid articles:

http://bitsquid.blogspot.com/2014/09/building-data-oriented-entity-system.html
http://bitsquid.blogspot.com/2015/12/data-driven-rendering-in-stingray.html
http://bitsquid.blogspot.com/2010/09/custom-memory-allocation-in-c.html
http://bitsquid.blogspot.com/2010/10/static-hash-values.html
http://bitsquid.blogspot.com/2010/10/time-step-smoothing.html
http://bitsquid.blogspot.com/2010/12/bitsquid-c-coding-style.html
http://bitsquid.blogspot.com/2011/01/managing-coupling.html
http://bitsquid.blogspot.com/2011/02/managing-decoupling-part-2-polling.html
http://bitsquid.blogspot.com/2011/02/some-systems-need-to-manipulate-objects.html
http://bitsquid.blogspot.com/2011/03/bitsquid-tech-benefits-of-data-driven.html
http://bitsquid.blogspot.com/2011/03/putting-some-of-power-of-programming.html
http://bitsquid.blogspot.com/2011/03/collaboration-and-merging.html
http://bitsquid.blogspot.com/2011/04/extreme-bug-hunting.html
http://bitsquid.blogspot.com/2011/04/universal-undo-copy-and-paste.html
http://bitsquid.blogspot.com/2011/05/flow-data-oriented-implementation-of.html
http://bitsquid.blogspot.com/2011/05/monitoring-your-game.html
http://bitsquid.blogspot.com/2011/06/strings-redux.html


 */


struct HashtableTest
{
    const char *name;
    u32 bucket_count;
    u64 begin_abstime;
    u64 end_abstime;
    u32 fail_count;
};


void test_fill_hashtable(HashtableTest *test)
{
    const s32 iterations = S32(test->bucket_count);

    OAHashtable<s32, s32> numbas;
    ht_init(&numbas, test->bucket_count);

    test->begin_abstime = query_abstime();
    for (s32 i = 0; i < iterations; ++i)
    {
        ht_set(&numbas, i, i);
    }
    test->end_abstime = query_abstime();

    ht_deinit(&numbas);
}


void test_hashtable_lookup(HashtableTest *test)
{
    const s32 iterations = S32(test->bucket_count);

    OAHashtable<s32, s32> numbas;
    ht_init(&numbas, test->bucket_count);

    for (s32 i = 0; i < iterations; ++i)
    {
        ht_set(&numbas, i, i);
    }

    test->fail_count = 0;
    test->begin_abstime = query_abstime();
    for (s32 i = 0; i < iterations; ++i)
    {
        s32 result = *ht_find(&numbas, i);
        if (result != i) {
            ++test->fail_count;
        }
    }
    test->end_abstime = query_abstime();

    // if (fail_count > 0) {
    //     printf_ln("\n%i cases failed", fail_count);
    // }

    ht_deinit(&numbas);
}


void test_hashtable_set_if_unset(HashtableTest *test)
{
    const s32 iterations = S32(test->bucket_count);

    OAHashtable<s32, s32> numbas;
    ht_init(&numbas, test->bucket_count);

    for (s32 i = 0; i < iterations; ++i)
    {
        if (i % 2 == 0)
        {
            ht_set(&numbas, i, i);
        }
        else
        {
            ht_set(&numbas, -i, i);
        }
    }

    test->fail_count = 0;
    test->begin_abstime = query_abstime();
    for (s32 i = 0; i < iterations; ++i)
    {
        bool key_was_negative = i % 2 != 0;
        bool was_occupied = ht_set_if_unset(&numbas, i, i);
        // If the key for this i value was flipped to negative, slot
        // should not have been occupied since we're now asking for
        // the non-negated key.
        if (key_was_negative == was_occupied)
        {
            ++test->fail_count;
        }
    }
    test->end_abstime = query_abstime();

    if (test->fail_count > 0) {
        printf_ln("\n%i cases failed", test->fail_count);
    }

    ht_deinit(&numbas);
}


s32 run_hashtable_tests()
{
    s32 total_fail_count = 0;

    { // Insertion
        const char *testname = "Hashtable Fill";
        const u32 limit = 100;
        DynArray<HashtableTest> tests = dynarray::init<HashtableTest>(limit);
        for (u32 i = 0; i < limit; ++i)
        {
            HashtableTest *test = dynarray::append(&tests);
            test->name = testname;
            test->bucket_count = i * 1000;
            test->fail_count = 0;
        }

        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("Running test '%s'   [%i]", test->name, i);
            test_fill_hashtable(test);
        }

        u32 fails = 0;
        printf_ln("Test '%s' Results:", testname);
        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("%i\t%f\tmilliseconds", test->bucket_count, milliseconds_since(test->end_abstime, test->begin_abstime));
            if (test->fail_count > 0) {
                printf_ln("FAILED: %i", test->fail_count);
                fails += test->fail_count;
            }
        }
        printf_ln("There were %i %s test failures", fails, testname);
        dynarray::deinit(&tests);
        total_fail_count += fails;
    }

    { // Lookup
        const char *testname = "Hashtable Lookup";
        const u32 limit = 100;
        DynArray<HashtableTest> tests = dynarray::init<HashtableTest>(limit);
        for (u32 i = 0; i < limit; ++i)
        {
            HashtableTest *test = dynarray::append(&tests);
            test->name = testname;
            test->bucket_count = i * 1000;
			test->fail_count = 0;
        }

        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("Running test '%s'   [%i]", test->name, i);
            test_hashtable_lookup(test);
        }

        
        u32 fails = 0;
        printf_ln("Test '%s' Results:", testname);
        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("%i\t%f\tmilliseconds", test->bucket_count, milliseconds_since(test->end_abstime, test->begin_abstime));
            if (test->fail_count > 0) {
                printf_ln("FAILED: %i", test->fail_count);
                fails += test->fail_count;
            }
        }
        printf_ln("There were %i %s test failures", fails, testname);
        dynarray::deinit(&tests);
        total_fail_count += fails;
    }

    { // Set-If-Unset
        const char *testname = "Hashtable Set-If-Unset";
        const u32 limit = 100;
        DynArray<HashtableTest> tests = dynarray::init<HashtableTest>(limit);
        for (u32 i = 0; i < limit; ++i)
        {
            HashtableTest *test = dynarray::append(&tests);
            test->name = testname;
            test->bucket_count = i * 1000;
			test->fail_count = 0;
        }

        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("Running test '%s'   [%i]", test->name, i);
            test_hashtable_set_if_unset(test);
        }


        u32 fails = 0;
        printf_ln("Test '%s' Results:", testname);
        for (u32 i = 0; i < tests.count; ++i)
        {
            HashtableTest *test = &tests[i];
            printf_ln("%i\t%f\tmilliseconds", test->bucket_count, milliseconds_since(test->end_abstime, test->begin_abstime));
            if (test->fail_count > 0) {
                printf_ln("FAILED: %i", test->fail_count);
                fails += test->fail_count;
            }
        }
        printf_ln("There were %i %s test failures", fails, testname);
        dynarray::deinit(&tests);
        total_fail_count += fails;
    }

    return total_fail_count;
}


// int main()
// {
//     run_hashtable_tests();
//     return 0;
// }
