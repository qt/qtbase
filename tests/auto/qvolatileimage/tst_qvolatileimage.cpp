/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
#ifdef Q_OS_SYMBIAN
#include <fbs.h>
#endif

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
    void paint();
    void fill();
    void copy();
    void bitmap();
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

#ifdef Q_OS_SYMBIAN
    CFbsBitmap *bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(100, 50), EColor16MAP) == KErrNone);
    QVolatileImage bmpimg(bmp);
    QVERIFY(!bmpimg.isNull());
    QCOMPARE(bmpimg.width(), 100);
    QCOMPARE(bmpimg.height(), 50);
    // Verify that we only did handle duplication, not pixel data copying.
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    delete bmp;
    // Check if content is still valid.
    QImage copyimg = bmpimg.toImage();
    QCOMPARE(copyimg.format(), QImage::Format_ARGB32_Premultiplied);
#endif
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

#ifdef Q_OS_SYMBIAN
    CFbsBitmap *bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(100, 50), EColor16MAP) == KErrNone);
    QVolatileImage bmpimg(bmp);
    QVERIFY(bmpimg.ensureFormat(QImage::Format_ARGB32_Premultiplied)); // no-op
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());

    // A different format should cause data copying.
    QVERIFY(bmpimg.ensureFormat(QImage::Format_RGB32));
    QVERIFY(bmpimg.constBits() != (const uchar *) bmp->DataAddress());
    const uchar *prevBits = bmpimg.constBits();

    QVERIFY(bmpimg.ensureFormat(QImage::Format_RGB16));
    QVERIFY(bmpimg.constBits() != (const uchar *) bmp->DataAddress());
    QVERIFY(bmpimg.constBits() != prevBits);
    prevBits = bmpimg.constBits();

    QVERIFY(bmpimg.ensureFormat(QImage::Format_MonoLSB));
    QVERIFY(bmpimg.constBits() != (const uchar *) bmp->DataAddress());
    QVERIFY(bmpimg.constBits() != prevBits);

    delete bmp;
#endif
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

#ifdef Q_OS_SYMBIAN
    CFbsBitmap *bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(100, 50), EColor16MAP) == KErrNone);
    QVolatileImage bmpimg(bmp);
    QVolatileImage bmpimg2;
    bmpimg2 = bmpimg;
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    QCOMPARE(bmpimg2.constBits(), (const uchar *) bmp->DataAddress());
    // Now force a detach, which should copy the pixel data under-the-hood.
    bmpimg.imageRef();
    QVERIFY(bmpimg.constBits() != (const uchar *) bmp->DataAddress());
    QCOMPARE(bmpimg2.constBits(), (const uchar *) bmp->DataAddress());
    delete bmp;
#endif
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

void tst_QVolatileImage::paint()
{
#ifdef Q_OS_SYMBIAN
    QVolatileImage img(100, 100, QImage::Format_ARGB32);
    img.beginDataAccess();
    img.imageRef().fill(QColor(Qt::green).rgba());
    QPainter p(&img.imageRef());
    p.drawRect(10, 10, 50, 50);
    p.end();
    img.endDataAccess();
    QImage imgA = img.toImage();

    // The following assumes that on openvg the pixmapdata is backed by QVolatileImage)
    // (and that openvg is in use)
    // It should pass with any engine nonetheless.
    // See if painting into the underlying QImage succeeds.
    QPixmap pm(100, 100);
    if (pm.paintEngine()->type() == QPaintEngine::Raster) {
        pm.fill(Qt::green);
        QPainter pmp(&pm);
        pmp.drawRect(10, 10, 50, 50);
        pmp.end();
        QImage imgB = pm.toImage();
        QVERIFY(fuzzyCompareImages(imgA, imgB, 0));
        // Exercise the accelerated QVolatileImagePaintEngine::drawPixmap() a bit.
        QPixmap targetPm(pm.size());
        targetPm.fill(Qt::black);
        pmp.begin(&targetPm);
        pmp.drawPixmap(QPointF(0, 0), pm);
        pmp.end();
        imgB = targetPm.toImage();
        QVERIFY(fuzzyCompareImages(imgA, imgB, 0));
        // Now the overload taking rects.
        targetPm.fill(Qt::black);
        pmp.begin(&targetPm);
        QRectF rect(QPointF(0, 0), pm.size());
        pmp.drawPixmap(rect, pm, rect);
        pmp.end();
        imgB = targetPm.toImage();
        QVERIFY(fuzzyCompareImages(imgA, imgB, 0));
    } else {
        QSKIP("Pixmaps not painted via raster, skipping paint test", SkipSingle);
    }
#endif
}

