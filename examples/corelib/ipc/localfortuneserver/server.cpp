// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "server.h"

#include <QtWidgets>
#include <QtNetwork>

Server::Server(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    server = new QLocalServer(this);
    if (!server->listen("fortune")) {
        QMessageBox::critical(this, tr("Local Fortune Server"),
                              tr("Unable to start the server: %1.")
                              .arg(server->errorString()));
        close();
        return;
    }

    QLabel *statusLabel = new QLabel;
    statusLabel->setWordWrap(true);
    statusLabel->setText(tr("The server is running.\n"
                            "Run the Local Fortune Client example now."));

    fortunes << tr("You've been leading a dog's life. Stay off the furniture.")
             << tr("You've got to think about tomorrow.")
             << tr("You will be surprised by a loud noise.")
             << tr("You will feel hungry again in another hour.")
             << tr("You might have mail.")
             << tr("You cannot kill time without injuring eternity.")
             << tr("Computers are not intelligent. They only think they are.");

    QPushButton *quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);
    connect(quitButton, &QPushButton::clicked, this, &Server::close);
    connect(server, &QLocalServer::newConnection, this, &Server::sendFortune);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);

    setWindowTitle(QGuiApplication::applicationDisplayName());
}

void Server::sendFortune()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    const int fortuneIndex = QRandomGenerator::global()->bounded(0, fortunes.size());
    const QString &message = fortunes.at(fortuneIndex);
    out << quint32(message.size());
    out << message;

    QLocalSocket *clientConnection = server->nextPendingConnection();
    connect(clientConnection, &QLocalSocket::disconnected,
            clientConnection, &QLocalSocket::deleteLater);

    clientConnection->write(block);
    clientConnection->flush();
    clientConnection->disconnectFromServer();
}
