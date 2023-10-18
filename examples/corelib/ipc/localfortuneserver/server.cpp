// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "server.h"

#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>

using namespace Qt::StringLiterals;

static const QString idleStateText = QObject::tr("Press \"Listen\" to start the server");

Server::Server(QWidget *parent)
    : QDialog(parent),
      server(new QLocalServer(this)),
      hostLineEdit(new QLineEdit(u"fortune"_s)),
      statusLabel(new QLabel(idleStateText)),
      listenButton(new QPushButton(tr("Listen"))),
      stopListeningButton(new QPushButton(tr("Stop Listening")))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    statusLabel->setWordWrap(true);

    stopListeningButton->setDisabled(true);

    fortunes << tr("You've been leading a dog's life. Stay off the furniture.")
             << tr("You've got to think about tomorrow.")
             << tr("You will be surprised by a loud noise.")
             << tr("You will feel hungry again in another hour.")
             << tr("You might have mail.")
             << tr("You cannot kill time without injuring eternity.")
             << tr("Computers are not intelligent. They only think they are.");

    QLabel *hostLabel = new QLabel(tr("Server name:"));

    connect(server, &QLocalServer::newConnection, this, &Server::sendFortune);
    connect(hostLineEdit, &QLineEdit::textChanged, this, &Server::toggleListenButton);
    connect(listenButton, &QPushButton::clicked, this, &Server::listenToServer);
    connect(stopListeningButton, &QPushButton::clicked,this, &Server::stopListening);

    QPushButton *quitButton = new QPushButton(tr("Quit"));
    quitButton->setAutoDefault(false);
    connect(quitButton, &QPushButton::clicked, this, &Server::close);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(listenButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(stopListeningButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostLineEdit, 0, 1);
    mainLayout->addWidget(statusLabel, 2, 0, 3, 2);
    mainLayout->addWidget(buttonBox, 10, 0, 2, 2);

    setWindowTitle(QGuiApplication::applicationDisplayName());
    hostLineEdit->setFocus();
}

void Server::listenToServer()
{
    name = hostLineEdit->text();
    if (!server->listen(name)) {
        QMessageBox::critical(this, tr("Local Fortune Server"),
                              tr("Unable to start the server: %1.")
                                      .arg(server->errorString()));
        name.clear();
        return;
    }
    statusLabel->setText(tr("The server is running.\n"
                            "Run the Local Fortune Client example now."));
    toggleListenButton();
}

void Server::stopListening()
{
    server->close();
    name.clear();
    statusLabel->setText(idleStateText);
    toggleListenButton();
}

void Server::toggleListenButton()
{
    if (server->isListening()) {
        listenButton->setDisabled(true);
        stopListeningButton->setEnabled(true);
    } else {
        listenButton->setEnabled(!hostLineEdit->text().isEmpty());
        stopListeningButton->setDisabled(true);
    }
}

void Server::sendFortune()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
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
