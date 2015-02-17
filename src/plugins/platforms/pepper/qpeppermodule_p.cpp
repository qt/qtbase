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
#include "qpepperinstance.h"
#include "qpeppermodule_p.h"

#include <ppapi/c/ppp.h>

static pp::Core *g_core = 0;
extern void *qtPepperModule; // QtCore

QPepperModulePrivate *qtCreatePepperModulePrivate(QPepperModule *module)
{
    return new QPepperModulePrivate(module);
}

QPepperModulePrivate::QPepperModulePrivate(QPepperModule *module)
    : q(module)
{
    qtPepperModule = module;
}

bool QPepperModulePrivate::init()
{
    g_core = q->core();
    return true;
}

pp::Instance *QPepperModulePrivate::createInstance(PP_Instance ppInstance)
{
    return new QPepperInstance(ppInstance);
}

pp::Core *QPepperModulePrivate::core() { return g_core; }
