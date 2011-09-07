#include "v8test.h"
#include <stdio.h>

#define RUN_TEST(testname) { \
    if (!v8test_ ## testname()) \
        printf ("Test %s FAILED\n", # testname); \
}

int main(int argc, char *argv[])
{
    v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

    RUN_TEST(eval);
    RUN_TEST(evalwithinwith);
    RUN_TEST(userobjectcompare);

    return -1;
}
