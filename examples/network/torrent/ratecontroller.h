// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RATECONTROLLER_H
#define RATECONTROLLER_H

#include <QObject>
#include <QSet>
#include <QElapsedTimer>

class PeerWireClient;

class RateController : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    static RateController *instance();

    void addSocket(PeerWireClient *socket);
    void removeSocket(PeerWireClient *socket);

    inline int uploadLimit() const { return upLimit; }
    inline int downloadLimit() const { return downLimit; }
    inline void setUploadLimit(int bytesPerSecond) { upLimit = bytesPerSecond; }
    void setDownloadLimit(int bytesPerSecond);

public slots:
    void transfer();
    void scheduleTransfer();

private:
    QElapsedTimer stopWatch;
    QSet<PeerWireClient *> sockets;
    int upLimit = 0;
    int downLimit = 0;
    bool transferScheduled = false;
};

#endif
