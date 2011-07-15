/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtGui/private/qpixmapdata_p.h>
#include <QtGui/private/qnativeimagehandleprovider_p.h>
#include <QScopedPointer>
#include <QPixmap>
#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_OPENVG)
#include <fbs.h>
#include <bitdev.h>
#include <QtOpenVG/private/qpixmapdata_vg_p.h>
#endif

QPixmap pixmapFromNativeImageHandleProvider(QNativeImageHandleProvider *source)
{
#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_OPENVG)
    if (!source)
        return QPixmap();
    QScopedPointer<QPlatformPixmap> pd(QPlatformPixmap::create(0, 0, QPlatformPixmap::PixmapType));
    pd->fromNativeType(source, QPlatformPixmap::NativeImageHandleProvider);
    return QPixmap(pd.take());
#else
    Q_UNUSED(source);
    return QPixmap();
#endif
}

class DummyProvider : public QNativeImageHandleProvider
{
public:
    void get(void **handle, QString *type);
    void release(void *handle, const QString &type);
};

void DummyProvider::get(void **handle, QString *type)
{
    *handle = (void *) 0x12345678;
    *type = "some dummy type";
}

void DummyProvider::release(void *handle, const QString &type)
{
    Q_UNUSED(handle);
    Q_UNUSED(type);
}

#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_OPENVG)
class BitmapProvider : public QNativeImageHandleProvider
{
public:
    BitmapProvider() : bmp(0), refCount(0), w(50), h(60) { }
    void get(void **handle, QString *type);
    void release(void *handle, const QString &type);

    CFbsBitmap *bmp;
    int refCount, w, h;
    void *returnedHandle;
    QString returnedType;
};

void BitmapProvider::get(void **handle, QString *type)
{
    // There may not be a release() if the get() fails so don't bother with
    // refcounting in such cases.
    if (bmp)
        ++refCount;
    returnedType = QLatin1String("CFbsBitmap");
    returnedHandle = bmp;
    *handle = returnedHandle;
    *type = returnedType;
}

void BitmapProvider::release(void *handle, const QString &type)
{
    if (handle == returnedHandle && type == returnedType && returnedHandle) {
        --refCount;
    }
}
#endif // symbian & openvg

class tst_NativeImageHandleProvider : public QObject
{
    Q_OBJECT

public:
    tst_NativeImageHandleProvider() { }

private slots:
    void create();
    void bitmap();
    void hibernate();
};

void tst_NativeImageHandleProvider::create()
{
    QPixmap pm = pixmapFromNativeImageHandleProvider(0);
    QVERIFY(pm.isNull());
    QPixmap tmp(10, 20);
    if (tmp.handle()->classId() == QPlatformPixmap::OpenVGClass) {
        // Verify that null pixmap is properly returned when get() provides bogus results.
        DummyProvider prov;
        pm = pixmapFromNativeImageHandleProvider(&prov);
        QVERIFY(pm.isNull());
        pm = QPixmap();
    } else {
        QSKIP("Not openvg, skipping non-trivial tests", SkipSingle);
    }
}

void tst_NativeImageHandleProvider::bitmap()
{
#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_OPENVG)
    QPixmap tmp(10, 20);
    if (tmp.handle()->classId() == QPlatformPixmap::OpenVGClass) {
        BitmapProvider prov;

        // This should fail because of null ptr.
        QPixmap pm = pixmapFromNativeImageHandleProvider(&prov);
        QVERIFY(pm.isNull());
        pm = QPixmap();
        QCOMPARE(prov.refCount, 0);

        prov.bmp = new CFbsBitmap;
        QCOMPARE(prov.bmp->Create(TSize(prov.w, prov.h), EColor16MAP), KErrNone);
        CFbsBitmapDevice *bitmapDevice = CFbsBitmapDevice::NewL(prov.bmp);
        CBitmapContext *bitmapContext = 0;
        QCOMPARE(bitmapDevice->CreateBitmapContext(bitmapContext), KErrNone);
        TRgb symbianColor = TRgb(255, 200, 100);
        bitmapContext->SetBrushColor(symbianColor);
        bitmapContext->Clear();
        delete bitmapContext;
        delete bitmapDevice;

        pm = pixmapFromNativeImageHandleProvider(&prov);
        QVERIFY(!pm.isNull());
        QCOMPARE(pm.width(), prov.w);
        QCOMPARE(pm.height(), prov.h);
        QVERIFY(prov.refCount == 1);
        QImage img = pm.toImage();
        QVERIFY(prov.refCount == 1);
        QRgb pix = img.pixel(QPoint(1, 2));
        QCOMPARE(qRed(pix), symbianColor.Red());
        QCOMPARE(qGreen(pix), symbianColor.Green());
        QCOMPARE(qBlue(pix), symbianColor.Blue());

        pm = QPixmap(); // should result in calling release
        QCOMPARE(prov.refCount, 0);
        delete prov.bmp;
    } else {
         QSKIP("Not openvg", SkipSingle);
    }
#else
    QSKIP("Not applicable", SkipSingle);
#endif
}

void tst_NativeImageHandleProvider::hibernate()
{
#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_OPENVG)
    QPixmap tmp(10, 20);
    if (tmp.handle()->classId() == QPlatformPixmap::OpenVGClass) {
        BitmapProvider prov;
        prov.bmp = new CFbsBitmap;
        QCOMPARE(prov.bmp->Create(TSize(prov.w, prov.h), EColor16MAP), KErrNone);

        QPixmap pm = pixmapFromNativeImageHandleProvider(&prov);
        QCOMPARE(prov.refCount, 1);

        QVGPlatformPixmap *vgpd = static_cast<QVGPlatformPixmap *>(pm.handle());
        vgpd->hibernate();
        QCOMPARE(prov.refCount, 0);

        // Calling toVGImage() may cause some warnings as we don't have a gui initialized,
        // but the only thing we care about here is get() being called.
        vgpd->toVGImage();
        QCOMPARE(prov.refCount, 1);

        pm = QPixmap();
        QCOMPARE(prov.refCount, 0);
        delete prov.bmp;
    } else {
         QSKIP("Not openvg", SkipSingle);
    }
#else
    QSKIP("Not applicable", SkipSingle);
#endif
}

int main(int argc, char *argv[])
{
    QApplication::setGraphicsSystem("openvg");
    QApplication app(argc, argv);
    tst_NativeImageHandleProvider tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_nativeimagehandleprovider.moc"
