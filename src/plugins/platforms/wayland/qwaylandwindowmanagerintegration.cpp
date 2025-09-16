// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandwindowmanagerintegration_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandshellsurface_p.h"

#include <stdint.h>
#include <QtCore/QEvent>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformwindow.h>
#include <QtGui/QtEvents>
#include <QtGui/QGuiApplication>

#include <QDebug>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandWindowManagerIntegration::QWaylandWindowManagerIntegration(QWaylandDisplay *waylandDisplay,
                                                                   uint id, uint version)
    : QtWayland::qt_windowmanager(waylandDisplay->object(), id, version)
{
}

QWaylandWindowManagerIntegration::~QWaylandWindowManagerIntegration()
{
    qt_windowmanager_destroy(object());
}

bool QWaylandWindowManagerIntegration::showIsFullScreen() const
{
    return m_showIsFullScreen;
}

void QWaylandWindowManagerIntegration::windowmanager_hints(int32_t showIsFullScreen)
{
    m_showIsFullScreen = showIsFullScreen;
}

void QWaylandWindowManagerIntegration::windowmanager_quit()
{
    QGuiApplication::quit();
}

void QWaylandWindowManagerIntegration::openUrl(const QUrl &url)
{
    QString data = url.toString();
    static const int chunkSize = 128;
    while (!data.isEmpty()) {
        QString chunk = data.left(chunkSize);
        data = data.mid(chunkSize);
        if (chunk.at(chunk.size() - 1).isHighSurrogate() && !data.isEmpty()) {
            chunk.append(data.at(0));
            data = data.mid(1);
        }
        open_url(!data.isEmpty(), chunk);
    }
}
}

QT_END_NAMESPACE
