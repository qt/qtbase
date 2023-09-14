// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SERVER_H
#define SERVER_H

#include <QApplication>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QLocalServer;
QT_END_NAMESPACE

class Server : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Server)

public:
    explicit Server(QWidget *parent = nullptr);

private:
    void sendFortune();
    QLocalServer *server;
    QStringList fortunes;
};

#endif
