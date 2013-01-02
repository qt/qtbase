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

#include "circlewidget.h"
#include "window.h"

#include <QtWidgets>

//! [0]
Window::Window()
{
    aliasedLabel = createLabel(tr("Aliased"));
    antialiasedLabel = createLabel(tr("Antialiased"));
    intLabel = createLabel(tr("Int"));
    floatLabel = createLabel(tr("Float"));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(aliasedLabel, 0, 1);
    layout->addWidget(antialiasedLabel, 0, 2);
    layout->addWidget(intLabel, 1, 0);
    layout->addWidget(floatLabel, 2, 0);
//! [0]

//! [1]
    QTimer *timer = new QTimer(this);

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            circleWidgets[i][j] = new CircleWidget;
            circleWidgets[i][j]->setAntialiased(j != 0);
            circleWidgets[i][j]->setFloatBased(i != 0);

            connect(timer, SIGNAL(timeout()),
                    circleWidgets[i][j], SLOT(nextAnimationFrame()));

            layout->addWidget(circleWidgets[i][j], i + 1, j + 1);
        }
    }
//! [1] //! [2]
    timer->start(100);
    setLayout(layout);

    setWindowTitle(tr("Concentric Circles"));
}
//! [2]

//! [3]
QLabel *Window::createLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setMargin(2);
    label->setFrameStyle(QFrame::Box | QFrame::Sunken);
    return label;
}
//! [3]
