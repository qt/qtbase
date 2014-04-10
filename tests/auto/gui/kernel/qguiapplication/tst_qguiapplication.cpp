/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtGui/QCursor>
#include <qpa/qwindowsysteminterface.h>
#include <qgenericplugin.h>

#if defined(Q_OS_QNX)
#include <QOpenGLContext>
#endif

#include <QDebug>

#include "tst_qcoreapplication.h"

enum { spacing  = 50, windowSize = 200 };

class tst_QGuiApplication: public tst_QCoreApplication
{
    Q_OBJECT

private slots:
    void displayName();
    void firstWindowTitle();
    void focusObject();
    void allWindows();
    void topLevelWindows();
    void abortQuitOnShow();
    void changeFocusWindow();
    void keyboardModifiers();
    void modalWindow();
    void quitOnLastWindowClosed();
    void genericPluginsAndWindowSystemEvents();
};

void tst_QGuiApplication::displayName()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>("tst_qguiapplication") };
    QGuiApplication app(argc, argv);
    QCOMPARE(::qAppName(), QString::fromLatin1("tst_qguiapplication"));
    QCOMPARE(QGuiApplication::applicationName(), QString::fromLatin1("tst_qguiapplication"));
    QCOMPARE(QGuiApplication::applicationDisplayName(), QString::fromLatin1("tst_qguiapplication"));
    QGuiApplication::setApplicationDisplayName("The GUI Application");
    QCOMPARE(QGuiApplication::applicationDisplayName(), QString::fromLatin1("The GUI Application"));
}

void tst_QGuiApplication::firstWindowTitle()
{
    int argc = 3;
    char *argv[] = { const_cast<char*>("tst_qguiapplication"), const_cast<char*>("-qwindowtitle"), const_cast<char*>("User Title") };
    QGuiApplication app(argc, argv);
    QWindow window;
    window.setTitle("Application Title");
    window.show();
    QCOMPARE(window.title(), QString("User Title"));
}

class DummyWindow : public QWindow
{
public:
    DummyWindow() : m_focusObject(0) {}

    virtual QObject *focusObject() const
    {
        return m_focusObject;
    }

    void setFocusObject(QObject *object)
    {
        m_focusObject = object;
        emit focusObjectChanged(object);
    }

    QObject *m_focusObject;
};


void tst_QGuiApplication::focusObject()
{
    int argc = 0;
    QGuiApplication app(argc, 0);

    QObject obj1, obj2, obj3;
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    DummyWindow window1;
#if defined(Q_OS_QNX)
    window1.setSurfaceType(QSurface::OpenGLSurface);
#endif
    window1.resize(windowSize, windowSize);
    window1.setTitle(QStringLiteral("focusObject:window1"));
    window1.setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));
    DummyWindow window2;
    window2.resize(windowSize, windowSize);
    window2.setFramePosition(QPoint(screenGeometry.left() + 2 * spacing + windowSize, screenGeometry.top() + spacing));
    window2.setTitle(QStringLiteral("focusObject:window2"));

    window1.show();

#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&window1);
    QTest::qWaitForWindowExposed(&window1); // Buffer swap only succeeds with exposed window
    context.swapBuffers(&window1);
#endif

    QSignalSpy spy(&app, SIGNAL(focusObjectChanged(QObject*)));


    // verify active window focus propagates to qguiapplication
    window1.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window1));
    QCOMPARE(app.focusWindow(), &window1);

    window1.setFocusObject(&obj1);
    QCOMPARE(app.focusObject(), &obj1);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    window1.setFocusObject(&obj2);
    QCOMPARE(app.focusObject(), &obj2);
    QCOMPARE(spy.count(), 1);

    spy.clear();
    window2.setFocusObject(&obj3);
    QCOMPARE(app.focusObject(), &obj2); // not yet changed
    window2.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window2));
    QTRY_COMPARE(app.focusWindow(), &window2);
    QCOMPARE(app.focusObject(), &obj3);
    QCOMPARE(spy.count(), 1);

    // focus change on unfocused window does not show
    spy.clear();
    window1.setFocusObject(&obj1);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(app.focusObject(), &obj3);
}

