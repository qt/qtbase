/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtTest/QTest>
#include <qcursor.h>
#include <qpixmap.h>
#include <qbitmap.h>

class tst_QCursor : public QObject
{
    Q_OBJECT

private slots:
    void equality();
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

QTEST_MAIN(tst_QCursor)
#include "tst_qcursor.moc"
