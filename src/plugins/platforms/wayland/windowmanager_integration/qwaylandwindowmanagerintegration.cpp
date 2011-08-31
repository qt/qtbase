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

#include "qwaylandwindowmanagerintegration.h"
#include "wayland-windowmanager-client-protocol.h"
#include "qwaylandwindow.h"

#include <stdint.h>
#include <QtCore/QEvent>
#include <QtCore/QHash>
#include <QtGui/QPlatformNativeInterface>
#include <QtGui/QPlatformWindow>
#include <QtGui/QtEvents>
#include <QtGui/QWidget>
#include <QtGui/QApplication>

#include <QDebug>

class QWaylandWindowManagerIntegrationPrivate {
public:
    QWaylandWindowManagerIntegrationPrivate(QWaylandDisplay *waylandDisplay);
    bool m_blockPropertyUpdates;
    QWaylandDisplay *m_waylandDisplay;
    struct wl_windowmanager *m_waylandWindowManager;
    QHash<QWaylandWindow*,QVariantMap> m_queuedProperties;

};

QWaylandWindowManagerIntegrationPrivate::QWaylandWindowManagerIntegrationPrivate(QWaylandDisplay *waylandDisplay)
    : m_blockPropertyUpdates(false)
    , m_waylandDisplay(waylandDisplay)
    , m_waylandWindowManager(0)
{

}

QWaylandWindowManagerIntegration *QWaylandWindowManagerIntegration::m_instance = 0;

const struct wl_windowmanager_listener QWaylandWindowManagerIntegration::m_windowManagerListener = {
    QWaylandWindowManagerIntegration::wlHandleOnScreenVisibilityChange,
    QWaylandWindowManagerIntegration::wlHandleScreenOrientationChange,
    QWaylandWindowManagerIntegration::wlHandleWindowPropertyChange
};

QWaylandWindowManagerIntegration *QWaylandWindowManagerIntegration::createIntegration(QWaylandDisplay *waylandDisplay)
{
    return new QWaylandWindowManagerIntegration(waylandDisplay);
}

QWaylandWindowManagerIntegration::QWaylandWindowManagerIntegration(QWaylandDisplay *waylandDisplay)
    : d_ptr(new QWaylandWindowManagerIntegrationPrivate(waylandDisplay))
{
    m_instance = this;

    wl_display_add_global_listener(d_ptr->m_waylandDisplay->wl_display(),
                                   QWaylandWindowManagerIntegration::wlHandleListenerGlobal,
                                   this);
}

QWaylandWindowManagerIntegration::~QWaylandWindowManagerIntegration()
{

}

QWaylandWindowManagerIntegration *QWaylandWindowManagerIntegration::instance()
{
    return m_instance;
}

struct wl_windowmanager *QWaylandWindowManagerIntegration::windowManager() const
{
    Q_D(const QWaylandWindowManagerIntegration);
    return d->m_waylandWindowManager;
}

void QWaylandWindowManagerIntegration::wlHandleListenerGlobal(wl_display *display, uint32_t id, const char *interface, uint32_t version, void *data)
{
    Q_UNUSED(version);
    if (strcmp(interface, "wl_windowmanager") == 0) {
        QWaylandWindowManagerIntegration *integration = static_cast<QWaylandWindowManagerIntegration *>(data);
        integration->d_ptr->m_waylandWindowManager = wl_windowmanager_create(display, id, 1);
        wl_windowmanager *windowManager = integration->d_ptr->m_waylandWindowManager;
        wl_windowmanager_add_listener(windowManager, &m_windowManagerListener, integration);
    }
}

void QWaylandWindowManagerIntegration::mapClientToProcess(long long processId)
{
    Q_D(QWaylandWindowManagerIntegration);
    if (d->m_waylandWindowManager)
        wl_windowmanager_map_client_to_process(d->m_waylandWindowManager, (uint32_t) processId);
}

void QWaylandWindowManagerIntegration::authenticateWithToken(const QByteArray &token)
{
    Q_D(QWaylandWindowManagerIntegration);
    QByteArray authToken = token;
    if (authToken.isEmpty())
        authToken = qgetenv("WL_AUTHENTICATION_TOKEN");

    if (d->m_waylandWindowManager && !authToken.isEmpty()) {
        wl_windowmanager_authenticate_with_token(d->m_waylandWindowManager, authToken.constData());
    }
}

static wl_array writePropertyValue(const QVariant &value)
{
    QByteArray byteValue;
    QDataStream ds(&byteValue, QIODevice::WriteOnly);
    ds << value;

    wl_array data;
    data.size = byteValue.size();
    data.data = (void*)byteValue.constData();
    data.alloc = 0;

    return data;
}