void tst_QGuiApplication::allWindows()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    QWindow *window1 = new QWindow;
    QWindow *window2 = new QWindow(window1);
    QVERIFY(app.allWindows().contains(window1));
    QVERIFY(app.allWindows().contains(window2));
    QCOMPARE(app.allWindows().count(), 2);
    delete window1;
    window1 = 0;
    window2 = 0;
    QVERIFY(!app.allWindows().contains(window2));
    QVERIFY(!app.allWindows().contains(window1));
    QCOMPARE(app.allWindows().count(), 0);
}

void tst_QGuiApplication::topLevelWindows()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    QWindow *window1 = new QWindow;
    QWindow *window2 = new QWindow(window1);
    QVERIFY(app.topLevelWindows().contains(window1));
    QVERIFY(!app.topLevelWindows().contains(window2));
    QCOMPARE(app.topLevelWindows().count(), 1);
    delete window1;
    window1 = 0;
    window2 = 0;
    QVERIFY(!app.topLevelWindows().contains(window2));
    QVERIFY(!app.topLevelWindows().contains(window1));
    QCOMPARE(app.topLevelWindows().count(), 0);
}

class ShowCloseShowWindow : public QWindow
{
    Q_OBJECT
public:
    ShowCloseShowWindow(bool showAgain, QWindow *parent = 0)
      : QWindow(parent), showAgain(showAgain)
    {
        QTimer::singleShot(0, this, SLOT(doClose()));
        QTimer::singleShot(500, this, SLOT(exitApp()));
    }

private slots:
    void doClose() {
        close();
        if (showAgain)
            show();
    }

    void exitApp() {
      qApp->exit(1);
    }

private:
    bool showAgain;
};

void tst_QGuiApplication::abortQuitOnShow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    QScopedPointer<QWindow> window1(new ShowCloseShowWindow(false));
    window1->resize(windowSize, windowSize);
    window1->setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));
    window1->setTitle(QStringLiteral("abortQuitOnShow:window1"));
    window1->show();
    QCOMPARE(app.exec(), 0);

    QScopedPointer<QWindow> window2(new ShowCloseShowWindow(true));
    window2->setTitle(QStringLiteral("abortQuitOnShow:window2"));
    window2->resize(windowSize, windowSize);
    window2->setFramePosition(QPoint(screenGeometry.left() + 2 * spacing + windowSize, screenGeometry.top() + spacing));
    window2->show();
    QCOMPARE(app.exec(), 1);
}


class FocusChangeWindow: public QWindow
{
protected:
    virtual bool event(QEvent *ev)
    {
        if (ev->type() == QEvent::FocusAboutToChange)
            windowDuringFocusAboutToChange = qGuiApp->focusWindow();
        return QWindow::event(ev);
    }

    virtual void focusOutEvent(QFocusEvent *)
    {
        windowDuringFocusOut = qGuiApp->focusWindow();
    }

public:
    FocusChangeWindow() : QWindow(), windowDuringFocusAboutToChange(0), windowDuringFocusOut(0) {}

    QWindow *windowDuringFocusAboutToChange;
    QWindow *windowDuringFocusOut;
};

void tst_QGuiApplication::changeFocusWindow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    // focus is changed between FocusAboutToChange and FocusChanged
    FocusChangeWindow window1;
#if defined(Q_OS_QNX)
    window1.setSurfaceType(QSurface::OpenGLSurface);
#endif
    window1.resize(windowSize, windowSize);
    window1.setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));
    window1.setTitle(QStringLiteral("changeFocusWindow:window1"));
    window1.show();
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&window1);
    QTest::qWaitForWindowExposed(&window1); // Buffer swap only succeeds with exposed window
    context.swapBuffers(&window1);
#endif
    FocusChangeWindow window2;
#if defined(Q_OS_QNX)
    window2.setSurfaceType(QSurface::OpenGLSurface);
#endif
    window2.resize(windowSize, windowSize);
    window2.setFramePosition(QPoint(screenGeometry.left() + 2 * spacing + windowSize, screenGeometry.top() + spacing));
    window2.setTitle(QStringLiteral("changeFocusWindow:window2"));
    window2.show();
