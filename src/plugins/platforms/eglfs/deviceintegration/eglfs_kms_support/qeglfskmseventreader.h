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

#ifndef QEGLFSKKMSEVENTREADER_H
#define QEGLFSKKMSEVENTREADER_H

#include "private/qeglfsglobal_p.h"
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE

class QEglFSKmsDevice;

struct QEglFSKmsEventHost : public QObject
{
    struct PendingFlipWait {
        void *key;
        QMutex *mutex;
        QWaitCondition *cond;
    };

    static const int MAX_FLIPS = 32;
    void *completedFlips[MAX_FLIPS] = {};
    QEglFSKmsEventHost::PendingFlipWait pendingFlipWaits[MAX_FLIPS] = {};

    bool event(QEvent *event) override;
    void updateStatus();
    void handlePageFlipCompleted(void *key);
};

class QEglFSKmsEventReaderThread : public QThread
{
public:
    QEglFSKmsEventReaderThread(int fd) : m_fd(fd) { }
    void run() override;
    QEglFSKmsEventHost *eventHost() { return &m_ev; }

private:
    int m_fd;
    QEglFSKmsEventHost m_ev;
};

class Q_EGLFS_EXPORT QEglFSKmsEventReader
{
public:
    ~QEglFSKmsEventReader();

    void create(QEglFSKmsDevice *device);
    void destroy();

    void startWaitFlip(void *key, QMutex *mutex, QWaitCondition *cond);

private:
    QEglFSKmsDevice *m_device = nullptr;
    QEglFSKmsEventReaderThread *m_thread = nullptr;
};

QT_END_NAMESPACE

#endif // QEGLFSKKMSEVENTREADER_H
