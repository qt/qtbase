/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "wigglywidget.h"

#include <chrono>
#include <iostream>
#include <sstream>

std::chrono::steady_clock::time_point tp;
int frames = 0;
float fps = -1.0;

//! [0]
WigglyWidget::WigglyWidget(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Midlight);
    setAutoFillBackground(true);

    QFont newFont = font();
    newFont.setPointSize(newFont.pointSize() + 20);
    setFont(newFont);

    step = 0;
    timer.start(60, this);

    tp = std::chrono::steady_clock::now();
}
//! [0]

//! [1]
void WigglyWidget::paintEvent(QPaintEvent * /* event */)
//! [1] //! [2]
{
    auto dt = std::chrono::steady_clock::now() - tp;

    auto const interval = std::chrono::seconds(1);
    if (dt > interval)
    {
        float const seconds = std::chrono::duration_cast<std::chrono::duration<float>>(interval).count();
        fps = (float)frames/seconds;
        std::cout << "Rendered " << frames << " in " << seconds << " seconds. Fps: " << fps << std::endl;
        frames = 0;
        tp = std::chrono::steady_clock::now();
    }
    else
        ++frames;

    static const int sineTable[16] = {
        0, 38, 71, 92, 100, 92, 71, 38, 0, -38, -71, -92, -100, -92, -71, -38
    };

    QFontMetrics metrics(font());
    //int y = (height() + metrics.ascent() - metrics.descent()) / 2;
    QColor color;
//! [2]

//! [3]
    QPainter painter(this);
//! [3] //! [4]

    int sts = 20;
    int st = height() / (sts+1);

    for (int j = 0; j < sts; ++j)
    {
        int x = (width() - metrics.width(text)) / 2;
        int y = (j+1) * st;

        for (int i = 0; i < text.size(); ++i) {
            int index = (step + i) % 16;
            color.setHsv((15 - index) * 16, 255, 191);
            painter.setPen(color);
            painter.drawText(x, y - ((sineTable[index] * metrics.height()) / 400),
                            QString(text[i]));
            x += metrics.width(text[i]);
        }
    }

    std::ostringstream ss;
    if (fps > 0.0f)
        ss << "Fps: " << fps;
    else
        ss << "Fps: --";

    painter.drawText(10, 40, QString(ss.str().c_str()));
}
//! [4]

//! [5]
void WigglyWidget::timerEvent(QTimerEvent *event)
//! [5] //! [6]
{
    if (event->timerId() == timer.timerId()) {
        ++step;
        update();
    } else {
        QWidget::timerEvent(event);
    }
//! [6]
}
