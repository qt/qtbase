/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef BASELINESERVER_H
#define BASELINESERVER_H

#include <QStringList>
#include <QTcpServer>
#include <QThread>
#include <QTcpSocket>
#include <QScopedPointer>
#include <QTimer>
#include <QDateTime>
#include <QSettings>

#include "baselineprotocol.h"
#include "report.h"

// #seconds between update checks
#define HEARTBEAT 10
#define MetadataFileExt "metadata"

class BaselineServer : public QTcpServer
{
    Q_OBJECT

public:
    BaselineServer(QObject *parent = 0);

    static QString storagePath();
    static QString baseUrl();
    static QString settingsFilePath();

protected:
    void incomingConnection(int socketDescriptor);

private slots:
    void heartbeat();

private:
    QTimer *heartbeatTimer;
    QDateTime meLastMod;
    QString lastRunId;
    int lastRunIdIdx;
    static QString storage;
    static QString url;
    static QString settingsFile;
};



class BaselineThread : public QThread
{
    Q_OBJECT

public:
    BaselineThread(const QString &runId, int socketDescriptor, QObject *parent);
    void run();

private:
    QString runId;
    int socketDescriptor;
};


class BaselineHandler : public QObject
{
    Q_OBJECT

public:
    BaselineHandler(const QString &runId, int socketDescriptor = -1);
    void testPathMapping();
    QString pathForItem(const ImageItem &item, bool isBaseline = true, bool absolute = true) const;

    // CGI callbacks:
    static QString view(const QString &baseline, const QString &rendered, const QString &compared);
    static QString clearAllBaselines(const QString &context);
    static QString updateBaselines(const QString &context, const QString &mismatchContext, const QString &itemFile);
    static QString blacklistTest(const QString &context, const QString &itemId, bool removeFromBlacklist = false);

private slots:
    void receiveRequest();
    void receiveDisconnect();

private:
    bool establishConnection();
    void provideBaselineChecksums(const QByteArray &itemListBlock);
    void storeImage(const QByteArray &itemBlock, bool isBaseline);
    void storeItemMetadata(const PlatformInfo &metadata, const QString &path);
    PlatformInfo fetchItemMetadata(const QString &path);
    void mapPlatformInfo() const;
    const char *logtime();
    QString computeMismatchScore(const QImage& baseline, const QImage& rendered);

    BaselineProtocol proto;
    PlatformInfo plat;
    mutable PlatformInfo mapped;
    QString runId;
    bool connectionEstablished;
    Report report;
    QSettings *settings;
};

#endif // BASELINESERVER_H
