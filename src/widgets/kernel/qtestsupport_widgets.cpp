// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtestsupport_widgets.h"

#include "qwidget.h"

#include <QtGui/qwindow.h>
#include <QtCore/qtestsupport_core.h>
#include <QtCore/qthread.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/private/qeventpoint_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

template <typename FunctorWindowGetter, typename FunctorPredicate>
static bool qWaitForWidgetWindow(FunctorWindowGetter windowGetter, FunctorPredicate predicate, int timeout)
{
    if (!windowGetter())
        return false;

    return QTest::qWaitFor([&]() {
        if (QWindow *window = windowGetter())
            return predicate(window);
        return false;
    }, timeout);
}

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
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))) {
        qWarning() << "qWaitForWindowActive was called on a platform that doesn't support window"
                   << "activation. This means there is an error in the test and it should either"
                   << "check for the WindowActivation platform capability before calling"
                   << "qWaitForWindowActivate, use qWaitForWindowExposed instead, or skip the test."
                   << "Falling back to qWaitForWindowExposed.";
        return qWaitForWindowExposed(widget, timeout);
    }
    return qWaitForWidgetWindow([&]() { return widget->window()->windowHandle(); },
                                [&](QWindow *window) { return window->isActive(); },
                                timeout);
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
    return qWaitForWidgetWindow([&]() { return widget->window()->windowHandle(); },
                                [&](QWindow *window) { return window->isExposed(); },
                                timeout);
}

namespace QTest {

QTouchEventWidgetSequence::~QTouchEventWidgetSequence()
{
    if (commitWhenDestroyed)
        QTouchEventWidgetSequence::commit();
}

QTouchEventWidgetSequence& QTouchEventWidgetSequence::press(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(widget, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Pressed);
    return *this;
}
QTouchEventWidgetSequence& QTouchEventWidgetSequence::move(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(widget, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Updated);
    return *this;
}
QTouchEventWidgetSequence& QTouchEventWidgetSequence::release(int touchId, const QPoint &pt, QWidget *widget)
{
    auto &p = point(touchId);
    QMutableEventPoint::setGlobalPosition(p, mapToScreen(widget, pt));
    QMutableEventPoint::setState(p, QEventPoint::State::Released);
    return *this;
}

QTouchEventWidgetSequence& QTouchEventWidgetSequence::stationary(int touchId)
{
    auto &p = pointOrPreviousPoint(touchId);
    QMutableEventPoint::setState(p, QEventPoint::State::Stationary);
    return *this;
}

bool QTouchEventWidgetSequence::commit(bool processEvents)
{
    bool ret = false;
    if (points.isEmpty())
        return ret;
    QThread::sleep(std::chrono::milliseconds{1});
    if (targetWindow) {
        ret = qt_handleTouchEventv2(targetWindow, device, points.values());
    } else if (targetWidget) {
        ret = qt_handleTouchEventv2(targetWidget->windowHandle(), device, points.values());
    }
    if (processEvents)
        QCoreApplication::processEvents();
    previousPoints = points;
    points.clear();
    return ret;
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
