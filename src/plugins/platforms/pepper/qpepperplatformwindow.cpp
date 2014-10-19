/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpepperplatformwindow.h"
#include "qpepperglcontext.h"
#include "qpeppermodule.h"
#include "qpeppercompositor.h"
#include <QtGui/QSurface>
#include <qpa/qwindowsysteminterface.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_WINDOW, "qt.platform.pepper.window")

QPepperPlatformWindow::QPepperPlatformWindow(QWindow *window)
:QPlatformWindow(window)
,m_isVisible(false)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "Create QPepperPlatformWindow for" << window;

    m_pepperIntegration = QPepperIntegration::getPepperIntegration();

    m_compositor = 0;

    // Set window state (fullscreen windows resize here)
    setWindowState(window->windowState());

    if (m_compositor)
        m_compositor->addRasterWindow(this->window());
}

QPepperPlatformWindow::~QPepperPlatformWindow()
{
    if (m_compositor)
        m_compositor->removeWindow(this->window());
}

WId QPepperPlatformWindow::winId() const
{
    return WId(this);
}

void QPepperPlatformWindow::setVisible(bool visible)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setVisible" << visible;

    QWindowSystemInterface::handleExposeEvent(window(), visible ?
                                              QRect(QPoint(), geometry().size()) :
                                              QRect());
    QPepperInstance::get()->scheduleWindowSystemEventsFlush();

    if (m_compositor)
        m_compositor->setVisible(this->window(), visible);
}

void QPepperPlatformWindow::setWindowState(Qt::WindowState state)
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
        QPepperInstance::get()->scheduleWindowSystemEventsFlush();
        break;
    }
    default:
        break;
    }
}

void QPepperPlatformWindow::raise()
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "raise";

    if (m_compositor)
        m_compositor->raise(this->window());
}

void QPepperPlatformWindow::lower()
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "lower";

    if (m_compositor)
        m_compositor->lower(this->window());
}

void QPepperPlatformWindow::setGeometry(const QRect &rect)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setGeometry" << rect;

    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);
    QPepperInstance::get()->scheduleWindowSystemEventsFlush();
}

void QPepperPlatformWindow::setParent(const QPlatformWindow *parent)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "QPepperPlatformWindow::setParent" << parent;

    if (m_compositor)
        m_compositor->setParent(this->window(), parent->window());
}

bool QPepperPlatformWindow::setKeyboardGrabEnabled(bool grab)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setKeyboardGrabEnabled" << grab;

    Q_UNUSED(grab);
    return false;
}

bool QPepperPlatformWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(QT_PLATFORM_PEPPER_WINDOW) << "setMouseGrabEnabled" << grab;

    Q_UNUSED(grab);
    return false;
}

qreal QPepperPlatformWindow::devicePixelRatio() const
{
    return QPepperInstance::get()->devicePixelRatio();
}


QT_END_NAMESPACE
