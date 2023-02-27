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

    Returns \c true, if \a window is active within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWindow::show() and rely on the window actually being
    active (i.e. being visible and having focus) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a window from
    becoming active.

    \note Since focus is an exclusive property, \a window may loose its focus to another window at
    any time - even after the method has returned \c true.

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

    Returns \c true, if \a window is exposed within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWindow::show() and rely on the window actually being
    being visible before proceeding.

    \note A window mapped to screen may still not be considered exposed, if the window client area is
    not visible, e.g. because it is completely covered by other windows.
    In such cases, the method will time out and return \c false.

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
        QTouchEventSequence::commit();
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
    QThread::sleep(std::chrono::milliseconds{1});
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
