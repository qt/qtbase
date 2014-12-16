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

#ifndef QT_PEPPER_INSTANCE_H
#define QT_PEPPER_INSTANCE_H

#include <QtCore/qglobal.h>

#include <ppapi/cpp/instance.h>

class QPepperInstancePrivate;
class QPepperInstance : public pp::Instance
{
public:
    QPepperInstance(PP_Instance instance);

    bool Init(uint32_t argc, const char* argn[], const char* argv[]) Q_DECL_OVERRIDE;
    void DidChangeView(const pp::View &view) Q_DECL_OVERRIDE;
    void DidChangeFocus(bool hasFucus) Q_DECL_OVERRIDE;
    bool HandleInputEvent(const pp::InputEvent& event) Q_DECL_OVERRIDE;
    void HandleMessage(const pp::Var& message) Q_DECL_OVERRIDE;

    virtual void applicationInit();
private:
    QPepperInstancePrivate *d;
};

#endif