#if defined(Q_OS_QNX) // We either need to create a eglSurface or a create a backing store
                      // and then post the window in order for screen to show the window
    context.makeCurrent(&window2);
    QTest::qWaitForWindowExposed(&window2); // Buffer swap only succeeds with exposed window
    context.swapBuffers(&window2);
#endif
    QVERIFY(QTest::qWaitForWindowExposed(&window1));
    QVERIFY(QTest::qWaitForWindowExposed(&window2));
    window1.requestActivate();
    QTRY_COMPARE(app.focusWindow(), &window1);

    window2.requestActivate();
    QTRY_COMPARE(app.focusWindow(), &window2);
    QCOMPARE(window1.windowDuringFocusAboutToChange, &window1);
    QCOMPARE(window1.windowDuringFocusOut, &window2);
}

void tst_QGuiApplication::keyboardModifiers()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    QScopedPointer<QWindow> window(new QWindow);
    window->resize(windowSize, windowSize);
    window->setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));
    window->setTitle(QStringLiteral("keyboardModifiers"));

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    // mouse events
    QPoint center = window->geometry().center();
    QTest::mouseEvent(QTest::MousePress, window.data(), Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::mouseEvent(QTest::MouseRelease, window.data(), Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::mouseEvent(QTest::MousePress, window.data(), Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::mouseEvent(QTest::MouseRelease, window.data(), Qt::RightButton, Qt::ControlModifier, center);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // shortcut events
    QWindowSystemInterface::tryHandleShortcutEvent(window.data(), Qt::Key_5, Qt::MetaModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::MetaModifier);
    QWindowSystemInterface::tryHandleShortcutEvent(window.data(), Qt::Key_Period, Qt::NoModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QWindowSystemInterface::tryHandleShortcutEvent(window.data(), Qt::Key_0, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // key events
    QTest::keyEvent(QTest::Press, window.data(), Qt::Key_C);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::keyEvent(QTest::Release, window.data(), Qt::Key_C);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    QTest::keyEvent(QTest::Press, window.data(), Qt::Key_U, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::keyEvent(QTest::Release, window.data(), Qt::Key_U, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    QTest::keyEvent(QTest::Press, window.data(), Qt::Key_T);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QTest::keyEvent(QTest::Release, window.data(), Qt::Key_T);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);

    QTest::keyEvent(QTest::Press, window.data(), Qt::Key_E, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);
    QTest::keyEvent(QTest::Release, window.data(), Qt::Key_E, Qt::ControlModifier);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // wheel events
    QPoint global = window->mapToGlobal(center);
    QPoint delta(0, 1);
    QWindowSystemInterface::handleWheelEvent(window.data(), center, global, delta, delta, Qt::NoModifier);
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    QWindowSystemInterface::handleWheelEvent(window.data(), center, global, delta, delta, Qt::AltModifier);
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::AltModifier);
    QWindowSystemInterface::handleWheelEvent(window.data(), center, global, delta, delta, Qt::ControlModifier);
    QWindowSystemInterface::sendWindowSystemEvents(QEventLoop::AllEvents);
    QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::ControlModifier);

    // touch events
    QList<const QTouchDevice *> touchDevices = QTouchDevice::devices();
    if (!touchDevices.isEmpty()) {
        QTouchDevice *touchDevice = const_cast<QTouchDevice *>(touchDevices.first());
        QTest::touchEvent(window.data(), touchDevice).press(1, center).release(1, center);
        QCOMPARE(QGuiApplication::keyboardModifiers(), Qt::NoModifier);
    }

    window->close();
}

class BlockableWindow : public QWindow
{
    Q_OBJECT
public:
    int blocked;
    int leaves;
    int enters;

    inline explicit BlockableWindow(QWindow *parent = 0)
        : QWindow(parent), blocked(false), leaves(0), enters(0) {}

    bool event(QEvent *e)
    {
        switch (e->type()) {
        case QEvent::WindowBlocked:
            ++blocked;
            break;
        case QEvent::WindowUnblocked:
            --blocked;
            break;
        case QEvent::Leave:
            leaves++;
            break;
        case QEvent::Enter:
            enters++;
            break;
        default:
            break;
        }
        return QWindow::event(e);
    }

    void resetCounts()
    {
        leaves = 0;
        enters = 0;
    }
};

void tst_QGuiApplication::modalWindow()
{
    int argc = 0;
    QGuiApplication app(argc, 0);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

    int x = screenGeometry.left() + spacing;
    int y = screenGeometry.top() + spacing;

    QScopedPointer<BlockableWindow> window1(new BlockableWindow);
    window1->setTitle(QStringLiteral("window1"));
    window1->resize(windowSize, windowSize);
    window1->setFramePosition(QPoint(x, y));
    BlockableWindow *childWindow1 = new BlockableWindow(window1.data());
    childWindow1->resize(windowSize / 2, windowSize / 2);
    x += spacing + windowSize;

    QScopedPointer<BlockableWindow> window2(new BlockableWindow);
    window2->setTitle(QStringLiteral("window2"));
    window2->setFlags(window2->flags() & Qt::Tool); // QTBUG-32433, don't be fooled by unusual window flags.
    window2->resize(windowSize, windowSize);
    window2->setFramePosition(QPoint(x, y));
    x += spacing + windowSize;

    QScopedPointer<BlockableWindow> windowModalWindow1(new BlockableWindow);
    windowModalWindow1->setTitle(QStringLiteral("windowModalWindow1"));
    windowModalWindow1->setTransientParent(window1.data());
    windowModalWindow1->setModality(Qt::WindowModal);
    windowModalWindow1->resize(windowSize, windowSize);
    windowModalWindow1->setFramePosition(QPoint(x, y));
    x += spacing + windowSize;

    QScopedPointer<BlockableWindow> windowModalWindow2(new BlockableWindow);
    windowModalWindow2->setTitle(QStringLiteral("windowModalWindow2"));
    windowModalWindow2->setTransientParent(windowModalWindow1.data());
    windowModalWindow2->setModality(Qt::WindowModal);
    windowModalWindow2->resize(windowSize, windowSize);
    windowModalWindow2->setFramePosition(QPoint(x, y));
    x = screenGeometry.left() + spacing;
    y += spacing + windowSize;

    QScopedPointer<BlockableWindow> applicationModalWindow1(new BlockableWindow);
    applicationModalWindow1->setTitle(QStringLiteral("applicationModalWindow1"));
    applicationModalWindow1->setModality(Qt::ApplicationModal);
    applicationModalWindow1->resize(windowSize, windowSize);
    applicationModalWindow1->setFramePosition(QPoint(x, y));

#ifndef QT_NO_CURSOR // Get the mouse cursor out of the way since we are manually sending enter/leave.
    QCursor::setPos(QPoint(x + 2 * spacing + windowSize, y));
#endif

    // show the 2 windows, nothing is blocked
    window1->show();
    window2->show();
    QVERIFY(QTest::qWaitForWindowExposed(window1.data()));
    QVERIFY(QTest::qWaitForWindowExposed(window2.data()));
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(childWindow1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // enter mouse in window1
    QWindowSystemInterface::handleEnterEvent(window1.data());
    QGuiApplication::processEvents();
    QCOMPARE(window1->enters, 1);
    QCOMPARE(window1->leaves, 0);

    // show applicationModalWindow1, everything is blocked
    applicationModalWindow1->show();
    QCOMPARE(app.modalWindow(), applicationModalWindow1.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(childWindow1->blocked, 1); // QTBUG-32242, blocked status needs to be set on children as well.
    QCOMPARE(window2->blocked, 1);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 1);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // opening modal causes leave for previously entered window, but not others
    QGuiApplication::processEvents();
    QCOMPARE(window1->enters, 1);
    QCOMPARE(window1->leaves, 1);
    QCOMPARE(window2->enters, 0);
    QCOMPARE(window2->leaves, 0);
    QCOMPARE(applicationModalWindow1->enters, 0);
    QCOMPARE(applicationModalWindow1->leaves, 0);
    window1->resetCounts();

    // Try entering/leaving blocked window2 - no events should reach it
    QWindowSystemInterface::handleEnterEvent(window2.data());
    QGuiApplication::processEvents();
    QWindowSystemInterface::handleLeaveEvent(window2.data());
    QGuiApplication::processEvents();
    QCOMPARE(window2->enters, 0);
    QCOMPARE(window2->leaves, 0);

    // everything is unblocked when applicationModalWindow1 is hidden
    applicationModalWindow1->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(childWindow1->blocked, 0); // QTBUG-32242, blocked status needs to be set on children as well.
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // Enter window2 - should not be blocked
    QWindowSystemInterface::handleEnterEvent(window2.data());
    QGuiApplication::processEvents();
    QCOMPARE(window2->enters, 1);
    QCOMPARE(window2->leaves, 0);

    // show the windowModalWindow1, only window1 is blocked
    windowModalWindow1->show();
    QCOMPARE(app.modalWindow(), windowModalWindow1.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // opening window modal window doesn't cause leave for unblocked window
    QGuiApplication::processEvents();
    QCOMPARE(window1->enters, 0);
    QCOMPARE(window1->leaves, 0);
    QCOMPARE(window2->enters, 1);
    QCOMPARE(window2->leaves, 0);
    QCOMPARE(windowModalWindow1->enters, 0);
    QCOMPARE(windowModalWindow1->leaves, 0);

    // show the windowModalWindow2, windowModalWindow1 is blocked as well
    windowModalWindow2->show();
    QCOMPARE(app.modalWindow(), windowModalWindow2.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow1, nothing is unblocked
    windowModalWindow1->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow2.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow2, windowModalWindow1 and window1 are unblocked
    windowModalWindow2->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show windowModalWindow1 again, window1 is blocked
    windowModalWindow1->show();
    QCOMPARE(app.modalWindow(), windowModalWindow1.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show windowModalWindow2 again, windowModalWindow1 is also blocked
    windowModalWindow2->show();
    QCOMPARE(app.modalWindow(), windowModalWindow2.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // show applicationModalWindow1, everything is blocked
    applicationModalWindow1->show();
    QCOMPARE(app.modalWindow(), applicationModalWindow1.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 1);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 1);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // window2 gets finally the leave
    QGuiApplication::processEvents();
    QCOMPARE(window1->enters, 0);
    QCOMPARE(window1->leaves, 0);
    QCOMPARE(window2->enters, 1);
    QCOMPARE(window2->leaves, 1);
    QCOMPARE(windowModalWindow1->enters, 0);
    QCOMPARE(windowModalWindow1->leaves, 0);
    QCOMPARE(applicationModalWindow1->enters, 0);
    QCOMPARE(applicationModalWindow1->leaves, 0);

    // hide applicationModalWindow1, windowModalWindow1 and window1 are blocked
    applicationModalWindow1->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow2.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 1);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow2, window1 is blocked
    windowModalWindow2->hide();
    QCOMPARE(app.modalWindow(), windowModalWindow1.data());
    QCOMPARE(window1->blocked, 1);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    // hide windowModalWindow1, everything is unblocked
    windowModalWindow1->hide();
    QCOMPARE(app.modalWindow(), static_cast<QWindow *>(0));
    QCOMPARE(window1->blocked, 0);
    QCOMPARE(window2->blocked, 0);
    QCOMPARE(windowModalWindow1->blocked, 0);
    QCOMPARE(windowModalWindow2->blocked, 0);
    QCOMPARE(applicationModalWindow1->blocked, 0);

    window2->hide();
    window1->hide();
}

void tst_QGuiApplication::quitOnLastWindowClosed()
{
    {
        int argc = 0;
        QGuiApplication app(argc, 0);
        const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        QWindow mainWindow;
        mainWindow.setTitle(QStringLiteral("quitOnLastWindowClosedMainWindow"));
        mainWindow.resize(windowSize, windowSize);
        mainWindow.setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));

        QWindow dialog;
        dialog.setTransientParent(&mainWindow);
        dialog.setTitle(QStringLiteral("quitOnLastWindowClosedDialog"));
        dialog.resize(windowSize, windowSize);
        dialog.setFramePosition(QPoint(screenGeometry.left() + 2 * spacing + windowSize, screenGeometry.top() + spacing));

        QVERIFY(app.quitOnLastWindowClosed());

        mainWindow.show();
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));

        timer.start();
        QTimer::singleShot(1000, &mainWindow, SLOT(close())); // This should quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));        // This makes sure we quit even if it didn't

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() < 15);      // Should be around 10 if closing caused the quit
    }
    {
        int argc = 0;
        QGuiApplication app(argc, 0);
        const QRect screenGeometry = QGuiApplication::primaryScreen()->availableVirtualGeometry();

        QTimer timer;
        timer.setInterval(100);

        QSignalSpy spy(&app, SIGNAL(aboutToQuit()));
        QSignalSpy spy2(&timer, SIGNAL(timeout()));

        QWindow mainWindow;
        mainWindow.setTitle(QStringLiteral("quitOnLastWindowClosedMainWindow"));
        mainWindow.resize(windowSize, windowSize);
        mainWindow.setFramePosition(QPoint(screenGeometry.left() + spacing, screenGeometry.top() + spacing));

        QWindow dialog;
        dialog.setTitle(QStringLiteral("quitOnLastWindowClosedDialog"));
        dialog.resize(windowSize, windowSize);
        dialog.setFramePosition(QPoint(screenGeometry.left() + 2 * spacing + windowSize, screenGeometry.top() + spacing));

        QVERIFY(!dialog.transientParent());
        QVERIFY(app.quitOnLastWindowClosed());

        mainWindow.show();
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));

        timer.start();
        QTimer::singleShot(1000, &mainWindow, SLOT(close())); // This should not quit the application
        QTimer::singleShot(2000, &app, SLOT(quit()));

        app.exec();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy2.count() > 15);      // Should be around 20 if closing did not cause the quit
    }
}

