// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <qcursor.h>
#include <qpixmap.h>
#include <qbitmap.h>

#if defined(Q_OS_WIN)
#include <QtCore/qscopeguard.h>
#include <QtCore/qt_windows.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#endif

class tst_QCursor : public QObject
{
    Q_OBJECT

private slots:
    void equality();
#if defined(Q_OS_WIN)
    void overrideCursorScalesOnMixedDpiScreens();
#endif
};

#define VERIFY_EQUAL(lhs, rhs) \
    QVERIFY(lhs == rhs); \
    QVERIFY(rhs == lhs); \
    QVERIFY(!(rhs != lhs)); \
    QVERIFY(!(lhs != rhs))

#define VERIFY_DIFFERENT(lhs, rhs) \
    QVERIFY(lhs != rhs); \
    QVERIFY(rhs != lhs); \
    QVERIFY(!(rhs == lhs)); \
    QVERIFY(!(lhs == rhs))

void tst_QCursor::equality()
{
    VERIFY_EQUAL(QCursor(), QCursor());
    VERIFY_EQUAL(QCursor(Qt::CrossCursor), QCursor(Qt::CrossCursor));
    VERIFY_DIFFERENT(QCursor(Qt::CrossCursor), QCursor());

    // Shape
    QCursor shapeCursor(Qt::WaitCursor);
    VERIFY_EQUAL(shapeCursor, shapeCursor);
    QCursor shapeCursorCopy(shapeCursor);
    VERIFY_EQUAL(shapeCursor, shapeCursorCopy);
    shapeCursorCopy.setShape(Qt::DragMoveCursor);
    VERIFY_DIFFERENT(shapeCursor, shapeCursorCopy);
    shapeCursorCopy.setShape(shapeCursor.shape());
    VERIFY_EQUAL(shapeCursor, shapeCursorCopy);

    // Pixmap
    QPixmap pixmap(16, 16);
    QCursor pixmapCursor(pixmap);
    VERIFY_EQUAL(pixmapCursor, pixmapCursor);
    VERIFY_EQUAL(pixmapCursor, QCursor(pixmapCursor));
    VERIFY_EQUAL(pixmapCursor, QCursor(pixmap));
    VERIFY_DIFFERENT(pixmapCursor, QCursor());
    VERIFY_DIFFERENT(pixmapCursor, QCursor(pixmap, 5, 5));
    VERIFY_DIFFERENT(pixmapCursor, QCursor(QPixmap(16, 16)));
    VERIFY_DIFFERENT(pixmapCursor, shapeCursor);

    // Bitmap & mask
    QBitmap bitmap(16, 16);
    QBitmap mask(16, 16);
    QCursor bitmapCursor(bitmap, mask);
    VERIFY_EQUAL(bitmapCursor, bitmapCursor);
    VERIFY_EQUAL(bitmapCursor, QCursor(bitmapCursor));
    VERIFY_EQUAL(bitmapCursor, QCursor(bitmap, mask));
    VERIFY_DIFFERENT(bitmapCursor, QCursor());
    VERIFY_DIFFERENT(bitmapCursor, QCursor(bitmap, mask, 5, 5));
    VERIFY_DIFFERENT(bitmapCursor, QCursor(bitmap, QBitmap(16, 16)));
    VERIFY_DIFFERENT(bitmapCursor, QCursor(QBitmap(16, 16), mask));
    VERIFY_DIFFERENT(bitmapCursor, shapeCursor);
    VERIFY_DIFFERENT(bitmapCursor, pixmapCursor);

    // Empty pixmap
    for (int i = 0; i < 18; ++i)
        QTest::ignoreMessage(QtWarningMsg, "QCursor: Cannot create bitmap cursor; invalid bitmap(s)");

    QPixmap emptyPixmap;
    QCursor emptyPixmapCursor(emptyPixmap);
    QCOMPARE(emptyPixmapCursor.shape(), Qt::ArrowCursor);
    VERIFY_EQUAL(emptyPixmapCursor, QCursor());
    VERIFY_EQUAL(emptyPixmapCursor, QCursor(emptyPixmap, 5, 5));
    VERIFY_DIFFERENT(emptyPixmapCursor, shapeCursor);
    VERIFY_DIFFERENT(emptyPixmapCursor, pixmapCursor);
    VERIFY_DIFFERENT(emptyPixmapCursor, bitmapCursor);

    // Empty bitmap & mask
    QBitmap emptyBitmap;
    QCursor emptyBitmapCursor(emptyBitmap, emptyBitmap);
    QCOMPARE(emptyBitmapCursor.shape(), Qt::ArrowCursor);
    VERIFY_EQUAL(emptyBitmapCursor, QCursor());
    VERIFY_EQUAL(emptyBitmapCursor, QCursor(emptyBitmap, emptyBitmap, 5, 5));
    VERIFY_EQUAL(emptyBitmapCursor, QCursor(emptyBitmap, mask));
    VERIFY_EQUAL(emptyBitmapCursor, QCursor(bitmap, emptyBitmap));
    VERIFY_EQUAL(emptyBitmapCursor, emptyPixmapCursor);
    VERIFY_DIFFERENT(emptyBitmapCursor, shapeCursor);
    VERIFY_DIFFERENT(emptyBitmapCursor, pixmapCursor);
    VERIFY_DIFFERENT(emptyBitmapCursor, bitmapCursor);
}

#undef VERIFY_EQUAL
#undef VERIFY_DIFFERENT

#if defined(Q_OS_WIN)
// QTBUG-132709: setOverrideCursor for pixmap cursors must produce an HCURSOR
// sized for the DPI of the screen under the pointer.
void tst_QCursor::overrideCursorScalesOnMixedDpiScreens()
{
    const auto screens = QGuiApplication::screens();
    QScreen *a = screens.value(0);
    QScreen *b = nullptr;
    for (QScreen *s : screens) {
        if (s != a && !qFuzzyCompare(s->devicePixelRatio(), a->devicePixelRatio())) {
            b = s;
            break;
        }
    }
    if (!b)
        QSKIP("Needs two screens with different device pixel ratios.");

    auto hcursorWidth = [] {
        ICONINFO info{};
        if (!GetIconInfo(GetCursor(), &info))
            return 0;
        BITMAP bm{};
        GetObject(info.hbmMask ? info.hbmMask : info.hbmColor, sizeof(bm), &bm);
        if (info.hbmColor) DeleteObject(info.hbmColor);
        if (info.hbmMask)  DeleteObject(info.hbmMask);
        return int(bm.bmWidth);
    };

    QCursor::setPos(a->geometry().center());
    QGuiApplication::setOverrideCursor(QCursor(Qt::SplitVCursor));
    const int wa = hcursorWidth();
    QGuiApplication::restoreOverrideCursor();

    QCursor::setPos(b->geometry().center());
    QGuiApplication::setOverrideCursor(QCursor(Qt::SplitVCursor));
    const int wb = hcursorWidth();
    auto guard = qScopeGuard([]{ QGuiApplication::restoreOverrideCursor(); });

    QVERIFY(wa > 0 && wb > 0);
    const qreal expected = b->devicePixelRatio() / a->devicePixelRatio();
    const qreal measured = qreal(wb) / qreal(wa);
    QVERIFY2(qAbs(measured - expected) / expected < 0.15,
             qPrintable(QString("dpr %1->%2, width %3->%4")
                        .arg(a->devicePixelRatio()).arg(b->devicePixelRatio())
                        .arg(wa).arg(wb)));
}
#endif // Q_OS_WIN

QTEST_MAIN(tst_QCursor)
#include "tst_qcursor.moc"
