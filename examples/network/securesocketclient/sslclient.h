// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SSLCLIENT_H
#define SSLCLIENT_H

#include <QtNetwork>

QT_REQUIRE_CONFIG(ssl);

#include <QtWidgets>

QT_BEGIN_NAMESPACE
class Ui_Form;
QT_END_NAMESPACE

class SslClient : public QWidget
{
    Q_OBJECT
public:
    explicit SslClient(QWidget *parent = nullptr);
    ~SslClient();

private slots:
    void updateEnabledState();
    void secureConnect();
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketEncrypted();
    void socketReadyRead();
    void sendData();
    void socketError(QAbstractSocket::SocketError error);
    void sslErrors(const QList<QSslError> &errors);
    void displayCertificateInfo();

private:
    void setupUi();
    void setupSecureSocket();
    void appendString(const QString &line);

    QSslSocket *socket = nullptr;
    QToolButton *padLock = nullptr;
    Ui_Form *form = nullptr;
    bool handlingSocketError = false;
    bool executingDialog = false;
};

#endif
