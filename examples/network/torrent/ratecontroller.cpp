// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    connect(socket, &PeerWireClient::readyToTransfer,
            this, &RateController::scheduleTransfer);
    socket->setReadBufferSize(downLimit * 4);
    sockets << socket;
    scheduleTransfer();
}

void RateController::removeSocket(PeerWireClient *socket)
{
    disconnect(socket, &PeerWireClient::readyToTransfer,
               this, &RateController::scheduleTransfer);
    socket->setReadBufferSize(0);
    sockets.remove(socket);
}

void RateController::setDownloadLimit(int bytesPerSecond)
{
    downLimit = bytesPerSecond;
    for (PeerWireClient *socket : std::as_const(sockets))
        socket->setReadBufferSize(downLimit * 4);
}

void RateController::scheduleTransfer()
{
    if (transferScheduled)
        return;
    transferScheduled = true;
    QTimer::singleShot(50, this, &RateController::transfer);
}

void RateController::transfer()
{
    transferScheduled = false;
    if (sockets.isEmpty())
        return;

    qint64 msecs = 1000;
    if (stopWatch.isValid())
        msecs = qMin(msecs, stopWatch.elapsed());

    qint64 bytesToWrite = (upLimit * msecs) / 1000;
    qint64 bytesToRead = (downLimit * msecs) / 1000;
    if (bytesToWrite == 0 && bytesToRead == 0) {
        scheduleTransfer();
        return;
    }

    QSet<PeerWireClient *> pendingSockets;
    for (PeerWireClient *client : std::as_const(sockets)) {
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

        for (auto it = pendingSockets.begin(), end = pendingSockets.end(); it != end && (bytesToWrite > 0 || bytesToRead > 0); /*erasing*/) {
            auto current = it++;
            PeerWireClient *socket = *current;
            if (socket->state() != QAbstractSocket::ConnectedState) {
                it = pendingSockets.erase(current);
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
                it = pendingSockets.erase(current);
        }
    } while (canTransferMore && (bytesToWrite > 0 || bytesToRead > 0) && !pendingSockets.isEmpty());

    if (canTransferMore || bytesToWrite == 0 || bytesToRead == 0)
        scheduleTransfer();
}
