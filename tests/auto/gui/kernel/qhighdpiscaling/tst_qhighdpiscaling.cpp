/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformscreen.h>

#include <QtTest/QtTest>

class tst_QHighDpiScaling: public QObject
{
    Q_OBJECT

private slots:
    void factor();
    void scale();
};

// Emulate the case of a High DPI secondary screen
class MyPlatformScreen : public QPlatformScreen
{
public:
    QRect geometry() const override { return QRect(3840, 0, 3840, 1920); }
    QRect availableGeometry() const override { return geometry(); }

    int depth() const override { return 32; }
    QImage::Format format() const override { return QImage::Format_ARGB32_Premultiplied; }
};

void tst_QHighDpiScaling::factor()
{
    QHighDpiScaling::setGlobalFactor(2);

    // Verfy that QHighDpiScaling::factor() does not crash on nullptr contexts.
    QPoint fakeNativePosition = QPoint(5, 5);
    QPlatformScreen *screenContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(screenContext) >= 0);
    QVERIFY(QHighDpiScaling::factor(screenContext, &fakeNativePosition) >= 0);
    QPlatformScreen *platformScreenContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(platformScreenContext) >= 0);
    QVERIFY(QHighDpiScaling::factor(platformScreenContext, &fakeNativePosition) >= 0);
    QWindow *windowContext = nullptr;
    QVERIFY(QHighDpiScaling::factor(windowContext) >= 0);
    QVERIFY(QHighDpiScaling::factor(windowContext, &fakeNativePosition) >= 0);
}

// QTBUG-77255: Test some scaling overloads
void tst_QHighDpiScaling::scale()
{
    QHighDpiScaling::setGlobalFactor(2);
    QScopedPointer<QPlatformScreen> screen(new MyPlatformScreen);

    qreal nativeValue = 10;
    const qreal value = QHighDpi::fromNativePixels(nativeValue, screen.data());
    QCOMPARE(value, qreal(5));
    QCOMPARE(QHighDpi::toNativePixels(value, screen.data()), nativeValue);

    // 10, 10 within screen should translate to 5,5 with origin preserved
    const QPoint nativePoint = screen->geometry().topLeft() + QPoint(10, 10);
    const QPoint point = QHighDpi::fromNativePixels(nativePoint, screen.data());
    QCOMPARE(point, QPoint(3845, 5));
    QCOMPARE(QHighDpi::toNativePixels(point, screen.data()), nativePoint);

    const QPointF nativePointF(nativePoint);
    const QPointF pointF = QHighDpi::fromNativePixels(nativePointF, screen.data());
    QCOMPARE(pointF, QPointF(3845, 5));
    QCOMPARE(QHighDpi::toNativePixels(pointF, screen.data()), nativePointF);
}

#include "tst_qhighdpiscaling.moc"
QTEST_MAIN(tst_QHighDpiScaling);
