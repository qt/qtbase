// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
    Q_UNUSED(timeout);
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

    GSource *source = g_source_new(&m_xcbEventSourceFuncs, sizeof(XcbEventSource));
    g_source_set_name(source, "[Qt] XcbEventSource");
    m_xcbEventSource = reinterpret_cast<XcbEventSource *>(source);

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

#include "moc_qxcbeventdispatcher.cpp"
