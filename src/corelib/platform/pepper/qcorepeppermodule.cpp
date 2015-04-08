/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qcorepeppermodule.h"
#include "qcorepepperinstance.h"

#include <ppapi/c/ppp.h>

class QCorePepperModulePrivate
{
    
    
};

QCorePepperModule::QCorePepperModule()
:d(new QCorePepperModulePrivate)
{
    
}

QCorePepperModule::~QCorePepperModule()
{
    delete d;
}

bool QCorePepperModule::Init()
{
    return true;
}

pp::Instance* QCorePepperModule::CreateInstance(PP_Instance ppInstance)
{
    return new QCorePepperInstance(ppInstance);
}

// Helper function for Q_CORE_MAIN: allows creating a QPepperModule
// without pulling in the ppapi headers.
pp::Module *qtCoreCreatePepperModule()
{
    return new QCorePepperModule();
}
