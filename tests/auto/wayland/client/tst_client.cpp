// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"

#include <QBackingStore>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <QMimeData>
#include <QPixmap>
#include <QDrag>
#include <QWindow>
#if QT_CONFIG(opengl)
#include <QOpenGLWindow>
#endif

#include <QtTest/QtTest>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <QtGui/private/qguiapplication_p.h>

using namespace MockCompositor;

constexpr int dataDeviceVersion = 1;

class TestCompositor : public WlShellCompositor {
public:
    explicit TestCompositor()
    {
        exec([this] {
            m_config.autoConfigure = true;
            add<DataDeviceManager>(dataDeviceVersion);
        });
    }
    DataDevice *dataDevice() { return get<DataDeviceManager>()->deviceFor(get<Seat>()); }
};

class TestWindow : public QWindow
{
public:
    TestWindow()
    {
        setSurfaceType(QSurface::RasterSurface);
        setGeometry(0, 0, 32, 32);
        create();
    }

    void focusInEvent(QFocusEvent *) override
    {
        ++focusInEventCount;
    }

    void focusOutEvent(QFocusEvent *) override
    {
        ++focusOutEventCount;
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        ++keyPressEventCount;
        keyCode = event->nativeScanCode();
    }

    void keyReleaseEvent(QKeyEvent *event) override
    {
        ++keyReleaseEventCount;
        keyCode = event->nativeScanCode();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        ++mousePressEventCount;
        mousePressPos = event->position().toPoint();
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        ++mouseReleaseEventCount;
    }

    void touchEvent(QTouchEvent *event) override
    {
        Q_UNUSED(event);
        ++touchEventCount;
    }

    void exposeEvent(QExposeEvent *event) override {
        Q_UNUSED(event);
        const QRect rect(QPoint(), size());

        if (!backingStore)
            backingStore.reset(new QBackingStore(this));
        backingStore->resize(rect.size());
        backingStore->beginPaint(rect);

        const QColor color = Qt::magenta;
        QPainter p(backingStore->paintDevice());
        p.fillRect(rect, color);
        p.end();

        backingStore->endPaint();
        backingStore->flush(rect);
    }

    QPoint frameOffset() const { return QPoint(frameMargins().left(), frameMargins().top()); }

    int focusInEventCount = 0;
    int focusOutEventCount = 0;
    int keyPressEventCount = 0;
    int keyReleaseEventCount = 0;
    int mousePressEventCount = 0;
    int mouseReleaseEventCount = 0;
    int touchEventCount = 0;

    uint keyCode = 0;
    QPoint mousePressPos;
    QScopedPointer<QBackingStore> backingStore;
};

#if QT_CONFIG(opengl)
class TestGlWindow : public QOpenGLWindow
{
    Q_OBJECT

public:
    TestGlWindow();
    int paintGLCalled = 0;

public slots:
    void hideShow();

protected:
    void paintGL() override;
};

TestGlWindow::TestGlWindow()
{}

void TestGlWindow::hideShow()
{
    setVisible(false);
    setVisible(true);
}

void TestGlWindow::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    ++paintGLCalled;
}
#endif // QT_CONFIG(opengl)

class tst_WaylandClient : public QObject, private TestCompositor
{
    Q_OBJECT

private slots:
    void cleanup() {
        QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
    }
    void createDestroyWindow();
    void activeWindowFollowsKeyboardFocus();
    void events();
    void backingStore();
    void touchDrag();
    void mouseDrag();
    void dontCrashOnMultipleCommits();
    void hiddenTransientParent();
    void hiddenPopupParent();
#if QT_CONFIG(opengl)
    void glWindow();
#endif // QT_CONFIG(opengl)
    void longWindowTitle();
    void longWindowTitleWithUtf16Characters();
};

void tst_WaylandClient::createDestroyWindow()
{
    TestWindow window;
    window.show();

    QCOMPOSITOR_TRY_VERIFY(surface());

    window.destroy();
    QCOMPOSITOR_TRY_VERIFY(!surface());
}

