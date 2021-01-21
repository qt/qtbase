/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

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

QT_BEGIN_NAMESPACE

class QWindowsGuiEventDispatcher : public QEventDispatcherWin32
{
    Q_OBJECT
public:
    explicit QWindowsGuiEventDispatcher(QObject *parent = 0);

    static const char *windowsMessageName(UINT msg);

    bool QT_ENSURE_STACK_ALIGNED_FOR_SSE processEvents(QEventLoop::ProcessEventsFlags flags) override;
    void sendPostedEvents() override;

private:
    QEventLoop::ProcessEventsFlags m_flags;
};

QT_END_NAMESPACE

#endif // QWINDOWSGUIEVENTDISPATCHER_H
