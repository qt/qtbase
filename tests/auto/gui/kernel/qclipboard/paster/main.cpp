/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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

#ifndef Q_OS_WINCE
    QString expected;
    if (parser.isSet(textOption))
        expected = parser.value(textOption);
#else // !Q_OS_WINCE
    const QString expected = QStringLiteral("testString.!");
#endif // Q_OS_WINCE
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
