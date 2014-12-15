/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qpeppermodule.h>
#include <QtGui/qpepperinstance.h>
#include <QtQuick>

// This example shows how the app can have more control over
// Ppapi Module and Instance creation by implementing pp::CreateInstance(),
// and QPepperModule and QPepperInstance subclasses. These classes
// inherit from pp::Module and pp::Instance, repsectively.

class AppInstance : public QPepperInstance
{
public:
    AppInstance(PP_Instance ppInstance)
    :QPepperInstance(ppInstance)
    ,engine(0)
    {

    }

    ~AppInstance()
    {
        delete engine;
    }

    // The applicationInit() function is called when both Ppapi and 
    // Qt has been initialized, currently in the first call to DidChangeView
    // At this point the "platform" is ready - there is a QScreen with
    // correct geometry etc.
    void applicationInit()
    {
        engine = new QQmlApplicationEngine(QUrl("qrc:///main.qml"));
    }
    
    // The app may override any pp::Instance virtuals, but must
    // also call the QPepperInstance implementation. In the case
    // of messages, Qt messages are always prefixed with "qt",
    // and this may be used to filter messages.
    void HandleMessage(const pp::Var &message)
    {
        // Forward to Qt:
        QPepperInstance::HandleMessage(message);
    }
    
private:
    QQmlApplicationEngine *engine;
};

class AppModule : public QPepperModule {
public:
    bool Init()
    {
        return QPepperModule::Init();
    }

    pp::Instance* CreateInstance(PP_Instance ppInstance)
    {
        return new AppInstance(ppInstance);
    }
};

// Ppapi main entry point:
namespace pp { 
    pp::Module * CreateModule() { return new AppModule(); }
}
