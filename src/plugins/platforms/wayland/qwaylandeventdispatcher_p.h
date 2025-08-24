// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDEVENTDISPATCHER_P_H
#define QWAYLANDEVENTDISPATCHER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qunixeventdispatcher_qpa_p.h>
#if !defined(QT_NO_GLIB) && !defined(Q_OS_WIN)
#include <QtGui/private/qeventdispatcher_glib_p.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandEventDispatcher
{
public:
    static QAbstractEventDispatcher *createEventDispatcher();

    static QWaylandEventDispatcher *eventDispatcher;

public:
    QWaylandEventDispatcher();
    virtual ~QWaylandEventDispatcher();

    virtual QEventLoop::ProcessEventsFlags flags() const = 0;
};

class QWaylandUnixEventDispatcher : public QUnixEventDispatcherQPA, QWaylandEventDispatcher
{
    Q_OBJECT
public:
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    QEventLoop::ProcessEventsFlags flags() const override;

private:
    QEventLoop::ProcessEventsFlags m_flags;
};

#if !defined(QT_NO_GLIB) && !defined(Q_OS_WIN)

class QWaylandGlibEventDispatcher : public QPAEventDispatcherGlib, QWaylandEventDispatcher
{
    Q_OBJECT
public:
    QEventLoop::ProcessEventsFlags flags() const override;
};

#endif

}

QT_END_NAMESPACE

#endif
