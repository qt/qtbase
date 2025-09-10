// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventdispatcher_glib_p.h"

#include "qguiapplication.h"

#include "qplatformdefs.h"

#include <glib.h>
#include "private/qguiapplication_p.h"

QT_BEGIN_NAMESPACE

struct GUserEventSource
{
    GSource source;
    QPAEventDispatcherGlib *q;
    QPAEventDispatcherGlibPrivate *d;
};

static gboolean userEventSourcePrepare(GSource *source, gint *timeout)
{
    Q_UNUSED(timeout);
    GUserEventSource *userEventSource = reinterpret_cast<GUserEventSource *>(source);
    return userEventSource->d->wakeUpCalled;
}

static gboolean userEventSourceCheck(GSource *source)
{
    return userEventSourcePrepare(source, nullptr);
}

static gboolean userEventSourceDispatch(GSource *source, GSourceFunc, gpointer)
{
    GUserEventSource *userEventSource = reinterpret_cast<GUserEventSource *>(source);
    QPAEventDispatcherGlib *dispatcher = userEventSource->q;
    QWindowSystemInterface::sendWindowSystemEvents(dispatcher->m_flags);
    return true;
}

static GSourceFuncs userEventSourceFuncs = {
    userEventSourcePrepare,
    userEventSourceCheck,
    userEventSourceDispatch,
    NULL,
    NULL,
    NULL
};

QPAEventDispatcherGlibPrivate::QPAEventDispatcherGlibPrivate(GMainContext *context)
    : QEventDispatcherGlibPrivate(context)
{
    Q_Q(QPAEventDispatcherGlib);

    GSource *source = g_source_new(&userEventSourceFuncs, sizeof(GUserEventSource));
    g_source_set_name(source, "[Qt] GUserEventSource");
    userEventSource = reinterpret_cast<GUserEventSource *>(source);

    userEventSource->q = q;
    userEventSource->d = this;
    g_source_set_can_recurse(&userEventSource->source, true);
    g_source_attach(&userEventSource->source, mainContext);
}

QPAEventDispatcherGlibPrivate::~QPAEventDispatcherGlibPrivate()
    = default;

QPAEventDispatcherGlib::QPAEventDispatcherGlib(QObject *parent)
    : QEventDispatcherGlib(*new QPAEventDispatcherGlibPrivate, parent)
    , m_flags(QEventLoop::AllEvents)
{
    Q_D(QPAEventDispatcherGlib);
    d->userEventSource->q = this;
}

QPAEventDispatcherGlib::~QPAEventDispatcherGlib()
{
    Q_D(QPAEventDispatcherGlib);

    g_source_destroy(&d->userEventSource->source);
    g_source_unref(&d->userEventSource->source);
    d->userEventSource = nullptr;
}

bool QPAEventDispatcherGlib::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    m_flags = flags;
    return QEventDispatcherGlib::processEvents(m_flags);
}

QT_END_NAMESPACE

#include "moc_qeventdispatcher_glib_p.cpp"
