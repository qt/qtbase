// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSPIPEWRITER_P_H
#define QWINDOWSPIPEWRITER_P_H

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

class Q_CORE_EXPORT QWindowsPipeWriter : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent = nullptr);
    ~QWindowsPipeWriter();

    void setHandle(HANDLE hPipeWriteEnd);
    void write(const QByteArray &ba);
    void write(const char *data, qint64 size);
    void stop();
    bool checkForWrite() { return consumePendingAndEmit(false); }
    qint64 bytesToWrite() const;
    bool isWriteOperationActive() const;
    HANDLE syncEvent() const { return syncHandle; }

Q_SIGNALS:
    void bytesWritten(qint64 bytes);
    void writeFailed();

protected:
    bool event(QEvent *e) override;

private:
    enum CompletionState { NoError, ErrorDetected, WriteDisabled };

    template <typename... Args>
    inline void writeImpl(Args... args);

    void startAsyncWriteHelper(QMutexLocker<QMutex> *locker);
    void startAsyncWriteLocked();
    static void CALLBACK waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                      PTP_WAIT wait, TP_WAIT_RESULT waitResult);
    bool writeCompleted(DWORD errorCode, DWORD numberOfBytesWritten);
    void notifyCompleted(QMutexLocker<QMutex> *locker);
    bool consumePendingAndEmit(bool allowWinActPosting);

    HANDLE handle;
    HANDLE eventHandle;
    HANDLE syncHandle;
    PTP_WAIT waitObject;
    OVERLAPPED overlapped;
    QRingBuffer writeBuffer;
    qint64 pendingBytesWrittenValue;
    mutable QMutex mutex;
    DWORD lastError;

    CompletionState completionState;
    bool stopped;
    bool writeSequenceStarted;
    bool bytesWrittenPending;
    bool winEventActPosted;
};

QT_END_NAMESPACE

#endif // QWINDOWSPIPEWRITER_P_H