static Qt::ScreenOrientation testOrientationToSend = Qt::PrimaryOrientation;

class TestPlugin : public QObject
{
    Q_OBJECT
public:
    TestPlugin()
    {
        QScreen* screen = QGuiApplication::primaryScreen();
        // Make sure the orientation we want to send doesn't get filtered out.
        screen->setOrientationUpdateMask(screen->orientationUpdateMask() | testOrientationToSend);
        QWindowSystemInterface::handleScreenOrientationChange(screen, testOrientationToSend);
    }
};

class TestPluginFactory : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "testplugin.json")
public:
    QObject* create(const QString &key, const QString &)
    {
        if (key == "testplugin")
            return new TestPlugin;
        return 0;
    }
};

class TestEventReceiver : public QObject
{
    Q_OBJECT
public:
    int customEvents;

    TestEventReceiver()
        : customEvents(0)
    {}

    virtual void customEvent(QEvent *)
    {
        customEvents++;
    }
};

#include "tst_qguiapplication.moc"

void tst_QGuiApplication::genericPluginsAndWindowSystemEvents()
{
    testOrientationToSend = Qt::InvertedLandscapeOrientation;

    TestEventReceiver testReceiver;
    QCoreApplication::postEvent(&testReceiver, new QEvent(QEvent::User));
    QCOMPARE(testReceiver.customEvents, 0);

    QStaticPlugin testPluginInfo;
    testPluginInfo.instance = qt_plugin_instance;
    testPluginInfo.rawMetaData = qt_plugin_query_metadata;
    qRegisterStaticPluginFunction(testPluginInfo);
    int argc = 3;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()), const_cast<char*>("-plugin"), const_cast<char*>("testplugin") };
    QGuiApplication app(argc, argv);

    QVERIFY(QGuiApplication::primaryScreen());
    QVERIFY(QGuiApplication::primaryScreen()->orientation() == testOrientationToSend);

    QCOMPARE(testReceiver.customEvents, 0);
    QCoreApplication::sendPostedEvents(&testReceiver);
    QCOMPARE(testReceiver.customEvents, 1);
}

QTEST_APPLESS_MAIN(tst_QGuiApplication)
