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

template <typename Predicate>
static bool qWaitForWidgetWindow(QWidget *w, Predicate predicate, QDeadlineTimer timeout)
{
    if (!w->window()->windowHandle())
        return false;

    return QTest::qWaitFor([&, wp = QPointer(w)]() {
        if (QWidget *widget = wp.data(); !widget)
            return false;
        else if (QWindow *window = widget->window()->windowHandle())
            return predicate(window);
        return false;
    }, timeout);
}

/*!
    \since 5.0
    \overload

    The \a timeout is in milliseconds.
*/
bool QTest::qWaitForWindowActive(QWidget *widget, int timeout)
{
    return qWaitForWindowActive(widget, QDeadlineTimer{timeout, Qt::TimerType::PreciseTimer});
}

/*!
    \since 6.10

    Returns \c true if \a widget is active within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWidget::show() and rely on the widget actually being
    active (i.e. being visible and having focus) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a widget from
    becoming active.

    \note Since focus is an exclusive property, \a widget may loose its focus to another window at
    any time - even after the method has returned \c true.

    \sa qWaitForWindowExposed(), QWidget::isActiveWindow()
*/
bool QTest::qWaitForWindowActive(QWidget *widget, QDeadlineTimer timeout)
{
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation))) {
        qWarning() << "qWaitForWindowActive was called on a platform that doesn't support window"
                   << "activation. This means there is an error in the test and it should either"
                   << "check for the WindowActivation platform capability before calling"
                   << "qWaitForWindowActivate, use qWaitForWindowExposed instead, or skip the test."
                   << "Falling back to qWaitForWindowExposed.";
        return qWaitForWindowExposed(widget, timeout);
    }
    return qWaitForWidgetWindow(widget,
                                [&](QWindow *window) { return window->isActive(); },
                                timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowActive(QWidget *widget)
{
    return qWaitForWindowActive(widget, Internal::defaultTryTimeout);
}

/*!
    \since 6.7

    Returns \c true, if \a widget is the focus window within \a timeout. Otherwise returns \c false.

    The method is useful in tests that call QWidget::show() and rely on the widget
    having focus (for receiving keyboard events e.g.) before proceeding.

    \note  The method will time out and return \c false if another window prevents \a widget from
    becoming focused.

    \note Since focus is an exclusive property, \a widget may loose its focus to another window at
    any time - even after the method has returned \c true.

    \sa qWaitForWindowExposed(), qWaitForWindowActive(), QGuiApplication::focusWindow()
*/
Q_WIDGETS_EXPORT bool QTest::qWaitForWindowFocused(QWidget *widget, QDeadlineTimer timeout)
{
    return qWaitForWidgetWindow(widget,
                                [&](QWindow *window) {
                                    return qGuiApp->focusWindow() == window;
                                }, timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowFocused(QWidget *widget)
{
    return qWaitForWindowFocused(widget, Internal::defaultTryTimeout);
}

/*!
    \since 5.0
    \overload

    The \a timeout is in milliseconds.
*/
bool QTest::qWaitForWindowExposed(QWidget *widget, int timeout)
{
    return qWaitForWindowExposed(widget, std::chrono::milliseconds(timeout));
}


/*!
    \since 6.10

    Returns \c true if \a widget is exposed within \a timeout milliseconds. Otherwise returns \c false.

    The method is useful in tests that call QWidget::show() and rely on the widget actually being
    being visible before proceeding.

    \note A window mapped to screen may still not be considered exposed, if the window client area is
    not visible, e.g. because it is completely covered by other windows.
    In such cases, the method will time out and return \c false.

    \sa qWaitForWindowActive(), QWidget::isVisible(), QWindow::isExposed()
*/
bool QTest::qWaitForWindowExposed(QWidget *widget, QDeadlineTimer timeout)
{
    return qWaitForWidgetWindow(widget,
                                [&](QWindow *window) { return window->isExposed(); },
                                timeout);
}

/*!
    \since 6.10
    \overload

    This function uses the default timeout of 5 seconds.
*/
bool QTest::qWaitForWindowExposed(QWidget *widget)
{
    return qWaitForWindowExposed(widget, Internal::defaultTryTimeout);
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
