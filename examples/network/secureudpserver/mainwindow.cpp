// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "nicselector.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow()
    : ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&server, &DtlsServer::errorMessage, this, &MainWindow::addErrorMessage);
    connect(&server, &DtlsServer::warningMessage, this, &MainWindow::addWarningMessage);
    connect(&server, &DtlsServer::infoMessage, this, &MainWindow::addInfoMessage);
    connect(&server, &DtlsServer::datagramReceived, this, &MainWindow::addClientMessage);

    updateUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked()
{
    if (!server.isListening()) {
        NicSelector ipDialog;
        if (ipDialog.exec() == QDialog::Accepted) {
            const QHostAddress address = ipDialog.selectedIp();
            const quint16 port = ipDialog.selectedPort();
            if (address.isNull()) {
                addErrorMessage(tr("Failed to start listening, no valid address/port"));
            } else if (server.listen(address, port)) {
                addInfoMessage(tr("Server is listening on address %1 and port %2")
                                 .arg(address.toString())
                                 .arg(port));
            }
        }
    } else {
        server.close();
        addInfoMessage(tr("Server is not accepting new connections"));
    }

    updateUi();
}

void MainWindow::on_quitButton_clicked()
{
    QCoreApplication::exit(0);
}

void MainWindow::updateUi()
{
    server.isListening() ? ui->startButton->setText(tr("Stop listening"))
                         : ui->startButton->setText(tr("Start listening"));
}

//! [0]
const QString colorizer(QStringLiteral("<font color=\"%1\">%2</font><br>"));

void MainWindow::addErrorMessage(const QString &message)
{
    ui->serverInfo->insertHtml(colorizer.arg(QStringLiteral("Crimson"), message));
}

void MainWindow::addWarningMessage(const QString &message)
{
    ui->serverInfo->insertHtml(colorizer.arg(QStringLiteral("DarkOrange"), message));
}

void MainWindow::addInfoMessage(const QString &message)
{
    ui->serverInfo->insertHtml(colorizer.arg(QStringLiteral("DarkBlue"), message));
}

void MainWindow::addClientMessage(const QString &peerInfo, const QByteArray &datagram,
                                  const QByteArray &plainText)
{
    static const QString messageColor = QStringLiteral("DarkMagenta");
    static const QString formatter = QStringLiteral("<br>---------------"
                                                    "<br>A message from %1"
                                                    "<br>DTLS datagram:<br> %2"
                                                    "<br>As plain text:<br> %3");

    const QString html = formatter.arg(peerInfo, QString::fromUtf8(datagram.toHex(' ')),
                                       QString::fromUtf8(plainText));
    ui->messages->insertHtml(colorizer.arg(messageColor, html));
}
//! [0]
