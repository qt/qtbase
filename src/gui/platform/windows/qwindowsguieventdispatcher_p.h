// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSGUIEVENTDISPATCHER_H
#define QWINDOWSGUIEVENTDISPATCHER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qeventdispatcher_win_p.h>
#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowsGuiEventDispatcher : public QEventDispatcherWin32
{
    Q_OBJECT
public:
    explicit QWindowsGuiEventDispatcher(QObject *parent = nullptr);

    static const char *windowsMessageName(UINT msg);

    bool QT_ENSURE_STACK_ALIGNED_FOR_SSE processEvents(QEventLoop::ProcessEventsFlags flags) override;
    void sendPostedEvents() override;

private:
    QEventLoop::ProcessEventsFlags m_flags;
};

QT_END_NAMESPACE

#endif // QWINDOWSGUIEVENTDISPATCHER_H
