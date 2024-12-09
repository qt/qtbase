// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mainwindow.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (a.arguments().size() > 5) {
        QString fontFamily = a.arguments().at(1);
        int fontSize = a.arguments().at(2).toInt();
        QString example = a.arguments().at(3);
        int weight = a.arguments().at(4).toInt();
        bool isItalic = a.arguments().at(5).toInt();

        QFont font(fontFamily);
        font.setPixelSize(fontSize);
        font.setWeight(QFont::Weight(weight));
        font.setItalic(isItalic);

        QTextLayout layout;
        layout.setFont(font);
        layout.setText(example);
        layout.beginLayout();
        layout.createLine();
        layout.endLayout();

        QRect brect = layout.boundingRect().toAlignedRect();

        QImage image(brect.size(), QImage::Format_RGB32);
        image.fill(Qt::white);
        image.setDevicePixelRatio(1.0);

        QPainter p;
        p.begin(&image);
        layout.draw(&p, -brect.topLeft());
        p.end();

        image.save(QStringLiteral("output.png"));

        return 0;
    } else {
        MainWindow w;
        w.show();
        return a.exec();
    }
}
