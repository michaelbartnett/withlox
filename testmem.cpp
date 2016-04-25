#if 0
compiler_command=$CC
if [ "$compiler_command" == "" ]; then
    echo Using CLANG!
    compiler_command="clang++"
fi

DEFAULT_FLAGS="\
-g \
-pedantic \
-Werror \
-Wno-error=unused-parameter \
-Wno-error=unused-variable \
-Wno-switch-enum \
-Wno-error=unreachable-code \
-Wno-unused-function \
-Wno-long-long \
-Wno-unused-macros \
-Wno-logical-op-parentheses \
-Wno-padded \
-Wno-weak-vtables \
-Wno-variadic-macros \
-Wno-old-style-cast \
-Wno-float-equal \
"

language_version='-std=c++03'
if [ "$(uname)" == "Linux" ]; then
    # The whole libc++ situation on linux is dumb
    language_version='-std=c++11'
fi

outfile=${0%.*}

echo "$compiler_command -DCPPSCRIPT_MAIN=1 $language_version $DEFAULT_FLAGS $CFLAGS $0 memory.cpp -o \"$outfile\""
$compiler_command -DCPPSCRIPT_MAIN=1 $language_version $DEFAULT_FLAGS $CFLAGS $0 memory.cpp -o "$outfile"
compiled_ok=$?
if [ "$compiled_ok" == "0" -a "$COMPILE_ONLY" != "1" ]; then
    $outfile
fi
exit

#endif


#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include "memory.h"

using std::size_t;
using std::vector;
using std::cout;


struct Allocation
{
    void *ptr;
    size_t size;
    int i;
};


void test_logf(void *userdata, const char *fmt, ...)
{
    UNUSED(userdata);
    va_list vargs;
    va_start(vargs, fmt);
    std::vprintf(fmt, vargs);
    va_end(vargs);
}

void run_memtest()
{
    mem::memory_init(&test_logf, nullptr);

    vector<Allocation> allocations;
    mem::Mallocator mallocator;
    const size_t align = 4;


    for (int i = 0; i < 100; ++i)
    {
        bool do_new_alloc = !allocations.size() || i % 3 != 0;
        size_t alloc_size = 31 * (i % 15) + 1;

        cout << i << ": ";

        if (do_new_alloc)
        {
            cout << "NEW ALLOC... ";
            void *ptr = mallocator.realloc(0, alloc_size, align);
            cout << mallocator.get_header(ptr) << "\n";
            Allocation new_alloc = {ptr, alloc_size, i};
            allocations.push_back(new_alloc);
        }
        else
        {
            Allocation old_alloc = allocations.back();
            allocations.pop_back();
            bool free_alloc = i % 5 == 0 && i % 3 == 0;
            if (free_alloc)
            {
                cout << "FREE ALLOC " << mallocator.get_header(old_alloc.ptr) << "\n";
                mallocator.dealloc(old_alloc.ptr);
            }
            else
            {
                cout << "REALLOC " << mallocator.get_header(old_alloc.ptr) << " to... ";
                void *ptr = mallocator.realloc(old_alloc.ptr, alloc_size, align);
                cout << mallocator.get_header(ptr) << "\n";
                Allocation new_alloc = {ptr, alloc_size, i};
                allocations.push_back(new_alloc);
            }
        }
        mallocator.log_allocations();
    }

    cout << "\n\n\n\n"
         << "FREEING ALL THE THINGS"
         << "\n\n\n\n";

    for (vector<Allocation>::iterator it = allocations.begin(),end = allocations.end();
         it != end;
        ++it)
    {
        ptrdiff_t idx = (it - allocations.begin());
        cout << "Freeing index " << idx << "\n";
        mallocator.dealloc(it->ptr);
    }
}


#if defined(CPPSCRIPT_MAIN) && CPPSCRIPT_MAIN
int main()
{
    run_memtest();
    return 0;
}
#endif
