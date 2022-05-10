// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FORTUNESERVER_H
#define FORTUNESERVER_H

#include <QStringList>
#include <QTcpServer>

//! [0]
class FortuneServer : public QTcpServer
{
    Q_OBJECT

public:
    FortuneServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QStringList fortunes;
};
//! [0]

#endif
