/****************************************************************************
**
** Copyright (C) 2016 Kai Pastor
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <QFileDialog>
#include <QPainter>
#include <QPdfWriter>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString filepath = QFileDialog::getSaveFileName(nullptr, "Save File", "",
                                                    "PDF files (*.pdf)");
    if (filepath.isEmpty())
        return 1;
    QPdfWriter writer(filepath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainterPath path;
    path.moveTo(0,0);
    path.lineTo(1000,0);
    path.lineTo(1000,1000);
    path.lineTo(0,800);
    path.lineTo(500,100);
    path.lineTo(800,900);
    path.lineTo(300,600);

    QPen pen;
    pen.setWidth(30);
    pen.setJoinStyle(Qt::MiterJoin);

    // The black path on the first page must always be visible in the PDF viewer.
    QPainter p(&writer);
    pen.setMiterLimit(6.0);
    p.setPen(pen);
    p.drawPath(path);

    // If a miter limit below 1.0 is written to the PDF,
    // broken PDF viewers may not show the red path on the second page.
    writer.newPage();
    pen.setMiterLimit(0.6);
    pen.setColor(Qt::red);
    p.setPen(pen);
    p.drawPath(path);

    p.end();
    return 0;
}
