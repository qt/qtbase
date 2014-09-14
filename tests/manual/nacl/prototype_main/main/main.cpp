#include <stdio.h>
#include <unistd.h>

#include "../lib/lib.h"

// App-provided init and exit functions:
void app_init(int argc, char **argv)
{
    puts("Hello World from app_init");
    fflush(stdout);
}

int app_exit()
{
    puts("Hello World from app_exit");
    fflush(stdout);
    return 0;
}

// Register functions with Qt. The type of register function
// selects the QApplicaiton type.
QT_CORE_REGISTER_APP_FUNCTIONS(app_init, app_exit);
//QT_GUI_REGISTER_APP_FUNCTIONS(app_init, app_exit);
//QT_WIDGETS_REGISTER_APP_FUNCTIONS(app_init, app_exit);


