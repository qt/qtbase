// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
