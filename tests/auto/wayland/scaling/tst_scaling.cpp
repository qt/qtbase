// Copyright (C) 2022 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include <QtGui/QRasterWindow>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

using namespace MockCompositor;

class tst_scaling : public QObject, private DefaultCompositor
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void scaledWindow();
    void roundingPolicy_data();
    void roundingPolicy();

};

void tst_scaling::initTestCase()
{
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
}

void tst_scaling::scaledWindow()
{
    QRasterWindow window;
    window.resize(100, 100);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    QSignalSpy configureSpy(exec([&] { return xdgSurface(); }), &XdgSurface::configureCommitted);
    QSignalSpy surfaceCommitSpy(exec([&] { return surface(); }), &Surface::commit);

    const QSize configureSize(100, 100);

    exec([&] {
        QVERIFY(fractionalScale());
        fractionalScale()->send_preferred_scale(1.5 * 120);
        xdgToplevel()->sendCompleteConfigure(configureSize);
    });

    QTRY_COMPARE(configureSpy.count(), 1);
    QCOMPARE(window.devicePixelRatio(), 1.5);

    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), QSize(150, 150));
        Viewport *vp = viewport();
        QVERIFY(vp);
        QCOMPARE(vp->m_destination, QSize(100, 100));
    });

    // resize the window
    window.resize(200,200);
    QCOMPARE(window.size(), QSize(200,200));

    QVERIFY(surfaceCommitSpy.wait());
    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), QSize(300, 300));
        Viewport *vp = viewport();
        QVERIFY(vp);
        QCOMPARE(vp->m_destination, QSize(200, 200));
    });

    surfaceCommitSpy.clear();

    // dynamic scale change
    exec([&] {
        QVERIFY(fractionalScale());
        fractionalScale()->send_preferred_scale(2.5 * 120);
    });
    QTRY_COMPARE(window.devicePixelRatio(), 2.5);
    QCOMPARE(window.size(), QSize(200,200));
    QTRY_VERIFY(surfaceCommitSpy.count());

    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), QSize(500, 500));
        Viewport *vp = viewport();
        QVERIFY(vp);
        QCOMPARE(vp->m_destination, QSize(200, 200));
    });
}

void tst_scaling::roundingPolicy_data()
{
    QTest::addColumn<QSize>("windowSize");
    QTest::addColumn<qreal>("scale");
    QTest::addColumn<QSize>("expectedBufferSize");

    QTest::newRow("1.125 - round down") << QSize(10, 10) << 1.125 << QSize(11,11);
    QTest::newRow("1.25 - round up") << QSize(10, 10) << 1.25 << QSize(13,13);
    QTest::newRow("1.5 - don't round") << QSize(10, 10) << 1.5 << QSize(15,15);
}

void tst_scaling::roundingPolicy()
{
    QFETCH(QSize, windowSize);
    QFETCH(qreal, scale);
    QFETCH(QSize, expectedBufferSize);


    QRasterWindow window;
    window.resize(windowSize);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgToplevel());

    QSignalSpy surfaceCommitSpy(exec([&] { return surface(); }), &Surface::commit);

    exec([&] {
        QVERIFY(fractionalScale());
        fractionalScale()->send_preferred_scale(scale * 120);
        xdgToplevel()->sendCompleteConfigure();
    });

    QVERIFY(surfaceCommitSpy.wait());

    exec([&] {
        Buffer *buffer = xdgToplevel()->surface()->m_committed.buffer;
        QVERIFY(buffer);
        QCOMPARE(buffer->size(), expectedBufferSize);
    });
}


QCOMPOSITOR_TEST_MAIN(tst_scaling)
#include "tst_scaling.moc"
