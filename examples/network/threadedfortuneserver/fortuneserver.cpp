// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fortuneserver.h"
#include "fortunethread.h"

#include <QRandomGenerator>

#include <stdlib.h>

//! [0]
FortuneServer::FortuneServer(QObject *parent)
    : QTcpServer(parent)
{
    fortunes << tr("You've been leading a dog's life. Stay off the furniture.")
             << tr("You've got to think about tomorrow.")
             << tr("You will be surprised by a loud noise.")
             << tr("You will feel hungry again in another hour.")
             << tr("You might have mail.")
             << tr("You cannot kill time without injuring eternity.")
             << tr("Computers are not intelligent. They only think they are.");
}
//! [0]

//! [1]
void FortuneServer::incomingConnection(qintptr socketDescriptor)
{
    QString fortune = fortunes.at(QRandomGenerator::global()->bounded(fortunes.size()));
    FortuneThread *thread = new FortuneThread(socketDescriptor, fortune, this);
    connect(thread, &FortuneThread::finished, thread, &FortuneThread::deleteLater);
    thread->start();
}
//! [1]
