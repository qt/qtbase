/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QEVDEVTABLETHANDLER_P_H
#define QEVDEVTABLETHANDLER_P_H

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

#include <QObject>
#include <QString>
#include <QThread>
#include <QtCore/private/qthread_p.h>

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QEvdevTabletData;

class QEvdevTabletHandler : public QObject
{
public:
    explicit QEvdevTabletHandler(const QString &device, const QString &spec = QString(), QObject *parent = nullptr);
    ~QEvdevTabletHandler();

    qint64 deviceId() const;

    void readData();

private:
    bool queryLimits();

    int m_fd;
    QString m_device;
    QSocketNotifier *m_notifier;
    QEvdevTabletData *d;
};

class QEvdevTabletHandlerThread : public QDaemonThread
{
public:
    explicit QEvdevTabletHandlerThread(const QString &device, const QString &spec, QObject *parent = nullptr);
    ~QEvdevTabletHandlerThread();
    void run() override;
    QEvdevTabletHandler *handler() { return m_handler; }

private:
    QString m_device;
    QString m_spec;
    QEvdevTabletHandler *m_handler;
};

QT_END_NAMESPACE

#endif // QEVDEVTABLETHANDLER_P_H
