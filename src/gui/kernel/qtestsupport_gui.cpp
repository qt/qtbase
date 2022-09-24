/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <private/qguiapplication_p.h>

#include <qpa/qplatformintegration.h>

#include "qtestsupport_gui.h"
#include "qwindow.h"

#include <QtCore/qtestsupport_core.h>
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

QT_END_NAMESPACE
