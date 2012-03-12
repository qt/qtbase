/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBNATIVEINTERFACE_H
#define QXCBNATIVEINTERFACE_H

#include <QtGui/QPlatformNativeInterface>

QT_BEGIN_NAMESPACE

class QWidget;
class QXcbScreen;

class QXcbNativeInterface : public QPlatformNativeInterface
{
public:
    enum ResourceType {
        Display,
        EglDisplay,
        Connection,
        Screen,
        GraphicsDevice,
        EglContext
    };

    enum EventFilterType {
        GenericEventFilter,
        EventFilterCount
    };

    QXcbNativeInterface();

    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context);
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window);

    inline const QByteArray &genericEventFilterType() const { return m_genericEventFilterType; }
    EventFilter setEventFilter(const QByteArray &eventType, EventFilter filter);
    EventFilter eventFilter(EventFilterType type) const { return m_eventFilters[type]; }

    void *displayForWindow(QWindow *window);
    void *eglDisplayForWindow(QWindow *window);
    void *connectionForWindow(QWindow *window);
    void *screenForWindow(QWindow *window);
    void *graphicsDeviceForWindow(QWindow *window);

    void *eglContextForContext(QOpenGLContext *context);

private:
    const QByteArray m_genericEventFilterType;
    EventFilter m_eventFilters[EventFilterCount];

    static QXcbScreen *qPlatformScreenForWindow(QWindow *window);
};

QT_END_NAMESPACE

#endif // QXCBNATIVEINTERFACE_H
