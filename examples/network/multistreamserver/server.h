// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QList>
#include <QList>

QT_BEGIN_NAMESPACE
class QSctpServer;
class QSctpSocket;
class QLabel;
class QByteArray;
QT_END_NAMESPACE

class Provider;

class Server : public QDialog
{
    Q_OBJECT
public:
    explicit Server(QWidget *parent = nullptr);
    virtual ~Server();

public slots:
    int exec() override;

private slots:
    void newConnection();
    void clientDisconnected();
    void readDatagram(int channel);
    void writeDatagram(QSctpSocket *to, const QByteArray &ba);

private:
    QList<Provider *> providers;
    QSctpServer *sctpServer;
    QList<QSctpSocket *> connections;

    QLabel *statusLabel;
};

#endif
