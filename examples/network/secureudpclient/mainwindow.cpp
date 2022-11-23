// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    lookupId = QHostInfo::lookupHost(hostName, this, &MainWindow::lookupFinished);
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
