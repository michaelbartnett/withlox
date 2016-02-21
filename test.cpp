#include "numeric_types.h"
#include "common.h"

s32 run_hashtable_tests();
s32 run_nametable_tests();


void run_tests()
{
    s32 fail_count = 0;
    fail_count += run_hashtable_tests();
    fail_count += run_nametable_tests();

    printf_ln("%i tests failed", fail_count);
}
