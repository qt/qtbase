// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FORTUNETHREAD_H
#define FORTUNETHREAD_H

#include <QThread>
#include <QTcpSocket>

//! [0]
class FortuneThread : public QThread
{
    Q_OBJECT

public:
    FortuneThread(qintptr socketDescriptor, const QString &fortune, QObject *parent);

    void run() override;

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    qintptr socketDescriptor;
    QString text;
};
//! [0]

#endif
