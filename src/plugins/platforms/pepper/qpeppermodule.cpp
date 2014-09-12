/*
    Licensed under the MIT license.
*/
#include "qpeppermodule.h"
#include "qpepperinstance.h"

#include <GLES2/gl2.h>
#include <ppapi/gles2/gl2ext_ppapi.h>
#include <ppapi/c/ppp.h>

static pp::Core *g_core = 0;

QtModule::QtModule()
{

}

QtModule::~QtModule()
{

}

bool QtModule::Init()
{
    g_core = Module::core();
//    bool glOk = glInitializePPAPI(get_browser_interface()) == GL_TRUE;

    return true;
}

pp::Instance* QtModule::CreateInstance(PP_Instance instance)
{
    return new QPepperInstance(instance);
}

pp::Core *QtModule::core()
{
    return g_core;
}

// This is the Pepper main entry point. We create a QtModule, which
// will subsequently create a QPepperInstance.
namespace pp {
    PP_EXPORT Module* CreateModule() {
        return new QtModule();
    }
}


