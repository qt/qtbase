/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#include <QApplication>
#include <QEventTransition>
#include <QPushButton>
#include <QStateMachine>
#include <QVBoxLayout>
#include <QWidget>

//! [0]
class Window : public QWidget
{
public:
    Window(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QPushButton *button = new QPushButton(this);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(button);
        layout->setContentsMargins(80, 80, 80, 80);
        setLayout(layout);
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
