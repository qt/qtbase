/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qtquickcontrolsapplication.h"

#include <QtCore>
#include <QtGui>
#include <QtQml/QQmlApplicationEngine>

// (This example tests several startup and internal
//  implementation options. Final use is not intended
//  to be this complicated.)

#if 1 // use Q_GUI_MAIN startup

// optionally run Qt on a separate thread

// class First { public: First() { qputenv("QT_PEPPER_RUN_QT_ON_THREAD", "1"); } }; First first;

// optionally use a custom message loop instead of pp::MessageLoop
// class Second { public: Second() { qputenv("QT_PEPPER_USE_QT_MESSAGE_LOOP", "1"); } }; Second second;

QQmlApplicationEngine *engine;

void appInit(int argc, char **argv)
{
    engine = new QQmlApplicationEngine(QUrl("qrc:///main.qml"));
}

void appExit()
{
    delete engine;
}

Q_GUI_MAIN(appInit, appExit);

#else // use Q_GUI_BLOCKING_MAIN

// (implies running Qt on a separate thread.)

// optionally don't use pp::MessageLoop
class Second { public: Second() { qputenv("QT_PEPPER_USE_QT_MESSAGE_LOOP", "1"); } }; Second second;

int appMain(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine(QUrl("qrc:///main.qml"));
    return app.exec();
}

Q_GUI_BLOCKING_MAIN(appMain);

#endif
