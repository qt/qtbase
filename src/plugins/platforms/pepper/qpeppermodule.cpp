/*
    Licensed under the MIT license.
*/
#include "qpeppermodule.h"
#include "qpepperinstance.h"

#include <GLES2/gl2.h>
#include <ppapi/gles2/gl2ext_ppapi.h>
#include <ppapi/c/ppp.h>

static pp::Core *g_core = 0;
extern void *qtPepperInstance; // QtCore
extern void *qtPepperModule; // QtCore

QtModule::QtModule()
{

}

QtModule::~QtModule()
{

}

bool QtModule::Init()
{
    g_core = Module::core();
    return true;
}

pp::Instance* QtModule::CreateInstance(PP_Instance ppInstance)
{
    QPepperInstance *instance = new QPepperInstance(ppInstance);
    // Grant non-platform plugin parts of Qt access to the instance.
    qtPepperInstance = instance;
    return instance;
}

pp::Core *QtModule::core()
{
    return g_core;
}

// This is the Pepper main entry point. We create a QtModule, which
// will subsequently create a QPepperInstance.
namespace pp {
    PP_EXPORT Module* CreateModule() {
        QtModule *module = new QtModule();
        // Grant non-platform plugin parts of Qt access to the module.
        qtPepperModule = module;
        return module;
    }
}
