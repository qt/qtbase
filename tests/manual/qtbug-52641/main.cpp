// Copyright (C) 2016 Kai Pastor
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
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
