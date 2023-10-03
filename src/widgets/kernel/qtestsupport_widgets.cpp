/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include "qtestsupport_widgets.h"

#include "qwidget.h"

#include <QtGui/qwindow.h>
#include <QtCore/qtestsupport_core.h>
#include <QtCore/qthread.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtGui/private/qevent_p.h>

QT_BEGIN_NAMESPACE

/*!
    \since 5.0

    Returns \c true if \a widget is active within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWidget::show() and rely on the widget actually being
    active (i.e. being visible and having focus) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a widget from
    becoming active.

    \note Since focus is an exclusive property, \a widget may loose its focus to another window at
    any time - even after the method has returned \c true.

    \sa qWaitForWindowExposed(), QWidget::isActiveWindow()
*/
Q_WIDGETS_EXPORT bool QTest::qWaitForWindowActive(QWidget *widget, int timeout)
{
    if (QWindow *window = widget->window()->windowHandle())
        return QTest::qWaitForWindowActive(window, timeout);
    return false;
}

/*!
    \since 5.0

    Returns \c true if \a widget is exposed within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWidget::show() and rely on the widget actually being
    being visible before proceeding.

    \note A window mapped to screen may still not be considered exposed, if the window client area is
    not visible, e.g. because it is completely covered by other windows.
    In such cases, the method will time out and return \c false.

    \sa qWaitForWindowActive(), QWidget::isVisible(), QWindow::isExposed()
*/
Q_WIDGETS_EXPORT bool QTest::qWaitForWindowExposed(QWidget *widget, int timeout)
{
    if (QWindow *window = widget->window()->windowHandle())
        return QTest::qWaitForWindowExposed(window, timeout);
    return false;
}

namespace QTest {

QTouchEventWidgetSequence::~QTouchEventWidgetSequence()
{
    if (commitWhenDestroyed)
        commit();
}

QTouchEventWidgetSequence& QTouchEventWidgetSequence::press(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(widget, pt));
    p.setState(QEventPoint::State::Pressed);
    return *this;
}
QTouchEventWidgetSequence& QTouchEventWidgetSequence::move(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(widget, pt));
    p.setState(QEventPoint::State::Updated);
    return *this;
}
QTouchEventWidgetSequence& QTouchEventWidgetSequence::release(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = QMutableEventPoint::from(point(touchId));
    p.setGlobalPosition(mapToScreen(widget, pt));
    p.setState(QEventPoint::State::Released);
    return *this;
}

QTouchEventWidgetSequence& QTouchEventWidgetSequence::stationary(int touchId)
{
    auto &p = QMutableEventPoint::from(pointOrPreviousPoint(touchId));
    p.setState(QEventPoint::State::Stationary);
    return *this;
}

void QTouchEventWidgetSequence::commit(bool processEvents)
{
    if (points.isEmpty())
        return;
    QThread::msleep(1);
    if (targetWindow) {
        qt_handleTouchEvent(targetWindow, device, points.values());
    } else if (targetWidget) {
        qt_handleTouchEvent(targetWidget->windowHandle(), device, points.values());
    }
    if (processEvents)
        QCoreApplication::processEvents();
    previousPoints = points;
    points.clear();
}

QTest::QTouchEventWidgetSequence::QTouchEventWidgetSequence(QWidget *widget, QPointingDevice *aDevice, bool autoCommit)
    : QTouchEventSequence(nullptr, aDevice, autoCommit), targetWidget(widget)
{
}

QPoint QTouchEventWidgetSequence::mapToScreen(QWidget *widget, const QPoint &pt)
{
    if (widget)
        return widget->mapToGlobal(pt);
    return targetWidget ? targetWidget->mapToGlobal(pt) : pt;
}

} // namespace QTest

QT_END_NAMESPACE
