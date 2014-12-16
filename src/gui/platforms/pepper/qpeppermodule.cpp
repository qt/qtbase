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

#include "qpeppermodule.h"
#include "qpeppermodule_p.h"
#include "qpepperinstance.h"

extern QPepperModulePrivate *qtCreatePepperModulePrivate(QPepperModule *module);

QPepperModule::QPepperModule()
:d(qtCreatePepperModulePrivate(this))
{
    
}

QPepperModule::~QPepperModule()
{
    delete d;
}

bool QPepperModule::Init()
{
    return d->init();
}

pp::Instance* QPepperModule::CreateInstance(PP_Instance ppInstance)
{
    return  d->createInstance(ppInstance);
}

pp::Module *qtCreatePepperModule()
{
    return new QPepperModule();
}
