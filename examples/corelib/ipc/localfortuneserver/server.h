// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SERVER_H
#define SERVER_H

#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocalServer>
#include <QPushButton>

class Server : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Server)

public:
    explicit Server(QWidget *parent = nullptr);

private:
    void sendFortune();
    void toggleListenButton();
    void listenToServer();
    void stopListening();

    QLocalServer *server;
    QLineEdit *hostLineEdit;
    QLabel *statusLabel;
    QPushButton *listenButton;
    QPushButton *stopListeningButton;
    QStringList fortunes;
    QString name;
};

#endif
