// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <qtest.h>
#include <QTest>
#include <QFile>
#include <QByteArray>
#include <QBuffer>
#include <QImageReader>
#include <QSize>

class tst_jpeg : public QObject
{
    Q_OBJECT
private slots:
    void jpegDecodingQtWebkitStyle();
};

void tst_jpeg::jpegDecodingQtWebkitStyle()
{
    // QtWebkit currently calls size() to get the image size for layouting purposes.
    // Then when it is in the viewport (we assume that here) it actually gets decoded.
    QString testFile = QFINDTESTDATA("n900.jpeg");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file n900.jpeg!");
    QFile inputJpeg(testFile);
    QVERIFY(inputJpeg.exists());
    inputJpeg.open(QIODevice::ReadOnly);
    QByteArray imageData = inputJpeg.readAll();
    QBuffer buffer;
    buffer.setData(imageData);
    buffer.open(QBuffer::ReadOnly);
    QCOMPARE(buffer.size(), qint64(19016));


    QBENCHMARK{
        for (int i = 0; i < 50; i++) {
            QImageReader reader(&buffer, "jpeg");
            QSize size = reader.size();
            QVERIFY(!size.isNull());
            QByteArray format = reader.format();
            QVERIFY(!format.isEmpty());
            QImage img = reader.read();
            QVERIFY(!img.isNull());
            buffer.reset();
        }
    }
}

QTEST_MAIN(tst_jpeg)

#include "jpeg.moc"
