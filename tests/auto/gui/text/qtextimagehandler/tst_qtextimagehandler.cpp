/****************************************************************************
 **
 ** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QTest>

#include <QPainter>
#include <private/qtextimagehandler_p.h>

class tst_QTextImageHandler : public QObject
{
    Q_OBJECT

public:
    tst_QTextImageHandler();

private slots:
    void init();
    void cleanup();
    void cleanupTestCase();
    void loadAtNImages();
};

tst_QTextImageHandler::tst_QTextImageHandler()
{
}

void tst_QTextImageHandler::init()
{
}

void tst_QTextImageHandler::cleanup()
{
}

void tst_QTextImageHandler::cleanupTestCase()
{
}

void tst_QTextImageHandler::loadAtNImages()
{
    QTextDocument doc;
    QTextCursor c(&doc);
    c.insertHtml("<img src=\"data/image.png\">");
    QTextImageHandler handler;
    QTextImageFormat fmt;
    fmt.setName("data/image.png");

    for (int i = 1; i < 3; ++i) {
        QImage img(20, 20, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::white);
        img.setDevicePixelRatio(i);
        QPainter p(&img);
        handler.drawObject(&p, QRect(0, 0, 20, 20), &doc, 0, fmt);
        p.end();
        QVERIFY(!img.isNull());
        const auto expectedColor = i == 1 ? Qt::red : Qt::green;
        QCOMPARE(img.pixelColor(0, 0), expectedColor);
    }
}

QTEST_MAIN(tst_QTextImageHandler)
#include "tst_qtextimagehandler.moc"