void tst_QVolatileImage::fill()
{
    QVolatileImage img(100, 100, QImage::Format_ARGB32_Premultiplied);
    QColor col = QColor(10, 20, 30);
    img.fill(col.rgba());
    QVERIFY(img.imageRef().pixel(1, 1) == col.rgba());
    QVERIFY(img.toImage().pixel(1, 1) == col.rgba());

#ifdef Q_OS_SYMBIAN
    CFbsBitmap *bmp = static_cast<CFbsBitmap *>(img.duplicateNativeImage());
    QVERIFY(bmp);
    TRgb pix;
    bmp->GetPixel(pix, TPoint(1, 1));
    QCOMPARE(pix.Red(), col.red());
    QCOMPARE(pix.Green(), col.green());
    QCOMPARE(pix.Blue(), col.blue());
    delete bmp;
#endif
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

void tst_QVolatileImage::bitmap()
{
#ifdef Q_OS_SYMBIAN
    CFbsBitmap *bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(100, 50), EColor64K) == KErrNone);
    QVolatileImage bmpimg(bmp);
    CFbsBitmap *dupbmp = static_cast<CFbsBitmap *>(bmpimg.duplicateNativeImage());
    QVERIFY(dupbmp);
    QVERIFY(dupbmp != bmp);
    QCOMPARE(dupbmp->DataAddress(), bmp->DataAddress());
    delete dupbmp;
    delete bmp;
    bmpimg.beginDataAccess();
    qMemSet(bmpimg.bits(), 0, bmpimg.byteCount());
    qMemSet(bmpimg.bits(), 1, bmpimg.bytesPerLine() * bmpimg.height());
    bmpimg.endDataAccess();

    // Test bgr->rgb conversion in case of EColor16M.
    bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(101, 89), EColor16M) == KErrNone);
    bmp->BeginDataAccess();
    TUint32 *addr = bmp->DataAddress();
    uint rgb = QColor(10, 20, 30).rgb();
    qMemCopy(bmp->DataAddress(), &rgb, 3);
    bmp->EndDataAccess();
    TRgb symrgb;
    bmp->GetPixel(symrgb, TPoint(0, 0));
    QVERIFY(symrgb.Red() == 10 && symrgb.Green() == 20 && symrgb.Blue() == 30);
    bmpimg = QVolatileImage(bmp);
    QVERIFY(bmpimg.toImage().pixel(0, 0) == rgb);
    // check if there really was a conversion
    bmp->BeginDataAccess();
    bmpimg.beginDataAccess();
    qMemCopy(&rgb, bmpimg.constBits(), 3);
    uint rgb2 = rgb;
    qMemCopy(&rgb2, bmp->DataAddress(), 3);
    QVERIFY(rgb != rgb2);
    bmpimg.endDataAccess(true);
    bmp->EndDataAccess(true);
    delete bmp;

    bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(101, 89), EGray2) == KErrNone);
    bmpimg = QVolatileImage(bmp); // inverts pixels, but should do it in place
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    QCOMPARE(bmpimg.format(), QImage::Format_MonoLSB);
    bmpimg.ensureFormat(QImage::Format_ARGB32_Premultiplied);
    QVERIFY(bmpimg.constBits() != (const uchar *) bmp->DataAddress());
    QCOMPARE(bmpimg.format(), QImage::Format_ARGB32_Premultiplied);
    delete bmp;

    // The following two formats must be optimal always.
    bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(101, 89), EColor16MAP) == KErrNone);
    bmpimg = QVolatileImage(bmp);
    QCOMPARE(bmpimg.format(), QImage::Format_ARGB32_Premultiplied);
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    bmpimg.ensureFormat(QImage::Format_ARGB32_Premultiplied);
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    delete bmp;
    bmp = new CFbsBitmap;
    QVERIFY(bmp->Create(TSize(101, 89), EColor16MU) == KErrNone);
    bmpimg = QVolatileImage(bmp);
    QCOMPARE(bmpimg.format(), QImage::Format_RGB32);
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    bmpimg.ensureFormat(QImage::Format_RGB32);
    QCOMPARE(bmpimg.constBits(), (const uchar *) bmp->DataAddress());
    delete bmp;

#else
    QSKIP("CFbsBitmap is only available on Symbian, skipping bitmap test", SkipSingle);
#endif
}

int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("openvg");
    QApplication app(argc, argv);
    tst_QVolatileImage tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qvolatileimage.moc"
