/****************************************************************************
**
** Copyright (C) 2016 Kai Pastor
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
