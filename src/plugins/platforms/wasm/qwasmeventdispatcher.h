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
****************************************************************************/

#ifndef QWASMEVENTDISPATCHER_H
#define QWASMEVENTDISPATCHER_H

#include <QtCore/qhash.h>
#include <QtCore/qloggingcategory.h>
#include <QtEventDispatcherSupport/private/qunixeventdispatcher_qpa_p.h>

QT_BEGIN_NAMESPACE

class QWasmEventDispatcherPrivate;

class QWasmEventDispatcher : public QUnixEventDispatcherQPA
{
    Q_DECLARE_PRIVATE(QWasmEventDispatcher)
public:
    explicit QWasmEventDispatcher(QObject *parent = nullptr);
    ~QWasmEventDispatcher();

    static bool registerRequestUpdateCallback(std::function<void(void)> callback);
    static void maintainTimers();

protected:
    bool processEvents(QEventLoop::ProcessEventsFlags flags) override;
    void doMaintainTimers();
    void wakeUp() override;
    static void mainThreadWakeUp(void *eventDispatcher);

private:
    bool m_hasMainLoop = false;
    bool m_hasZeroTimer = false;
    uint64_t m_currentTargetTime = std::numeric_limits<uint64_t>::max();
    QVector<std::function<void(void)>> m_requestUpdateCallbacks;
};

QT_END_NAMESPACE

#endif
