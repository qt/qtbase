/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperwindow.h"

#include "qpeppercompositor.h"
#include "qpepperglcontext.h"
#include "qpepperintegration.h"
#include "qpeppermodule.h"

#include <QtCore/QDebug>
#include <QtGui/QSurface>
#include <private/qwindow_p.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_WINDOW, "qt.platform.pepper.window")

QPepperWindow::QPepperWindow(QWindow *window)
    : QPlatformWindow(window)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "Create QPepperWindow for" << window;

    m_compositor = 0;

    // Set window state (fullscreen windows resize here)
    setWindowState(window->windowState());

    if (m_compositor)
        m_compositor->addRasterWindow(this->window());
}

QPepperWindow::~QPepperWindow()
{
    if (m_compositor)
        m_compositor->removeWindow(this->window());
}

void QPepperWindow::setGeometry(const QRect &rect)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setGeometry" << rect;

    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);
    QPepperInstancePrivate::get()->scheduleWindowSystemEventsFlush();
}

void QPepperWindow::setVisible(bool visible)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setVisible" << visible;

    QWindowSystemInterface::handleExposeEvent(window(), visible ? QRect(QPoint(), geometry().size())
                                                                : QRect());
    QPepperInstancePrivate::get()->scheduleWindowSystemEventsFlush();

    if (m_compositor)
        m_compositor->setVisible(this->window(), visible);
}

void QPepperWindow::setWindowState(Qt::WindowState state)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setWindowState" << state;

    switch (state) {
    case Qt::WindowMaximized:
    case Qt::WindowFullScreen: {
        // Set full-scree window geometry
        QRect targetWindowGeometry = QRect(QPoint(), screen()->geometry().size());
        QPlatformWindow::setGeometry(targetWindowGeometry);
        QWindowSystemInterface::handleGeometryChange(window(), targetWindowGeometry);
        QWindowSystemInterface::handleExposeEvent(window(), targetWindowGeometry);
        QPepperInstancePrivate::get()->scheduleWindowSystemEventsFlush();
        break;
    }
    default:
        break;
    }
}

WId QPepperWindow::winId() const { return WId(this); }

void QPepperWindow::setParent(const QPlatformWindow *parent)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "QPepperWindow::setParent" << parent;

    if (m_compositor)
        m_compositor->setParent(this->window(), parent->window());
}

void QPepperWindow::raise()
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "raise";

    if (m_compositor)
        m_compositor->raise(this->window());
}

void QPepperWindow::lower()
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "lower";

    if (m_compositor)
        m_compositor->lower(this->window());
}

qreal QPepperWindow::devicePixelRatio() const
{
    return QPepperInstancePrivate::get()->devicePixelRatio();
}

bool QPepperWindow::setKeyboardGrabEnabled(bool grab)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setKeyboardGrabEnabled" << grab;

    Q_UNUSED(grab);
    return false;
}

bool QPepperWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setMouseGrabEnabled" << grab;

    Q_UNUSED(grab);
    return false;
}

#ifdef Q_OS_NACL_EMSCRIPTEN
void QPepperWindow::requestUpdate()
{
    // Work around missing timer event(?) when using the default
    // QPlatformWindow requestUpdate() implementation.
    QWindowPrivate *wp = (QWindowPrivate *) QObjectPrivate::get(window());
    wp->deliverUpdateRequest();
}
#endif

QT_END_NAMESPACE