void QWaylandWindowManagerIntegration::setWindowProperty(QWaylandWindow *window, const QString &propertyName, const QVariant &propertyValue)
{
    Q_D(QWaylandWindowManagerIntegration);
    if (d->m_blockPropertyUpdates)
        return;

    if (window->wl_surface()) {
        wl_array data = writePropertyValue(propertyValue);
        wl_windowmanager_update_generic_property(d->m_waylandWindowManager, window->wl_surface(),
                                                 propertyName.toLatin1().constData(),
                                                 &data);
    } else {
        QVariantMap props = d->m_queuedProperties.value(window);
        props.insert(propertyName, propertyValue);
        d->m_queuedProperties.insert(window, props);
        // ### TODO we'll need to add listening to destroyed() of QWindow that owns QWaylandWindow
        // once refactor changes are in, and connect to removeQueuedPropertiesForWindow().
    }
}

void QWaylandWindowManagerIntegration::flushPropertyChanges(QWaylandWindow *windowToFlush)
{
    // write all changes we got while we did not have a surface.
    // this can happen during startup, for example, or while the window is hidden.
    Q_D(QWaylandWindowManagerIntegration);

    if (!windowToFlush)
        return;

    QVariantMap properties = d->m_queuedProperties.value(windowToFlush);
    wl_surface *surface = windowToFlush->wl_surface();

    QMapIterator<QString, QVariant> pIt(properties);
    while (pIt.hasNext()) {
        pIt.next();
        wl_array data = writePropertyValue(pIt.value());
        wl_windowmanager_update_generic_property(d->m_waylandWindowManager, surface, pIt.key().toLatin1().constData(), &data);
    }

    d->m_queuedProperties.clear();
}

void QWaylandWindowManagerIntegration::removeQueuedPropertiesForWindow()
{
    //  TODO enable this later once refactor changes are in.
//    Q_D(QWaylandWindowManagerIntegration);
//    QWaylandWindow *window = 0;
//    d->m_queuedProperties.remove(window);
}

void QWaylandWindowManagerIntegration::wlHandleOnScreenVisibilityChange(void *data, struct wl_windowmanager *wl_windowmanager, int visible)
{
    Q_UNUSED(data);
    Q_UNUSED(wl_windowmanager);
    QEvent evt(visible != 0 ? QEvent::ApplicationActivate : QEvent::ApplicationDeactivate);
    QCoreApplication::sendEvent(QCoreApplication::instance(), &evt);
}

void QWaylandWindowManagerIntegration::wlHandleScreenOrientationChange(void *data, struct wl_windowmanager *wl_windowmanager, int screenOrientation)
{
    Q_UNUSED(data);
    Q_UNUSED(wl_windowmanager);
    QScreenOrientationChangeEvent event(screenOrientation);
    QCoreApplication::sendEvent(QCoreApplication::instance(), &event);
}

void QWaylandWindowManagerIntegration::wlHandleWindowPropertyChange(void *data, struct wl_windowmanager *wl_windowmanager,
                                                                    struct wl_surface *surface,
                                                                    const char *propertyName, struct wl_array *propertyValue)
{
    // window manager changes a window property
    Q_UNUSED(data);
    Q_UNUSED(wl_windowmanager);

    QVariant variantValue;
    QByteArray baValue = QByteArray((const char*)propertyValue->data, propertyValue->size);
    QDataStream ds(&baValue, QIODevice::ReadOnly);
    ds >> variantValue;

    QPlatformNativeInterface *nativeInterface = qApp->platformNativeInterface();
    QWaylandWindowManagerIntegration *inst = QWaylandWindowManagerIntegration::instance();

    QWidgetList widgets = qApp->topLevelWidgets();
    foreach (QWidget *widget, widgets) {
        QPlatformWindow *platformWindowForWidget = widget->platformWindow();
        if (!platformWindowForWidget)
            continue;
        QWaylandWindow *window = static_cast<QWaylandWindow*>(platformWindowForWidget);
        wl_surface *windowSurface = (wl_surface*)nativeInterface->nativeResourceForWidget(QByteArray("surface"), widget);
        if (windowSurface == surface) {
            inst->handleWindowPropertyChange(window, QString(propertyName), variantValue);
            break;
        }
    }
}

void QWaylandWindowManagerIntegration::handleWindowPropertyChange(QWaylandWindow *window,
                                                                  const QString &propertyName, const QVariant &propertyValue)
{
    Q_D(QWaylandWindowManagerIntegration);
    d->m_blockPropertyUpdates = true;

    QPlatformNativeInterface *nativeInterface = qApp->platformNativeInterface();
    nativeInterface->setWindowProperty(window, propertyName, propertyValue);

    d->m_blockPropertyUpdates = false;
}
