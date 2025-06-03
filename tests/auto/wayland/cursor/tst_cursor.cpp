// Copyright (C) 2023 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"
#include <QtGui/QRasterWindow>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtWaylandClient/private/wayland-wayland-client-protocol.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

#include "cursorshapev1.h"

using namespace MockCompositor;

class tst_cursor : public QObject, private DefaultCompositor
{
    Q_OBJECT
public:
    tst_cursor();
    CursorShapeDevice* cursorShape();
private slots:
    void initTestCase();
    void cleanup() { QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage())); }
    void setCursor();
    void overrideCursor();
};

tst_cursor::tst_cursor()
{
    exec([this] {
        m_config.autoConfigure = true;
        add<CursorShapeManager>(1);
    });
}

CursorShapeDevice* tst_cursor::cursorShape()
{
    auto manager = get<CursorShapeManager>();
    if (!manager->m_cursorDevices.count())
        return nullptr;
    return manager->m_cursorDevices[0];
}

void tst_cursor::initTestCase()
{
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");
}

void tst_cursor::setCursor()
{
    QCOMPOSITOR_TRY_VERIFY(cursorShape());
    QSignalSpy setCursorSpy(exec([&] { return pointer(); }), &Pointer::setCursor);
    QSignalSpy setCursorShapeSpy(exec([&] { return cursorShape(); }), &CursorShapeDevice::setCursor);

    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    uint enterSerial = exec([&] {
        return pointer()->sendEnter(xdgSurface()->m_surface, {32, 32});
    });
    setCursorShapeSpy.wait();
    // verify we got given a cursor on enter
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_default);
    QVERIFY(setCursorSpy.isEmpty());
    QCOMPARE(setCursorShapeSpy.takeFirst().at(0).toUInt(), enterSerial);

    // client sets a different shape
    window.setCursor(QCursor(Qt::WaitCursor));
    QVERIFY(setCursorShapeSpy.wait());
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_wait);

    setCursorShapeSpy.clear();

    // client hides the cursor
    // CursorShape will not be used, instead, it uses the old path
    window.setCursor(QCursor(Qt::BlankCursor));
    QVERIFY(setCursorSpy.wait());
    QVERIFY(setCursorShapeSpy.isEmpty());
    QCOMPOSITOR_VERIFY(!pointer()->cursorSurface());

    // same for bitmaps
    QPixmap myCustomPixmap(10, 10);
    myCustomPixmap.fill(Qt::red);
    window.setCursor(QCursor(myCustomPixmap));
    QVERIFY(setCursorSpy.wait());
    QVERIFY(setCursorShapeSpy.isEmpty());

    // set a shape again
    window.setCursor(QCursor(Qt::BusyCursor));
    QVERIFY(setCursorShapeSpy.wait());
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_progress);

    setCursorShapeSpy.clear();

    // set the same bitmap again, make sure switching from new to old path works
    // even if the bitmap cursor's properties haven't changed
    window.setCursor(QCursor(myCustomPixmap));
    QVERIFY(setCursorSpy.wait());
    QVERIFY(setCursorShapeSpy.isEmpty());

    window.setCursor(QCursor(Qt::ArrowCursor));
}

void tst_cursor::overrideCursor()
{
    QCOMPOSITOR_TRY_VERIFY(cursorShape());
    QSignalSpy setCursorSpy(exec([&] { return pointer(); }), &Pointer::setCursor);
    QSignalSpy setCursorShapeSpy(exec([&] { return cursorShape(); }),
                                 &CursorShapeDevice::setCursor);

    QRasterWindow window1;
    window1.resize(64, 64);
    window1.setCursor(QCursor(Qt::OpenHandCursor));
    window1.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    QRasterWindow window2;
    window2.resize(64, 64);
    window2.setCursor(QCursor(Qt::PointingHandCursor));
    window2.show();

    // first window should be shape_grab
    uint enterSerial = exec([&] {
        const auto enterSerial = pointer()->sendEnter(xdgSurface(0)->m_surface, { 32, 32 });
        pointer()->sendFrame(client());
        return enterSerial;
    });
    QVERIFY(setCursorShapeSpy.wait());
    // verify we got given a cursor on enter
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_grab);
    QVERIFY(setCursorSpy.isEmpty());
    QCOMPARE(setCursorShapeSpy.takeFirst().at(0).toUInt(), enterSerial);

    // leave the previous surface
    exec([&] {
        pointer()->sendLeave(xdgSurface(0)->m_surface);
        pointer()->sendFrame(client());
    });
    setCursorShapeSpy.clear();

    // second window should be shape_pointer
    enterSerial = exec([&] {
        const auto enterSerial = pointer()->sendEnter(xdgSurface(1)->m_surface, { 32, 32 });
        pointer()->sendFrame(client());
        return enterSerial;
    });
    QVERIFY(setCursorShapeSpy.wait());
    // verify we got given a cursor on enter
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_pointer);
    QVERIFY(setCursorSpy.isEmpty());
    QCOMPARE(setCursorShapeSpy.takeFirst().at(0).toUInt(), enterSerial);

    // set the override cursor
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // verify it's set to shape_wait
    QVERIFY(setCursorShapeSpy.wait());
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_wait);
    QVERIFY(setCursorSpy.isEmpty());

    // now restore the override cursor
    QGuiApplication::restoreOverrideCursor();

    // it should be shape_pointer because we never left the second surface
    QVERIFY(setCursorShapeSpy.wait());
    QCOMPOSITOR_COMPARE(cursorShape()->m_currentShape, CursorShapeDevice::shape_pointer);
    QVERIFY(setCursorSpy.isEmpty());
}

QCOMPOSITOR_TEST_MAIN(tst_cursor)
#include "tst_cursor.moc"
