// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QProgressBar;
class QPushButton;
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog(QWidget *parent = nullptr);

public slots:
    void start();
    void acceptConnection();
    void startTransfer();
    void updateServerProgress();
    void updateClientProgress(qint64 numBytes);
    void displayError(QAbstractSocket::SocketError socketError);

private:
    QProgressBar *clientProgressBar = nullptr;
    QProgressBar *serverProgressBar = nullptr;
    QLabel *clientStatusLabel = nullptr;
    QLabel *serverStatusLabel = nullptr;

    QPushButton *startButton = nullptr;
    QPushButton *quitButton = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    QTcpServer tcpServer;
    QTcpSocket tcpClient;
    QTcpSocket *tcpServerConnection = nullptr;
    int bytesToWrite = 0;
    int bytesWritten = 0;
    int bytesReceived = 0;
};

#endif
