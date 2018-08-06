/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
