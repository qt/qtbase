// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FORTUNETHREAD_H
#define FORTUNETHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

//! [0]
class FortuneThread : public QThread
{
    Q_OBJECT

public:
    FortuneThread(QObject *parent = nullptr);
    ~FortuneThread();

    void requestNewFortune(const QString &hostName, quint16 port);
    void run() override;

signals:
    void newFortune(const QString &fortune);
    void error(int socketError, const QString &message);

private:
    QString hostName;
    quint16 port;
    QMutex mutex;
    QWaitCondition cond;
    bool quit;
};
//! [0]

#endif
