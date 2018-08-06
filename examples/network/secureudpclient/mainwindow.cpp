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

#include <QtCore>
#include <QtNetwork>

#include "addressdialog.h"
#include "association.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <utility>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      nameTemplate(QStringLiteral("Alice (clone number %1)"))
{
    ui->setupUi(this);
    updateUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//! [0]

const QString colorizer(QStringLiteral("<font color=\"%1\">%2</font><br>"));

void MainWindow::addErrorMessage(const QString &message)
{
    ui->clientMessages->insertHtml(colorizer.arg(QStringLiteral("Crimson"), message));
}

void MainWindow::addWarningMessage(const QString &message)
{
    ui->clientMessages->insertHtml(colorizer.arg(QStringLiteral("DarkOrange"), message));
}

void MainWindow::addInfoMessage(const QString &message)
{
    ui->clientMessages->insertHtml(colorizer.arg(QStringLiteral("DarkBlue"), message));
}

void MainWindow::addServerResponse(const QString &clientInfo, const QByteArray &datagram,
                                   const QByteArray &plainText)
{
    static const QString messageColor = QStringLiteral("DarkMagenta");
    static const QString formatter = QStringLiteral("<br>---------------"
                                                    "<br>%1 received a DTLS datagram:<br> %2"
                                                    "<br>As plain text:<br> %3");

    const QString html = formatter.arg(clientInfo, QString::fromUtf8(datagram.toHex(' ')),
                                       QString::fromUtf8(plainText));
    ui->serverMessages->insertHtml(colorizer.arg(messageColor, html));
}

//! [0]

void MainWindow::on_connectButton_clicked()
{
    if (lookupId != -1) {
        QHostInfo::abortHostLookup(lookupId);
        lookupId = -1;
        port = 0;
        updateUi();
        return;
    }

    AddressDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString hostName = dialog.remoteName();
    if (hostName.isEmpty())
        return addWarningMessage(tr("Host name or address required to connect"));

    port = dialog.remotePort();
    QHostAddress remoteAddress;
    if (remoteAddress.setAddress(hostName))
        return startNewConnection(remoteAddress);

    addInfoMessage(tr("Looking up the host ..."));
    lookupId = QHostInfo::lookupHost(hostName, this, SLOT(lookupFinished(QHostInfo)));
    updateUi();
}

void MainWindow::updateUi()
{
    ui->connectButton->setText(lookupId == -1 ? tr("Connect ...") : tr("Cancel lookup"));
    ui->shutdownButton->setEnabled(connections.size() != 0);
}

void MainWindow::lookupFinished(const QHostInfo &hostInfo)
{
    if (hostInfo.lookupId() != lookupId)
        return;

    lookupId = -1;
    updateUi();

    if (hostInfo.error() != QHostInfo::NoError) {
        addErrorMessage(hostInfo.errorString());
        return;
    }

    const QList<QHostAddress> foundAddresses = hostInfo.addresses();
    if (foundAddresses.empty()) {
        addWarningMessage(tr("Host not found"));
        return;
    }

    const auto remoteAddress = foundAddresses.at(0);
    addInfoMessage(tr("Connecting to: %1").arg(remoteAddress.toString()));
    startNewConnection(remoteAddress);
}

void MainWindow::startNewConnection(const QHostAddress &address)
{
    AssocPtr newConnection(new DtlsAssociation(address, port, nameTemplate.arg(nextId)));
    connect(newConnection.data(), &DtlsAssociation::errorMessage, this, &MainWindow::addErrorMessage);
    connect(newConnection.data(), &DtlsAssociation::warningMessage, this, &MainWindow::addWarningMessage);
    connect(newConnection.data(), &DtlsAssociation::infoMessage, this, &MainWindow::addInfoMessage);
    connect(newConnection.data(), &DtlsAssociation::serverResponse, this, &MainWindow::addServerResponse);
    connections.push_back(std::move(newConnection));
    connections.back()->startHandshake();
    updateUi();

    ++nextId;
}

void MainWindow::on_shutdownButton_clicked()
{
    connections.clear();
    updateUi();
}
