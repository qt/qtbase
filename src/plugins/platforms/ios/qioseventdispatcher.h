// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSEVENTDISPATCHER_H
#define QIOSEVENTDISPATCHER_H

#include <QtCore/private/qeventdispatcher_cf_p.h>

QT_BEGIN_NAMESPACE

class QIOSEventDispatcher : public QEventDispatcherCoreFoundation
{
    Q_OBJECT

public:
    static QIOSEventDispatcher* create();
    bool processPostedEvents() override;

    static bool isQtApplication();

protected:
    explicit QIOSEventDispatcher(QObject *parent = nullptr);
};

class QIOSJumpingEventDispatcher : public QIOSEventDispatcher
{
    Q_OBJECT

public:
    QIOSJumpingEventDispatcher(QObject *parent = nullptr);
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;

    // Public since we can't friend Objective-C methods
    void handleRunLoopExit(CFRunLoopActivity activity);

    void interruptEventLoopExec();

private:
    uint m_processEventLevel;
    RunLoopObserver<QIOSJumpingEventDispatcher> m_runLoopExitObserver;
};

QT_END_NAMESPACE

#endif // QIOSEVENTDISPATCHER_H
