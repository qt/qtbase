// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSPIPEREADER_P_H
#define QWINDOWSPIPEREADER_P_H

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

#include <qobject.h>
#include <qmutex.h>
#include <private/qringbuffer_p.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWindowsPipeReader : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeReader(QObject *parent = nullptr);
    ~QWindowsPipeReader();

    void setHandle(HANDLE hPipeReadEnd);
    void startAsyncRead();
    void stop();
    void drainAndStop();
    void stopAndClear();

    void setMaxReadBufferSize(qint64 size);
    qint64 maxReadBufferSize() const { return readBufferMaxSize; }

    bool isPipeClosed() const { return pipeBroken; }
    qint64 bytesAvailable() const;
    qint64 read(char *data, qint64 maxlen);
    qint64 readLine(char *data, qint64 maxlen);
    qint64 skip(qint64 maxlen);
    bool canReadLine() const;
    DWORD checkPipeState();
    bool checkForReadyRead() { return consumePendingAndEmit(false); }

    bool isReadOperationActive() const;
    HANDLE syncEvent() const { return syncHandle; }

Q_SIGNALS:
    void winError(ulong, const QString &);
    void readyRead();
    void pipeClosed();

protected:
    bool event(QEvent *e) override;

private:
    enum State { Stopped, Running, Draining };

    void startAsyncReadHelper(QMutexLocker<QMutex> *locker);
    void startAsyncReadLocked();
    void cancelAsyncRead(State newState);
    static void CALLBACK waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                      PTP_WAIT wait, TP_WAIT_RESULT waitResult);
    bool readCompleted(DWORD errorCode, DWORD numberOfBytesRead);
    bool waitForNotification();
    bool consumePendingAndEmit(bool allowWinActPosting);
    bool consumePending();

    HANDLE handle;
    HANDLE eventHandle;
    HANDLE syncHandle;
    PTP_WAIT waitObject;
    OVERLAPPED overlapped;
    qint64 readBufferMaxSize;
    QRingBuffer readBuffer;
    qint64 actualReadBufferSize;
    qint64 pendingReadBytes;
    mutable QMutex mutex;
    DWORD lastError;

    State state;
    bool readSequenceStarted;
    bool pipeBroken;
    bool readyReadPending;
    bool winEventActPosted;
};

QT_END_NAMESPACE

#endif // QWINDOWSPIPEREADER_P_H
