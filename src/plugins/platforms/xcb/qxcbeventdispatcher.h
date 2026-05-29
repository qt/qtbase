// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QtCore/QObject>
#include <QtCore/QEventLoop>

#include <QtCore/private/qeventdispatcher_unix_p.h>
#if QT_CONFIG(glib)
#include <QtCore/private/qeventdispatcher_glib_p.h>
#include <glib.h>
#endif

QT_BEGIN_NAMESPACE

class QXcbConnection;

class QXcbUnixEventDispatcher : public QEventDispatcherUNIX
{
    Q_OBJECT
public:
    explicit QXcbUnixEventDispatcher(QXcbConnection *connection, QObject *parent = nullptr);
    ~QXcbUnixEventDispatcher();
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

private:
    QXcbConnection *m_connection;
};

#if QT_CONFIG(glib)

struct XcbEventSource;
class QXcbGlibEventDispatcherPrivate;

class QXcbGlibEventDispatcher : public QEventDispatcherGlib
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QXcbGlibEventDispatcher)

public:
    explicit QXcbGlibEventDispatcher(QXcbConnection *connection, QObject *parent = nullptr);
    ~QXcbGlibEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
    QEventLoop::ProcessEventsFlags flags() const { return m_flags; }

private:
    XcbEventSource *m_xcbEventSource;
    GSourceFuncs m_xcbEventSourceFuncs;
    QEventLoop::ProcessEventsFlags m_flags;
};

class QXcbGlibEventDispatcherPrivate : public QEventDispatcherGlibPrivate
{
    Q_DECLARE_PUBLIC(QXcbGlibEventDispatcher)

public:
    QXcbGlibEventDispatcherPrivate();
};

#endif // QT_CONFIG(glib)

class QXcbEventDispatcher
{
public:
    static QAbstractEventDispatcher *createEventDispatcher(QXcbConnection *connection);
};

QT_END_NAMESPACE
