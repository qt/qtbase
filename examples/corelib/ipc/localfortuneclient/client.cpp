// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "client.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMessageBox>
#include <QTimer>

using namespace Qt::StringLiterals;

Client::Client(QWidget *parent)
    : QDialog(parent),
      hostLineEdit(new QLineEdit(u"fortune"_s)),
      getFortuneButton(new QPushButton(tr("Get Fortune"))),
      statusLabel(new QLabel(tr("This examples requires that you run the "
                                "Local Fortune Server example as well."))),
      socket(new QLocalSocket(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QLabel *hostLabel = new QLabel(tr("&Server name:"));
    hostLabel->setBuddy(hostLineEdit);

    statusLabel->setWordWrap(true);

    getFortuneButton->setDefault(true);
    QPushButton *quitButton = new QPushButton(tr("Quit"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(getFortuneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    in.setDevice(socket);
    in.setVersion(QDataStream::Qt_6_0);

    connect(hostLineEdit, &QLineEdit::textChanged,
            this, &Client::enableGetFortuneButton);
    connect(getFortuneButton, &QPushButton::clicked,
            this, &Client::requestNewFortune);
    connect(quitButton, &QPushButton::clicked, this, &Client::close);
    connect(socket, &QLocalSocket::readyRead, this, &Client::readFortune);
    connect(socket, &QLocalSocket::errorOccurred, this, &Client::displayError);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostLineEdit, 0, 1);
    mainLayout->addWidget(statusLabel, 2, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 2);

    setWindowTitle(QGuiApplication::applicationDisplayName());
    hostLineEdit->setFocus();
}

void Client::requestNewFortune()
{
    getFortuneButton->setEnabled(false);
    blockSize = 0;
    socket->abort();
    socket->connectToServer(hostLineEdit->text());
}

void Client::readFortune()
{
    in.startTransaction();

    QString nextFortune;
    in >> nextFortune;

    if (!in.commitTransaction())
        return;

    if (nextFortune == currentFortune) {
        QTimer::singleShot(0, this, &Client::requestNewFortune);
        return;
    }

    currentFortune = nextFortune;
    statusLabel->setText(currentFortune);
    getFortuneButton->setEnabled(true);
}

void Client::displayError(QLocalSocket::LocalSocketError socketError)
{
    switch (socketError) {
    case QLocalSocket::ServerNotFoundError:
        QMessageBox::information(this, tr("Local Fortune Client"),
                                 tr("The host was not found. Please make sure "
                                    "that the server is running and that the "
                                    "server name is correct."));
        break;
    case QLocalSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Local Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the server name "
                                    "is correct."));
        break;
    case QLocalSocket::PeerClosedError:
        break;
    default:
        QMessageBox::information(this, tr("Local Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(socket->errorString()));
    }

    getFortuneButton->setEnabled(true);
}

void Client::enableGetFortuneButton()
{
    getFortuneButton->setEnabled(!hostLineEdit->text().isEmpty());
}