void tst_WaylandClient::activeWindowFollowsKeyboardFocus()
{
    TestWindow window;
    window.show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    QCOMPOSITOR_TRY_VERIFY(window.isExposed());

    QCOMPARE(window.focusInEventCount, 0);
    exec([&] {
        keyboard()->sendEnter(s);
    });
    QTRY_COMPARE(window.focusInEventCount, 1);
    QCOMPARE(QGuiApplication::focusWindow(), &window);

    QCOMPARE(window.focusOutEventCount, 0);
    exec([&] {
        keyboard()->sendLeave(s); // or implement setFocus in Keyboard
    });
    QTRY_COMPARE(window.focusOutEventCount, 1);
    QCOMPARE(QGuiApplication::focusWindow(), static_cast<QWindow *>(nullptr));
}

void tst_WaylandClient::events()
{
    TestWindow window;
    window.show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    QCOMPOSITOR_TRY_VERIFY(window.isExposed());

    QCOMPARE(window.focusInEventCount, 0);

    exec([&] {
        keyboard()->sendEnter(s);
    });
    QTRY_COMPARE(window.focusInEventCount, 1);
    QCOMPARE(QGuiApplication::focusWindow(), &window);

    // See also https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
    // wl_keyboard::keymap_format
    // keymap_format { no_keymap, xkb_v1 }
    // Argument Value Description
    // no_keymap 0 no keymap; client must understand how to interpret the raw keycode
    // xkb_v1 1 libxkbcommon compatible; to determine the xkb keycode, clients must add 8 to the key event keycode
    uint keyCode = 80; // arbitrarily chosen
    QCOMPARE(window.keyPressEventCount, 0);
    exec([&] {
        keyboard()->sendKey(client(), keyCode - 8, Keyboard::key_state_pressed); // related with native scan code
    });
    QTRY_COMPARE(window.keyPressEventCount, 1);
    QCOMPARE(window.keyCode, keyCode);

    QCOMPARE(window.keyReleaseEventCount, 0);
    exec([&] {
        keyboard()->sendKey(client(), keyCode - 8, Keyboard::key_state_released); // related with native scan code
    });
    QTRY_COMPARE(window.keyReleaseEventCount, 1);
    QCOMPARE(window.keyCode, keyCode);

    const int touchId = 0;
    exec([&] {
        touch()->sendDown(s, window.frameOffset() + QPoint(10, 10), touchId);
    });
    // Note: wl_touch.frame should not be the last event in a test until QTBUG-66563 is fixed.
    // See also: QTBUG-66537
    exec([&] {
        touch()->sendFrame(client());
    });
    QTRY_COMPARE(window.touchEventCount, 1);

    exec([&] {
        touch()->sendUp(client(), touchId);
        touch()->sendFrame(client());
    });
    QTRY_COMPARE(window.touchEventCount, 2);

    QPoint mousePressPos(16, 16);
    QCOMPARE(window.mousePressEventCount, 0);
    exec([&] {
        pointer()->sendEnter(s, window.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendMotion(client(), window.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        pointer()->sendFrame(client());
    });
    QTRY_COMPARE(window.mousePressEventCount, 1);
    QTRY_COMPARE(window.mousePressPos, mousePressPos);

    QCOMPARE(window.mouseReleaseEventCount, 0);
    exec([&] {
        pointer()->sendButton(client(), BTN_LEFT, Pointer::button_state_released);
        pointer()->sendFrame(client());
    });
    QTRY_COMPARE(window.mouseReleaseEventCount, 1);
}

void tst_WaylandClient::backingStore()
{
    TestWindow window;
    window.show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    const QColor color = Qt::magenta; // see expose event in TestWindow

    QTRY_COMPARE(s->m_image.size(), window.frameGeometry().size());
    QTRY_COMPARE(s->m_image.pixel(window.frameMargins().left(), window.frameMargins().top()), color.rgba());

    window.hide();

    // hiding the window should destroy the surface
    QCOMPOSITOR_TRY_VERIFY(!surface());
}

class DndWindow : public QWindow
{
    Q_OBJECT

public:
    DndWindow(QWindow *parent = nullptr)
        : QWindow(parent)
    {
        QImage cursorImage(64,64,QImage::Format_ARGB32);
        cursorImage.fill(Qt::blue);
        m_dragIcon = QPixmap::fromImage(cursorImage);
    }
    ~DndWindow() override{}
    QPoint frameOffset() const { return QPoint(frameMargins().left(), frameMargins().top()); }
    bool dragStarted = false;

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        Q_UNUSED(event);
        if (dragStarted)
            return;
        dragStarted = true;

        QByteArray dataBytes;
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-dnditemdata", dataBytes);
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(m_dragIcon);
        drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    }
private:
    QPixmap m_dragIcon;
};

class DNDTest : public QObject
{
    Q_OBJECT

public:
    DNDTest(QObject *parent = nullptr)
        : QObject(parent) {}

    Surface *m_surface = nullptr;
    TestCompositor *m_compositor = nullptr;
    QPoint m_frameOffset;

public slots:
    void finishMouseDrag();
    void touchDrag();
};

void DNDTest::finishMouseDrag()
{
    m_compositor->exec([&] {
        m_compositor->dataDevice()->sendDrop(m_surface);
        m_compositor->dataDevice()->sendLeave(m_surface);
    });
}

void DNDTest::touchDrag()
{
    m_compositor->exec([&] {
        m_compositor->dataDevice()->sendDataOffer(m_surface->resource()->client());
        m_compositor->dataDevice()->sendEnter(m_surface, m_frameOffset + QPoint(20, 20));
        m_compositor->dataDevice()->sendMotion(m_surface, m_frameOffset + QPoint(21, 21));
        m_compositor->dataDevice()->sendDrop(m_surface);
        m_compositor->dataDevice()->sendLeave(m_surface);
    });
}

void tst_WaylandClient::touchDrag()
{
    DndWindow window;
    window.show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    DNDTest test;
    test.m_surface = s;
    test.m_compositor = this;
    test.m_frameOffset = window.frameOffset();

    exec([&] {
        QObject::connect(dataDevice(), &DataDevice::dragStarted,
                         &test, &DNDTest::touchDrag);
    });

    exec([&] {
        keyboard()->sendEnter(s);
    });
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    const int touchId = 0;
    exec([&] {
        touch()->sendDown(s, window.frameOffset() + QPoint(10, 10), touchId);
        touch()->sendFrame(client());
        touch()->sendMotion(client(), window.frameOffset() + QPoint(20, 20), touchId);
        touch()->sendFrame(client());
    });

    QTRY_VERIFY(window.dragStarted);
}

void tst_WaylandClient::mouseDrag()
{
    DndWindow window;
    window.show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    DNDTest test;
    test.m_surface = s;
    test.m_compositor = this;

    exec([&] {
        QObject::connect(dataDevice(), &DataDevice::dragStarted,
                         &test, &DNDTest::finishMouseDrag);
    });

    exec([&] {
        keyboard()->sendEnter(s);
    });
    QTRY_COMPARE(QGuiApplication::focusWindow(), &window);

    QPoint mousePressPos(16, 16);
    exec([&] {
        pointer()->sendEnter(s, window.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendMotion(client(), window.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        pointer()->sendFrame(client());

        dataDevice()->sendDataOffer(s->resource()->client());
        dataDevice()->sendEnter(s, window.frameOffset() + QPoint(20, 20));
        dataDevice()->sendMotion(s, window.frameOffset() + QPoint(21, 21));
    });

    QTRY_VERIFY(window.dragStarted);
}

void tst_WaylandClient::dontCrashOnMultipleCommits()
{
    auto window = new TestWindow();
    window->show();

    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    const QRect rect(QPoint(), window->size());

    window->backingStore->flush(rect);
    window->backingStore->flush(rect);
    window->backingStore->flush(rect);

    QCOMPOSITOR_TRY_VERIFY(surface());

    delete window;
    QCOMPOSITOR_TRY_VERIFY(!surface());
}

void tst_WaylandClient::hiddenTransientParent()
{
    QWindow parent;
    QWindow transient;

    transient.setTransientParent(&parent);

    parent.show();
    QCOMPOSITOR_TRY_VERIFY(surface());

    parent.hide();
    QCOMPOSITOR_TRY_VERIFY(!surface());

    transient.show();
    QCOMPOSITOR_TRY_VERIFY(surface());
}
void tst_WaylandClient::hiddenPopupParent()
{
    TestWindow toplevel;
    toplevel.show();

    // wl_shell relies on a mouse event in order to send a serial and seat
    // with the set_popup request.
    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });
    QCOMPOSITOR_TRY_VERIFY(toplevel.isExposed());

    QPoint mousePressPos(16, 16);
    QCOMPARE(toplevel.mousePressEventCount, 0);
    exec([&] {
        pointer()->sendEnter(s, toplevel.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendMotion(client(), toplevel.frameOffset() + mousePressPos);
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_LEFT, Pointer::button_state_pressed);
        pointer()->sendFrame(client());
    });
    QTRY_COMPARE(toplevel.mousePressEventCount, 1);

    QWindow popup;
    popup.setTransientParent(&toplevel);
    popup.setFlag(Qt::Popup, true);

    toplevel.hide();
    QCOMPOSITOR_TRY_VERIFY(!surface());

    popup.show();
    QCOMPOSITOR_TRY_VERIFY(surface());
}

#if QT_CONFIG(opengl)
void tst_WaylandClient::glWindow()
{
    QSKIP("Skipping GL tests, as not supported by all CI systems: See https://bugreports.qt.io/browse/QTBUG-65802");

    QScopedPointer<TestGlWindow> testWindow(new TestGlWindow);
    testWindow->show();
    Surface *s = nullptr;
    QCOMPOSITOR_TRY_VERIFY(s = surface());
    exec([&] {
        sendShellSurfaceConfigure(s);
    });

    QTRY_COMPARE(testWindow->paintGLCalled, 1);

    //QTBUG-63411
    QMetaObject::invokeMethod(testWindow.data(), "hideShow", Qt::QueuedConnection);
    testWindow->requestUpdate();
    QTRY_COMPARE(testWindow->paintGLCalled, 2);

    testWindow->requestUpdate();
    QTRY_COMPARE(testWindow->paintGLCalled, 3);

    //confirm we don't crash when we delete an already hidden GL window
    //QTBUG-65553
    testWindow->setVisible(false);
    QCOMPOSITOR_TRY_VERIFY(!surface());
}
#endif // QT_CONFIG(opengl)

void tst_WaylandClient::longWindowTitle()
{
    // See QTBUG-68715
    QWindow window;
    QString absurdlyLongTitle(10000, QLatin1Char('z'));
    window.setTitle(absurdlyLongTitle);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(surface());
}

void tst_WaylandClient::longWindowTitleWithUtf16Characters()
{
    QWindow window;
    QString absurdlyLongTitle = QString("三").repeated(10000);
    Q_ASSERT(absurdlyLongTitle.size() == 10000); // just making sure the test isn't broken
    window.setTitle(absurdlyLongTitle);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(surface());
}

int main(int argc, char **argv)
{
    QTemporaryDir tmpRuntimeDir;
    qputenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit());
    qputenv("QT_QPA_PLATFORM", "wayland"); // force QGuiApplication to use wayland plugin
    QString shell = QString::fromLocal8Bit(qgetenv("QT_WAYLAND_SHELL_INTEGRATION"));
    if (shell.isEmpty())
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "wl-shell");

    tst_WaylandClient tc;
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include <tst_client.moc>

