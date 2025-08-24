// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandeventdispatcher_p.h"

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandEventDispatcher *QWaylandEventDispatcher::eventDispatcher = nullptr;

QAbstractEventDispatcher *QWaylandEventDispatcher::createEventDispatcher()
{
#if !defined(QT_NO_GLIB) && !defined(Q_OS_WIN)
    if (qEnvironmentVariableIsEmpty("QT_NO_GLIB") && QEventDispatcherGlib::versionSupported())
        return new QWaylandGlibEventDispatcher();
#endif
        return new QWaylandUnixEventDispatcher();
}

QWaylandEventDispatcher::QWaylandEventDispatcher()
{
    Q_ASSERT(!eventDispatcher);
    eventDispatcher = this;
}

QWaylandEventDispatcher::~QWaylandEventDispatcher()
{
    Q_ASSERT(eventDispatcher == this);
    eventDispatcher = nullptr;
}

bool QWaylandUnixEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_flags = flags;
    return QUnixEventDispatcherQPA::processEvents(flags);
}

QEventLoop::ProcessEventsFlags QWaylandUnixEventDispatcher::flags() const
{
    return m_flags;
}

#if !defined(QT_NO_GLIB) && !defined(Q_OS_WIN)

QEventLoop::ProcessEventsFlags QWaylandGlibEventDispatcher::flags() const
{
    return m_flags;
}

#endif

}

QT_END_NAMESPACE
