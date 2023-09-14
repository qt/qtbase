// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QLocalServer>

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = nullptr);

private slots:
    void sendFortune();

private:
    QLocalServer *server;
    QStringList fortunes;
};

#endif
