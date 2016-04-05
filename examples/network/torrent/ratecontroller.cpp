/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "peerwireclient.h"
#include "ratecontroller.h"

#include <QtCore>

Q_GLOBAL_STATIC(RateController, rateController)

RateController *RateController::instance()
{
    return rateController();
}

void RateController::addSocket(PeerWireClient *socket)
{
    connect(socket, SIGNAL(readyToTransfer()), this, SLOT(scheduleTransfer()));
    socket->setReadBufferSize(downLimit * 4);
    sockets << socket;
    scheduleTransfer();
}

void RateController::removeSocket(PeerWireClient *socket)
{
    disconnect(socket, SIGNAL(readyToTransfer()), this, SLOT(scheduleTransfer()));
    socket->setReadBufferSize(0);
    sockets.remove(socket);
}

void RateController::setDownloadLimit(int bytesPerSecond)
{
    downLimit = bytesPerSecond;
    foreach (PeerWireClient *socket, sockets)
        socket->setReadBufferSize(downLimit * 4);
}

void RateController::scheduleTransfer()
{
    if (transferScheduled)
        return;
    transferScheduled = true;
    QTimer::singleShot(50, this, SLOT(transfer()));
}

void RateController::transfer()
{
    transferScheduled = false;
    if (sockets.isEmpty())
        return;

    int msecs = 1000;
    if (!stopWatch.isNull())
        msecs = qMin(msecs, stopWatch.elapsed());

    qint64 bytesToWrite = (upLimit * msecs) / 1000;
    qint64 bytesToRead = (downLimit * msecs) / 1000;
    if (bytesToWrite == 0 && bytesToRead == 0) {
        scheduleTransfer();
        return;
    }

    QSet<PeerWireClient *> pendingSockets;
    foreach (PeerWireClient *client, sockets) {
        if (client->canTransferMore())
            pendingSockets << client;
    }
    if (pendingSockets.isEmpty())
        return;

    stopWatch.start();

    bool canTransferMore;
    do {
        canTransferMore = false;
        qint64 writeChunk = qMax<qint64>(1, bytesToWrite / pendingSockets.size());
        qint64 readChunk = qMax<qint64>(1, bytesToRead / pendingSockets.size());

        QSetIterator<PeerWireClient *> it(pendingSockets);
        while (it.hasNext() && (bytesToWrite > 0 || bytesToRead > 0)) {
            PeerWireClient *socket = it.next();
            if (socket->state() != QAbstractSocket::ConnectedState) {
                pendingSockets.remove(socket);
                continue;
            }

            bool dataTransferred = false;
            qint64 available = qMin<qint64>(socket->socketBytesAvailable(), readChunk);
            if (available > 0) {
                qint64 readBytes = socket->readFromSocket(qMin<qint64>(available, bytesToRead));
                if (readBytes > 0) {
                    bytesToRead -= readBytes;
                    dataTransferred = true;
                }
            }

            if (upLimit * 2 > socket->bytesToWrite()) {
                qint64 chunkSize = qMin<qint64>(writeChunk, bytesToWrite);
                qint64 toWrite = qMin(upLimit * 2 - socket->bytesToWrite(), chunkSize);
                if (toWrite > 0) {
                    qint64 writtenBytes = socket->writeToSocket(toWrite);
                    if (writtenBytes > 0) {
                        bytesToWrite -= writtenBytes;
                        dataTransferred = true;
                    }
                }
            }

            if (dataTransferred && socket->canTransferMore())
                canTransferMore = true;
            else
                pendingSockets.remove(socket);
        }
    } while (canTransferMore && (bytesToWrite > 0 || bytesToRead > 0) && !pendingSockets.isEmpty());

    if (canTransferMore || bytesToWrite == 0 || bytesToRead == 0)
        scheduleTransfer();
}
