/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

//! [0]
class Window : public QWidget
{
public:
    Window(QWidget *parent = 0)
        : QWidget(parent)
    {
        QPushButton *button = new QPushButton(this);
        button->setGeometry(QRect(100, 100, 100, 100));
//! [0]

//! [1]
        QStateMachine *machine = new QStateMachine(this);

        QState *s1 = new QState();
        s1->assignProperty(button, "text", "Outside");

        QState *s2 = new QState();
        s2->assignProperty(button, "text", "Inside");
//! [1]

//! [2]
        QEventTransition *enterTransition = new QEventTransition(button, QEvent::Enter);
        enterTransition->setTargetState(s2);
        s1->addTransition(enterTransition);
//! [2]

//! [3]
        QEventTransition *leaveTransition = new QEventTransition(button, QEvent::Leave);
        leaveTransition->setTargetState(s1);
        s2->addTransition(leaveTransition);
//! [3]

//! [4]
        QState *s3 = new QState();
        s3->assignProperty(button, "text", "Pressing...");

        QEventTransition *pressTransition = new QEventTransition(button, QEvent::MouseButtonPress);
        pressTransition->setTargetState(s3);
        s2->addTransition(pressTransition);

        QEventTransition *releaseTransition = new QEventTransition(button, QEvent::MouseButtonRelease);
        releaseTransition->setTargetState(s2);
        s3->addTransition(releaseTransition);
//! [4]

//! [5]
        machine->addState(s1);
        machine->addState(s2);
        machine->addState(s3);

        machine->setInitialState(s1);
        machine->start();
    }
};
//! [5]

//! [6]
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Window window;
    window.resize(300, 300);
    window.show();

    return app.exec();
}
//! [6]
