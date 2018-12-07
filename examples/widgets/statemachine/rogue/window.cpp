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

#include <QtWidgets>

#include "window.h"
#include "movementtransition.h"

//![0]
Window::Window()
{
    pX = 5;
    pY = 5;
//![0]

    QFontDatabase database;
    QFont font;
    if (database.families().contains("Monospace")) {
        font = QFont("Monospace");
    }
    else {
        const QStringList fontFamilies = database.families();
        for (const QString &family : fontFamilies ) {
            if (database.isFixedPitch(family)) {
                font = QFont(family);
                break;
            }
        }
    }
    font.setPointSize(12);
    setFont(font);

//![1]
    setupMap();
    buildMachine();
}
//![1]

void Window::setStatus(const QString &status)
{
    myStatus = status;
    repaint();
}

QString Window::status() const
{
    return myStatus;
}

void Window::paintEvent(QPaintEvent * /* event */)
{
    QFontMetrics metrics(font());
    QPainter painter(this);
    int fontHeight = metrics.height();
    int fontWidth = metrics.horizontalAdvance('X');
    int yPos = fontHeight;
    int xPos;

    painter.fillRect(rect(), Qt::black);
    painter.setPen(Qt::white);

    painter.drawText(QPoint(0, yPos), status());

    for (int y = 0; y < HEIGHT; ++y) {
        yPos += fontHeight;
        xPos = 0;

        for (int x = 0; x < WIDTH; ++x) {
            if (y == pY && x == pX) {
                xPos += fontWidth;
                continue;
            }

            painter.setPen(Qt::white);

            double x1 = static_cast<double>(pX);
            double y1 = static_cast<double>(pY);
            double x2 = static_cast<double>(x);
            double y2 = static_cast<double>(y);

            if (x2<x1) {
                x2+=0.5;
            } else if (x2>x1) {
                 x2-=0.5;
            }

            if (y2<y1) {
                 y2+=0.5;
            } else if (y2>y1) {
                 y2-=0.5;
            }

            double dx = x2 - x1;
            double dy = y2 - y1;

            double length = qSqrt(dx*dx+dy*dy);

            dx /= length;
            dy /= length;

            double xi = x1;
            double yi = y1;

            while (length > 0) {
                int cx = static_cast<int>(xi+0.5);
                int cy = static_cast<int>(yi+0.5);

                if (x2 == cx && y2 == cy)
                    break;

                if (!(x1==cx && y1==cy)
                    && (map[cx][cy] == '#' || (length-10) > 0)) {
                    painter.setPen(QColor(60,60,60));
                    break;
                }

                xi += dx;
                yi += dy;
                --length;
            }

            painter.drawText(QPoint(xPos, yPos), map[x][y]);
            xPos += fontWidth;
        }
    }
    painter.setPen(Qt::white);
    painter.drawText(QPoint(pX * fontWidth, (pY + 2) * fontHeight), QChar('@'));
}

QSize Window::sizeHint() const
{
    QFontMetrics metrics(font());

    return QSize(metrics.horizontalAdvance('X') * WIDTH, metrics.height() * (HEIGHT + 1));
}

//![2]
void Window::buildMachine()
{
    machine = new QStateMachine;

    QState *inputState = new QState(machine);
    inputState->assignProperty(this, "status", "Move the rogue with 2, 4, 6, and 8");

    MovementTransition *transition = new MovementTransition(this);
    inputState->addTransition(transition);
//![2]

//![3]
    QState *quitState = new QState(machine);
    quitState->assignProperty(this, "status", "Really quit(y/n)?");

    QKeyEventTransition *yesTransition = new
        QKeyEventTransition(this, QEvent::KeyPress, Qt::Key_Y);
    yesTransition->setTargetState(new QFinalState(machine));
    quitState->addTransition(yesTransition);

    QKeyEventTransition *noTransition =
        new QKeyEventTransition(this, QEvent::KeyPress, Qt::Key_N);
    noTransition->setTargetState(inputState);
    quitState->addTransition(noTransition);
//![3]

//![4]
    QKeyEventTransition *quitTransition =
        new QKeyEventTransition(this, QEvent::KeyPress, Qt::Key_Q);
    quitTransition->setTargetState(quitState);
    inputState->addTransition(quitTransition);
//![4]

//![5]
    machine->setInitialState(inputState);

    connect(machine, &QStateMachine::finished,
            qApp, &QApplication::quit);

    machine->start();
}
//![5]

void Window::movePlayer(Direction direction)
{
    switch (direction) {
        case Left:
            if (map[pX - 1][pY] != '#')
                --pX;
            break;
        case Right:
            if (map[pX + 1][pY] != '#')
                ++pX;
            break;
        case Up:
            if (map[pX][pY - 1] != '#')
                --pY;
            break;
        case Down:
            if (map[pX][pY + 1] != '#')
                ++pY;
            break;
    }
    repaint();
}

void Window::setupMap()
{
    for (int x = 0; x < WIDTH; ++x)
        for (int y = 0; y < HEIGHT; ++y) {
        if (x == 0 || x == WIDTH - 1 || y == 0 || y == HEIGHT - 1 || QRandomGenerator::global()->bounded(40) == 0)
            map[x][y] = '#';
        else
            map[x][y] = '.';
    }
}

