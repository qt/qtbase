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
#include <QtGui/qtestsupport_gui.h>

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
Q_WIDGETS_EXPORT Q_REQUIRED_RESULT bool QTest::qWaitForWindowActive(QWidget *widget, int timeout)
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
Q_WIDGETS_EXPORT Q_REQUIRED_RESULT bool QTest::qWaitForWindowExposed(QWidget *widget, int timeout)
{
    if (QWindow *window = widget->window()->windowHandle())
        return QTest::qWaitForWindowExposed(window, timeout);
    return false;
}

/*! \fn bool QTest::qWaitForWindowShown(QWidget *widget, int timeout)
    \since 5.0
    \deprecated

    Use qWaitForWindowExposed() instead.

    Waits for \a timeout milliseconds or until the \a widget's window is exposed.
    Returns \c true if \c widget's window is exposed within \a timeout milliseconds, otherwise returns \c false.

    This function does the same as qWaitForWindowExposed().

    Example:

    \code
        QWidget widget;
        widget.show();
        QTest::qWaitForWindowShown(&widget);
    \endcode

    \sa qWaitForWindowActive(), qWaitForWindowExposed()
*/

QT_END_NAMESPACE
