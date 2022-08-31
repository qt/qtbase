// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qguiapplication_p.h>
#include <private/qeventpoint_p.h>

#include <qpa/qplatformintegration.h>

#include "qtestsupport_gui.h"
#include "qwindow.h"

#include <QtCore/qtestsupport_core.h>
#include <QtCore/qthread.h>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \since 5.0

    Waits for \a timeout milliseconds or until the \a window is active.

    Returns \c true if \c window is active within \a timeout milliseconds, otherwise returns \c false.

    \sa qWaitForWindowExposed(), QWindow::isActive()
*/
Q_GUI_EXPORT bool QTest::qWaitForWindowActive(QWindow *window, int timeout)
{
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))) {
        qWarning() << "qWaitForWindowActive was called on a platform that doesn't support window"
                   << "activation. This means there is an error in the test and it should either"
                   << "check for the WindowActivation platform capability before calling"
                   << "qWaitForWindowActivate, use qWaitForWindowExposed instead, or skip the test."
                   << "Falling back to qWaitForWindowExposed.";
        return qWaitForWindowExposed(window, timeout);
    }
    return QTest::qWaitFor([&]() { return window->isActive(); }, timeout);
}

/*!
    \since 5.0

    Waits for \a timeout milliseconds or until the \a window is exposed.
    Returns \c true if \c window is exposed within \a timeout milliseconds, otherwise returns \c false.

    This is mainly useful for asynchronous systems like X11, where a window will be mapped to screen some
    time after being asked to show itself on the screen.

    Note that a window that is mapped to screen may still not be considered exposed if the window client
    area is completely covered by other windows, or if the window is otherwise not visible. This function
    will then time out when waiting for such a window.

    \sa qWaitForWindowActive(), QWindow::isExposed()
*/
Q_GUI_EXPORT bool QTest::qWaitForWindowExposed(QWindow *window, int timeout)
{
    return QTest::qWaitFor([&]() { return window->isExposed(); }, timeout);
}

namespace QTest {

QTouchEventSequence::~QTouchEventSequence()
{
    if (commitWhenDestroyed)
        commit();
}
QTouchEventSequence& QTouchEventSequence::press(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Pressed);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::move(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Updated);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::release(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(window, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Released);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::stationary(int touchId)
{
    auto &p = pointOrPreviousPoint(touchId);
    QMutableEventPoint::setState(p, QEventPoint::State::Stationary);
    return *this;
}

bool QTouchEventSequence::commit(bool processEvents)
{
    if (points.isEmpty())
        return false;
    QThread::msleep(1);
    bool ret = false;
    if (targetWindow)
        ret = qt_handleTouchEventv2(targetWindow, device, points.values());
    if (processEvents)
        QCoreApplication::processEvents();
    previousPoints = points;
    points.clear();
    return ret;
}

QTouchEventSequence::QTouchEventSequence(QWindow *window, QPointingDevice *aDevice, bool autoCommit)
    : targetWindow(window), device(aDevice), commitWhenDestroyed(autoCommit)
{
}

QPoint QTouchEventSequence::mapToScreen(QWindow *window, const QPoint &pt)
{
    if (window)
        return window->mapToGlobal(pt);
    return targetWindow ? targetWindow->mapToGlobal(pt) : pt;
}

QEventPoint &QTouchEventSequence::point(int touchId)
{
    if (!points.contains(touchId))
        points[touchId] = QEventPoint(touchId);
    return points[touchId];
}

QEventPoint &QTouchEventSequence::pointOrPreviousPoint(int touchId)
{
    if (!points.contains(touchId)) {
        if (previousPoints.contains(touchId))
            points[touchId] = previousPoints.value(touchId);
        else
            points[touchId] = QEventPoint(touchId);
    }
    return points[touchId];
}

} // namespace QTest

QT_END_NAMESPACE
