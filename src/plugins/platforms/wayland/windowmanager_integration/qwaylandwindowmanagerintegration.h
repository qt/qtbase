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

#ifndef QWAYLANDWINDOWMANAGERINTEGRATION_H
#define QWAYLANDWINDOWMANAGERINTEGRATION_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include "wayland-client.h"
#include "qwaylanddisplay.h"

class QWaylandWindow;

class QWaylandWindowManagerIntegrationPrivate;

class QWaylandWindowManagerIntegration : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWaylandWindowManagerIntegration)
public:
    explicit QWaylandWindowManagerIntegration(QWaylandDisplay *waylandDisplay);
    virtual ~QWaylandWindowManagerIntegration();
    static QWaylandWindowManagerIntegration *createIntegration(QWaylandDisplay *waylandDisplay);
    struct wl_windowmanager *windowManager() const;

    static QWaylandWindowManagerIntegration *instance();

    void mapSurfaceToProcess(struct wl_surface *surface, long long processId);
    void mapClientToProcess(long long processId);
    void authenticateWithToken(const QByteArray &token = QByteArray());
    void setWindowProperty(QWaylandWindow *window, const QString &propertyName, const QVariant &propertyValue);

    void flushPropertyChanges(QWaylandWindow *windowToFlush);

private slots:
    void removeQueuedPropertiesForWindow();

private:
    static void wlHandleListenerGlobal(wl_display *display, uint32_t id,
                                       const char *interface, uint32_t version, void *data);

    static void wlHandleOnScreenVisibilityChange(void *data, struct wl_windowmanager *wl_windowmanager, int visible);
    static void wlHandleScreenOrientationChange(void *data, struct wl_windowmanager *wl_windowmanager, int screenOrientation);
    static void wlHandleWindowPropertyChange(void *data, struct wl_windowmanager *wl_windowmanager,
                                             struct wl_surface *surface,
                                             const char *propertyName, struct wl_array *propertyValue);

    void handleWindowPropertyChange(QWaylandWindow *window, const QString &propertyName, const QVariant &propertyValue);

private:
    QScopedPointer<QWaylandWindowManagerIntegrationPrivate> d_ptr;
    static QWaylandWindowManagerIntegration *m_instance;

    static const struct wl_windowmanager_listener m_windowManagerListener;
};

#endif // QWAYLANDWINDOWMANAGERINTEGRATION_H
