/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qguiapplication_p.h>

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
        commit();
}
QTouchEventSequence& QTouchEventSequence::press(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(window, pt));
    p.setState(QEventPoint::State::Pressed);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::move(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(window, pt));
    p.setState(QEventPoint::State::Updated);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::release(int touchId, const QPoint &pt, QWindow *window)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(window, pt));
    p.setState(QEventPoint::State::Released);
    return *this;
}
QTouchEventSequence& QTouchEventSequence::stationary(int touchId)
{
    auto &p = QMutableEventPoint::from(pointOrPreviousPoint(touchId));
    p.setState(QEventPoint::State::Stationary);
    return *this;
}

void QTouchEventSequence::commit(bool processEvents)
{
    if (points.isEmpty())
        return;
    QThread::msleep(1);
    if (targetWindow)
        qt_handleTouchEvent(targetWindow, device, points.values());
    if (processEvents)
        QCoreApplication::processEvents();
    previousPoints = points;
    points.clear();
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
