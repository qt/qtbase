#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppapi/c/ppp.h"

extern "C" { int PpapiPluginMain(void); }

PP_EXPORT int run_main(int argc, char**argv)
{
   // Comment in this line to fail verification.
    PpapiPluginMain();
}
