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

#include "qpepperinstance.h"
#include "qpepperinstance_p.h"

extern QPepperInstancePrivate *qtCreatePepperInstancePrivate(QPepperInstance *instance);

QPepperInstance::QPepperInstance(PP_Instance instance)
: pp::Instance(instance)
, d(qtCreatePepperInstancePrivate(this))
{

}

bool QPepperInstance::Init(uint32_t argc, const char* argn[], const char* argv[])
{
    return d->init(argc, argn, argv);
}

void QPepperInstance::DidChangeView(const pp::View &view)
{
    d->didChangeView(view);
}

void QPepperInstance::DidChangeFocus(bool hasFucus)
{
    d->didChangeFocus(hasFucus);
}

bool QPepperInstance::HandleInputEvent(const pp::InputEvent& event)
{
    return d->handleInputEvent(event);
}

void QPepperInstance::HandleMessage(const pp::Var& message)
{
    d->handleMessage(message);
}

void QPepperInstance::applicationInit()
{
    // The default applicationInit() implementation calls the app_init
    // function registered with QT_GUI_MAIN.
    extern void qGuiAppInit();
    qGuiAppInit();
}
