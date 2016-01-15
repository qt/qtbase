/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
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
