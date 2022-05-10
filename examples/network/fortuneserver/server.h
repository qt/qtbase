// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QString>
#include <QList>

QT_BEGIN_NAMESPACE
class QLabel;
class QTcpServer;
QT_END_NAMESPACE

//! [0]
class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = nullptr);

private slots:
    void sendFortune();

private:
    void initServer();

    QLabel *statusLabel = nullptr;
    QTcpServer *tcpServer = nullptr;
    QList<QString> fortunes;
};
//! [0]

#endif
