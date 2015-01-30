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
    // argn/argv will not be valid by the time the message loop processes
    // the message posted below. Make a copy
    QVector<QByteArray> vargn;
    QVector<QByteArray> vargv;
    for (uint32_t i = 0; i < argc; ++i) {
        vargn.push_back(argn[i]);
        vargv.push_back(argv[i]);
    }

    if (d->m_runQtOnThread) {
        d->processCall(d->m_callbackFactory.NewCallback(
                &QPepperInstancePrivate::init, argc, vargn, vargv));
    } else {
        uint32_t unused = 0;
        d->init(unused, argc, vargn, vargv);
    }
}

void QPepperInstance::DidChangeView(const pp::View &view)
{
    if (d->m_runQtOnThread) {
        d->processCall(d->m_callbackFactory.NewCallback(
            &QPepperInstancePrivate::didChangeView, view));
    } else {
        uint32_t unused = 0;
        d->didChangeView(unused, view);
    }
}

void QPepperInstance::DidChangeFocus(bool hasFucus)
{
    if (d->m_runQtOnThread) {
        d->processCall(d->m_callbackFactory.NewCallback(
            &QPepperInstancePrivate::didChangeFocus, hasFucus));
    } else {
        uint32_t unused = 0;
        d->didChangeFocus(unused, hasFucus);
    }
}

bool QPepperInstance::HandleInputEvent(const pp::InputEvent& event)
{
    if (d->m_runQtOnThread) {
        d->processCall(d->m_callbackFactory.NewCallback(
            &QPepperInstancePrivate::handleInputEvent, event));
        return true; // FIXME: Get result from async call above.
    } else {
        uint32_t unused = 0;
        return d->handleInputEvent(unused, event);
    }
}

void QPepperInstance::HandleMessage(const pp::Var& message)
{
    if (d->m_runQtOnThread) {
        d->processCall(d->m_callbackFactory.NewCallback(
            &QPepperInstancePrivate::handleMessage, message));
    } else {
        uint32_t unused = 0;
        d->handleMessage(unused, message);
    }
}

void QPepperInstance::applicationInit()
{
    // The default applicationInit() implementation calls the app_init
    // function registered with QT_GUI_MAIN.
    extern void qGuiAppInit();
    qGuiAppInit();
}
