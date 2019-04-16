/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include "qxcbeventdispatcher.h"
#include "qxcbconnection.h"

#include <QtCore/QCoreApplication>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QXcbUnixEventDispatcher::QXcbUnixEventDispatcher(QXcbConnection *connection, QObject *parent)
    : QEventDispatcherUNIX(parent)
    , m_connection(connection)
{
}

QXcbUnixEventDispatcher::~QXcbUnixEventDispatcher()
{
}

bool QXcbUnixEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    const bool didSendEvents = QEventDispatcherUNIX::processEvents(flags);
    m_connection->processXcbEvents(flags);
    // The following line should not be necessary after QTBUG-70095
    return QWindowSystemInterface::sendWindowSystemEvents(flags) || didSendEvents;
}

bool QXcbUnixEventDispatcher::hasPendingEvents()
{
    extern uint qGlobalPostedEventsCount();
    return qGlobalPostedEventsCount() || QWindowSystemInterface::windowSystemEventsQueued();
}

void QXcbUnixEventDispatcher::flush()
{
    if (qApp)
        qApp->sendPostedEvents();
}

#if QT_CONFIG(glib)
struct XcbEventSource
{
    GSource source;
    QXcbGlibEventDispatcher *dispatcher;
    QXcbGlibEventDispatcherPrivate *dispatcher_p;
    QXcbConnection *connection = nullptr;
};

static gboolean xcbSourcePrepare(GSource *source, gint *timeout)
{
    Q_UNUSED(timeout)
    auto xcbEventSource = reinterpret_cast<XcbEventSource *>(source);
    return xcbEventSource->dispatcher_p->wakeUpCalled;
}

static gboolean xcbSourceCheck(GSource *source)
{
    return xcbSourcePrepare(source, nullptr);
}

static gboolean xcbSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    auto xcbEventSource = reinterpret_cast<XcbEventSource *>(source);
    QEventLoop::ProcessEventsFlags flags = xcbEventSource->dispatcher->flags();
    xcbEventSource->connection->processXcbEvents(flags);
    // The following line should not be necessary after QTBUG-70095
    QWindowSystemInterface::sendWindowSystemEvents(flags);
    return true;
}

QXcbGlibEventDispatcher::QXcbGlibEventDispatcher(QXcbConnection *connection, QObject *parent)
    : QEventDispatcherGlib(*new QXcbGlibEventDispatcherPrivate(), parent)
{
    Q_D(QXcbGlibEventDispatcher);

    m_xcbEventSourceFuncs.prepare = xcbSourcePrepare;
    m_xcbEventSourceFuncs.check = xcbSourceCheck;
    m_xcbEventSourceFuncs.dispatch = xcbSourceDispatch;
    m_xcbEventSourceFuncs.finalize = nullptr;

    m_xcbEventSource = reinterpret_cast<XcbEventSource *>(
                g_source_new(&m_xcbEventSourceFuncs, sizeof(XcbEventSource)));

    m_xcbEventSource->dispatcher = this;
    m_xcbEventSource->dispatcher_p = d_func();
    m_xcbEventSource->connection = connection;

    g_source_set_can_recurse(&m_xcbEventSource->source, true);
    g_source_attach(&m_xcbEventSource->source, d->mainContext);
}

QXcbGlibEventDispatcherPrivate::QXcbGlibEventDispatcherPrivate()
{
}

QXcbGlibEventDispatcher::~QXcbGlibEventDispatcher()
{
    g_source_destroy(&m_xcbEventSource->source);
    g_source_unref(&m_xcbEventSource->source);
}

bool QXcbGlibEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_flags = flags;
    return QEventDispatcherGlib::processEvents(m_flags);
}

#endif // QT_CONFIG(glib)

QAbstractEventDispatcher *QXcbEventDispatcher::createEventDispatcher(QXcbConnection *connection)
{
#if QT_CONFIG(glib)
    if (qEnvironmentVariableIsEmpty("QT_NO_GLIB") && QEventDispatcherGlib::versionSupported()) {
        qCDebug(lcQpaXcb, "using glib dispatcher");
        return new QXcbGlibEventDispatcher(connection);
    } else
#endif
    {
        qCDebug(lcQpaXcb, "using unix dispatcher");
        return new QXcbUnixEventDispatcher(connection);
    }
}

QT_END_NAMESPACE
