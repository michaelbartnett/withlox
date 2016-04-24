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

# DEFAULT_FLAGS=""

language_version='-std=c++-03'
if [ "$(uname)" == "Linux" ]; then
    language_version='-std=c++11'
fi

outfile=${0%.*}

echo "$compiler_command $language_version $DEFAULT_FLAGS $CFLAGS $0 memory.cpp -o \"$outfile\""
$compiler_command $language_version $DEFAULT_FLAGS $CFLAGS $0 memory.cpp -o "$outfile"
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


typedef void signal_handler(int);
// signal_handler *signal(int sig, signal_handler *func);
// fucking wow
// void (∗signal(int sig , void (∗func)(int) ))(int);

void handle_sigsegv(int sig)
{
    assert(sig == SIGSEGV);

    printf("BEGIN MEMCALL LOG\n");

    mem::log_memcalls();

    printf("\nEND MEMCALL LOG\n");

    signal(SIGSEGV, SIG_DFL);
    pid_t my_pid = getpid();
    kill(my_pid, SIGSEGV);
}


void run_memtest()
{
    // signal_handler *result = signal(SIGSEGV, &handle_sigsegv);
    // assert(result != SIG_ERR);

    vector<Allocation> allocations;
    mem::IAllocator *mallocator = mem::make_mallocator();
    const size_t align = 4;

    // void *ptrs[8];

    // cout << "\nNEW\n";
    // ptrs[0] = mallocator->realloc(0, 1, 4);
    // mallocator->log_allocations();

    // cout << "\nNEW\n";
    // ptrs[1] = mallocator->realloc(0, 1, 4);
    // mallocator->log_allocations();

    // cout << "\nREALLOC: " << mem::get_mallocator_header(mallocator, ptrs[1]) << "\n";
    // ptrs[2] = mallocator->realloc(ptrs[1], 1, 4);
    // mallocator->log_allocations();

    // cout << "\nNEW\n";
    // ptrs[3] = mallocator->realloc(0, 1, 4);
    // mallocator->log_allocations();

    // return 0;


    for (int i = 0; i < 100; ++i)
    {
        bool do_new_alloc = !allocations.size() || i % 3 != 0;
        size_t alloc_size = 31 * (i % 15) + 1;

        cout << i << ": ";

        if (do_new_alloc)
        {
            cout << "NEW ALLOC... ";
            void *ptr = mallocator->realloc(0, alloc_size, align);
            cout << mem::get_mallocator_header(mallocator, ptr) << "\n";
            Allocation new_alloc = {ptr, alloc_size, i};
            allocations.push_back(new_alloc);
        }
        else
        {
            Allocation old_alloc = allocations.back();
            allocations.pop_back();
            bool free_alloc = i % 5 == 0 && i % 3 == 0;
            free_alloc = false;
            if (free_alloc)
            {
                cout << "FREE ALLOC " << mem::get_mallocator_header(mallocator, old_alloc.ptr) << "\n";
                mallocator->dealloc(old_alloc.ptr);
            }
            else
            {
                cout << "REALLOC " << mem::get_mallocator_header(mallocator, old_alloc.ptr) << " to... ";
                void *ptr = mallocator->realloc(old_alloc.ptr, alloc_size, align);
                cout << mem::get_mallocator_header(mallocator, ptr) << "\n";
                Allocation new_alloc = {ptr, alloc_size, i};
                allocations.push_back(new_alloc);
            }
        }
        mallocator->log_allocations();
    }

    // return;

    cout << "\n\n\n\n"
         << "\n\n\n\n"
         << "\n\n\n\n"
         << "FREEING ALL THE THINGS"
         << "\n\n\n\n";

    mallocator->log_allocations();

    cout << "\n\n\n\n"
         << "\n\n\n\n"
         << "\n\n\n\n";


    for (vector<Allocation>::iterator it = allocations.begin(),end = allocations.end();
         it != end;
        ++it)
    {
        ptrdiff_t idx = (it - allocations.begin());

        if (idx == 66)
        {
            cout << "at index 47\n";
        }

        cout << "Freeing index " << idx << "\n";
        mallocator->dealloc(it->ptr);
    }

    delete mallocator;
}


int main()
{
    run_memtest();
    return 0;
}
