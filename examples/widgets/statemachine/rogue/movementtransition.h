/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

#ifndef MOVEMENTTRANSITION_H
#define MOVEMENTTRANSITION_H

#include <QtWidgets>

#include "window.h"

//![0]
class MovementTransition : public QEventTransition
{
    Q_OBJECT

public:
    MovementTransition(Window *window) :
        QEventTransition(window, QEvent::KeyPress) {
        this->window = window;
    }
//![0]

//![1]
protected:
    bool eventTest(QEvent *event) Q_DECL_OVERRIDE {
        if (event->type() == QEvent::StateMachineWrapped &&
            static_cast<QStateMachine::WrappedEvent *>(event)->event()->type() == QEvent::KeyPress) {
            QEvent *wrappedEvent = static_cast<QStateMachine::WrappedEvent *>(event)->event();

            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(wrappedEvent);
            int key = keyEvent->key();

            return key == Qt::Key_2 || key == Qt::Key_8 || key == Qt::Key_6 ||
                   key == Qt::Key_4 || key == Qt::Key_Down || key == Qt::Key_Up ||
                   key == Qt::Key_Right || key == Qt::Key_Left;
        }
        return false;
    }
//![1]

//![2]
    void onTransition(QEvent *event) Q_DECL_OVERRIDE {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(
            static_cast<QStateMachine::WrappedEvent *>(event)->event());

        int key = keyEvent->key();
        switch (key) {
            case Qt::Key_Left:
            case Qt::Key_4:
                window->movePlayer(Window::Left);
                break;
            case Qt::Key_Up:
            case Qt::Key_8:
                window->movePlayer(Window::Up);
                break;
            case Qt::Key_Right:
            case Qt::Key_6:
                window->movePlayer(Window::Right);
                break;
            case Qt::Key_Down:
            case Qt::Key_2:
                window->movePlayer(Window::Down);
                break;
            default:
                ;
        }
    }
//![2]

private:
    Window *window;
};

#endif

