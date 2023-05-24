// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_CTFLIB_H
#define QT_CTFLIB_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
//

#include <private/qctf_p.h>
#include "qctfplugin_p.h"
#include <qstring.h>
#include <qmutex.h>
#include <qelapsedtimer.h>
#include <qhash.h>
#include <qset.h>
#include <qthreadstorage.h>
#include <qthread.h>
#include <qloggingcategory.h>
#include "qctfserver_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcDebugTrace)

struct QCtfTracePointPrivate
{
    QString metadata;
    quint32 id = 0;
    quint32 payloadSize = 0;
    bool metadataWritten = false;
};

class QCtfLibImpl : public QCtfLib, public QCtfServer::ServerCallback
{
    struct Session
    {
        QString name;
        QStringList tracepoints;
        bool all = false;
    };
    struct Channel
    {
        char channelName[512];
        QByteArray data;
        quint64 minTimestamp = 0;
        quint64 maxTimestamp = 0;
        quint64 seqnumber = 0;
        QThread *thread = nullptr;
        quint32 threadIndex = 0;
        QByteArray threadName;
        quint32 threadNameLength = 0;
        bool locked = false;
        QCtfLibImpl *impl = nullptr;
        Channel()
        {
            memset(channelName, 0, sizeof(channelName));
        }

        ~Channel();
    };

public:
    QCtfLibImpl();
    ~QCtfLibImpl();

    bool tracepointEnabled(const QCtfTracePointEvent &point) override;
    void doTracepoint(const QCtfTracePointEvent &point, const QByteArray &arr) override;
    bool sessionEnabled() override;
    QCtfTracePointPrivate *initializeTracepoint(const QCtfTracePointEvent &point) override;
    void registerMetadata(const QCtfTraceMetadata &metadata);
    int eventId();
    void shutdown(bool *) override
    {

    }

    static QCtfLib *instance();
    static void cleanup();
private:
    static QCtfLibImpl *s_instance;
    QHash<QString, QCtfTracePointPrivate *> m_eventPrivs;
    void removeChannel(Channel *ch);
    void updateMetadata(const QCtfTracePointEvent &point);
    void writeMetadata(const QString &metadata, bool overwrite = false);
    void clearLocation();
    void handleSessionChange() override;
    void handleStatusChange(QCtfServer::ServerStatus status) override;
    void writeCtfPacket(Channel &ch);
    void buildMetadata();

    static constexpr QUuid s_TraceUuid = QUuid(0x3e589c95, 0xed11, 0xc159, 0x42, 0x02, 0x6a, 0x9b, 0x02, 0x00, 0x12, 0xac);
    static constexpr quint32 s_CtfHeaderMagic = 0xC1FC1FC1;

    QMutex m_mutex;
    QElapsedTimer m_timer;
    QString m_metadata;
    QString m_location;
    Session m_session;
    QHash<QThread*, quint32> m_threadIndices;
    QThreadStorage<Channel> m_threadData;
    QList<Channel *> m_channels;
    QHash<QString, const QCtfTraceMetadata *> m_additionalMetadata;
    QSet<QString> m_newAdditionalMetadata;
    QDateTime m_datetime;
    int m_eventId = 0;
    bool m_streaming = false;
    std::atomic_bool m_sessionChanged = false;
    std::atomic_bool m_serverClosed = false;
    QScopedPointer<QCtfServer> m_server;
    friend struct Channel;
};

QT_END_NAMESPACE

#endif
