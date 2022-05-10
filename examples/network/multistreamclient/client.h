// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CLIENT_H
#define CLIENT_H

#include <QDialog>
#include <QList>
#include <QSctpSocket>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
class QByteArray;
QT_END_NAMESPACE

class Consumer;

class Client : public QDialog
{
    Q_OBJECT
public:
    explicit Client(QWidget *parent = nullptr);
    virtual ~Client();

private slots:
    void connected();
    void disconnected();
    void requestConnect();
    void readDatagram(int channel);
    void displayError(QAbstractSocket::SocketError socketError);
    void enableConnectButton();
    void writeDatagram(const QByteArray &ba);

private:
    QList<Consumer *> consumers;
    QSctpSocket *sctpSocket;

    QComboBox *hostCombo;
    QLineEdit *portLineEdit;
    QPushButton *connectButton;
};

#endif
