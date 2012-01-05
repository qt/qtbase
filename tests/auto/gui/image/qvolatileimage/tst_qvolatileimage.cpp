/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGui/qpainter.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/private/qvolatileimage_p.h>

class tst_QVolatileImage : public QObject
{
    Q_OBJECT

public:
    tst_QVolatileImage() { }

private slots:
    void create();
    void ensureFormat();
    void dataAccess();
    void sharing();
    void fill();
    void copy();
};

void tst_QVolatileImage::create()
{
    QVolatileImage nullImg;
    QVERIFY(nullImg.isNull());

    QVolatileImage img(100, 200, QImage::Format_ARGB32);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 100);
    QCOMPARE(img.height(), 200);
    QCOMPARE(img.format(), QImage::Format_ARGB32);
    QCOMPARE(img.byteCount(), img.bytesPerLine() * img.height());
    QCOMPARE(img.hasAlphaChannel(), true);
    QCOMPARE(img.depth(), 32);

    QImage source(12, 23, QImage::Format_ARGB32_Premultiplied);
    img = QVolatileImage(source);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 12);
    QCOMPARE(img.height(), 23);
    QCOMPARE(img.format(), source.format());
    QCOMPARE(img.byteCount(), img.bytesPerLine() * img.height());
    QVERIFY(img.imageRef() == source);
    QVERIFY(img.toImage() == source);
    QCOMPARE(img.hasAlphaChannel(), true);
    QCOMPARE(img.hasAlphaChannel(), img.imageRef().hasAlphaChannel());
    QCOMPARE(img.hasAlphaChannel(), img.toImage().hasAlphaChannel());
    QCOMPARE(img.depth(), 32);
}

void tst_QVolatileImage::ensureFormat()
{
    QImage source(12, 23, QImage::Format_ARGB32_Premultiplied);
    QVolatileImage img(source);
    QVERIFY(!img.isNull());
    QVERIFY(img.imageRef() == source);
    QVERIFY(img.toImage() == source);

    QVERIFY(img.ensureFormat(QImage::Format_ARGB32_Premultiplied)); // no-op
    QVERIFY(img.imageRef() == source);
    QVERIFY(img.toImage() == source);
    QVERIFY(img.format() == QImage::Format_ARGB32_Premultiplied);

    QVERIFY(img.ensureFormat(QImage::Format_RGB32)); // new data under-the-hood
    QVERIFY(img.imageRef() != source);
    QVERIFY(img.toImage() != source);
    QVERIFY(img.format() == QImage::Format_RGB32);
}

void tst_QVolatileImage::dataAccess()
{
    QImage source(12, 23, QImage::Format_ARGB32_Premultiplied);
    QVolatileImage img(source);
    QVERIFY(!img.isNull());
    img.beginDataAccess();
    QVERIFY(img.constBits());
    QVERIFY(img.imageRef().constBits());
    QVERIFY(img.bits());
    QVERIFY(img.imageRef().bits());
    img.endDataAccess();

    img = QVolatileImage(12, 23, QImage::Format_ARGB32);
    img.beginDataAccess();
    QVERIFY(img.constBits() && img.bits());
    img.endDataAccess();
}

void tst_QVolatileImage::sharing()
{
    QVolatileImage img1(100, 100, QImage::Format_ARGB32);
    QVolatileImage img2 = img1;
    img1.beginDataAccess();
    img2.beginDataAccess();
    QVERIFY(img1.constBits() == img2.constBits());
    img2.endDataAccess();
    img1.endDataAccess();
    img1.imageRef(); // non-const call, should detach
    img1.beginDataAccess();
    img2.beginDataAccess();
    QVERIFY(img1.constBits() != img2.constBits());
    img2.endDataAccess();
    img1.endDataAccess();

    // toImage() should return a copy of the internal QImage.
    // imageRef() is a reference to the internal QImage.
    QVERIFY(img1.imageRef().constBits() != img1.toImage().constBits());
}

bool fuzzyCompareImages(const QImage &image1, const QImage &image2, int tolerance)
{
    if (image1.bytesPerLine() != image2.bytesPerLine()
            || image1.width() != image2.width()
            || image1.height() != image2.height()) {
        return false;
    }
    for (int i = 0; i < image1.height(); i++) {
        const uchar *line1 = image1.scanLine(i);
        const uchar *line2 = image2.scanLine(i);
        int bytes = image1.bytesPerLine();
        for (int j = 0; j < bytes; j++) {
            int delta = line1[j] - line2[j];
            if (qAbs(delta) > tolerance)
                return false;
        }
    }
    return true;
}

void tst_QVolatileImage::fill()
{
    QVolatileImage img(100, 100, QImage::Format_ARGB32_Premultiplied);
    QColor col = QColor(10, 20, 30);
    img.fill(col.rgba());
    QVERIFY(img.imageRef().pixel(1, 1) == col.rgba());
    QVERIFY(img.toImage().pixel(1, 1) == col.rgba());
}

void tst_QVolatileImage::copy()
{
    QVolatileImage img(100, 100, QImage::Format_RGB32);
    img.beginDataAccess();
    img.imageRef().fill(QColor(Qt::green).rgba());
    QPainter p(&img.imageRef());
    p.drawRect(10, 10, 50, 50);
    p.end();
    img.endDataAccess();

    QVolatileImage img2(100, 100, QImage::Format_RGB32);
    img2.copyFrom(&img, QRect());
    QImage imgA = img.toImage();
    QImage imgB = img2.toImage();
    QCOMPARE(imgA.size(), imgB.size());
    QVERIFY(fuzzyCompareImages(imgA, imgB, 0));

    img2 = QVolatileImage(20, 20, QImage::Format_RGB32);
    img2.copyFrom(&img, QRect(5, 5, 20, 20));
    imgA = img.toImage().copy(5, 5, 20, 20);
    imgB = img2.toImage();
    QCOMPARE(imgA.size(), imgB.size());
    QVERIFY(fuzzyCompareImages(imgA, imgB, 0));
}

QTEST_MAIN(tst_QVolatileImage)
#include "tst_qvolatileimage.moc"
