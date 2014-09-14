#include <stdio.h>
#include <unistd.h>

#include "ppapi/c/ppp.h"

extern "C" { int PpapiPluginMain(void); }
PP_EXPORT int run_main(int argc, char**argv);

int main(int argc, char **argv)
{
    puts("Hello World from main");
    fflush(stdout);
    sleep(1);
    puts("Hello World from main 2");
    fflush(stdout);
    
    run_main(argc, argv);
}

PP_EXPORT int run_main(int argc, char**argv)
{
    PpapiPluginMain();
}

PP_EXPORT int32_t PPP_InitializeModule(PP_Module a_module_id,
                                       PPB_GetInterface get_browser) {
  return 1;
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
  return NULL;
}

PP_EXPORT void PPP_ShutdownModule() {
}


