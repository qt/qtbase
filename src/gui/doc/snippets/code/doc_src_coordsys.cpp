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
#include <QPainter>
#include <QWidget>

namespace doc_src_coordsys {
struct SnippetWrappers : public QWidget
{
    void wrapper0();
    void wrapper1();
    void wrapper2();
    void wrapper3();
    void wrapper4();
};


void SnippetWrappers::wrapper0() {

//! [0]
QPainter painter(this);

painter.setPen(Qt::darkGreen);
// Using the (x  y  w  h) overload
painter.drawRect(1, 2, 6, 4);
//! [0]

} // wrapper0


void SnippetWrappers::wrapper1() {

//! [1]
QPainter painter(this);

painter.setPen(Qt::darkGreen);
painter.drawLine(2, 7, 6, 1);
//! [1]

} // wrapper2


void SnippetWrappers::wrapper2() {

//! [2]
QPainter painter(this);
painter.setRenderHint(
    QPainter::Antialiasing);
painter.setPen(Qt::darkGreen);
// Using the (x  y  w  h) overload
painter.drawRect(1, 2, 6, 4);
//! [2]

} // wrapper2


void SnippetWrappers::wrapper3() {

//! [3]
QPainter painter(this);
painter.setRenderHint(
    QPainter::Antialiasing);
painter.setPen(Qt::darkGreen);
painter.drawLine(2, 7, 6, 1);
//! [3]

} // wrapper3


void SnippetWrappers::wrapper4() {

//! [4]
QPainter painter(this);
painter.setWindow(QRect(-50, -50, 100, 100));
//! [4]


//! [5]
int side = qMin(width(), height());
int x = (width() - side / 2);
int y = (height() - side / 2);

painter.setViewport(x, y, side, side);
//! [5]

} // wrapper4

} // doc_src_coordsys
