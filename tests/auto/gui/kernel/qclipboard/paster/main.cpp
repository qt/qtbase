// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QtCore/QStringList>
#include <QtCore/QCommandLineParser>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption textOption(QStringLiteral("text"),
                                  QStringLiteral("Text to compare"),
                                  QStringLiteral("text"));
    parser.addOption(textOption);
    QCommandLineOption imageOption(QStringLiteral("image"),
                                   QStringLiteral("Perform image check"));
    parser.addOption(imageOption);
    parser.process(QCoreApplication::arguments());

    if (parser.isSet(imageOption)) {
#ifndef QT_NO_CLIPBOARD
        const QImage actual = QGuiApplication::clipboard()->image();
#else
        const QImage actual;
#endif
        // Perform hard-coded checks on copied image (size, pixel 0,0: transparent,
        // pixel 1,0: blue). Note: Windows sets RGB of transparent to 0xFF when converting
        // to DIB5.
        if (actual.size() != QSize(100, 100))
            return 1;
        const QRgb pixel00 = actual.pixel(QPoint(0, 0));
        if (qAlpha(pixel00))
            return 2;
        const QRgb pixel01 = actual.pixel(QPoint(1, 0));
        if (pixel01 != QColor(Qt::blue).rgba())
            return 3;
        return 0;
    }

    QString expected;
    if (parser.isSet(textOption))
        expected = parser.value(textOption);
    if (!expected.isEmpty()) {
#ifndef QT_NO_CLIPBOARD
        const QString actual = QGuiApplication::clipboard()->text();
#else
        const QString actual;
#endif
        return actual == expected ? 0 : 1;
    }
    return -2;
}
