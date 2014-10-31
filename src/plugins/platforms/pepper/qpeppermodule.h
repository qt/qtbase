/*
    Licensed under the MIT license.
*/
#ifndef QT_PEPPER_MODULE_H
#define QT_PEPPER_MODULE_H

#include "ppapi/cpp/module.h"
#include "ppapi/cpp/core.h"

class QtModule : public pp::Module {
public:
    QtModule();
    ~QtModule();
    virtual bool Init();
    virtual pp::Instance* CreateInstance(PP_Instance ppInstance);
    static pp::Core* core();
private:
};

#endif
