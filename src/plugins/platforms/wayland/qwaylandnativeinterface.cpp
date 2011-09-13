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

#include "qwaylandnativeinterface.h"
#include "qwaylanddisplay.h"
#include "qwaylandwindow.h"
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/QScreen>

#include "windowmanager_integration/qwaylandwindowmanagerintegration.h"

void *QWaylandNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();

    if (lowerCaseResource == "display")
	return qPlatformScreenForWindow(window)->display()->wl_display();
    if (lowerCaseResource == "surface") {
	return ((QWaylandWindow *) window->handle())->wl_surface();
    }

    return NULL;
}


QWaylandScreen * QWaylandNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QWaylandScreen *screen;

    if (window) {
        screen = static_cast<QWaylandScreen *>(window->screen()->handle());
    } else {
        screen = static_cast<QWaylandScreen *>(QGuiApplication::primaryScreen()->handle());
    }
    return screen;
}

QVariantMap QWaylandNativeInterface::windowProperties(QPlatformWindow *window) const
{
    return m_windowProperties.value(window);
}

QVariant QWaylandNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    const QVariantMap properties = m_windowProperties.value(window);
    return properties.value(name);
}

QVariant QWaylandNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    const QVariantMap properties = m_windowProperties.value(window);
    return properties.value(name, defaultValue);
}

void QWaylandNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QVariantMap props = m_windowProperties.value(window);
    props.insert(name, value);
    m_windowProperties.insert(window, props);

    QWaylandWindow *wlWindow = static_cast<QWaylandWindow*>(window);
    QWaylandWindowManagerIntegration::instance()->setWindowProperty(wlWindow, name, value);

    emit windowPropertyChanged(window, name);
}
