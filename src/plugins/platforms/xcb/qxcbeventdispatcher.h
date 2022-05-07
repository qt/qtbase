/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/
#ifndef QXCBEVENTDISPATCHER_H
#define QXCBEVENTDISPATCHER_H

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

#endif // QXCBEVENTDISPATCHER_H
