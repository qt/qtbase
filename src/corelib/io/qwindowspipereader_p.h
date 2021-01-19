/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
#include <private/qringbuffer_p.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWindowsPipeReader : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeReader(QObject *parent = 0);
    ~QWindowsPipeReader();

    void setHandle(HANDLE hPipeReadEnd);
    void startAsyncRead();
    void stop();

    void setMaxReadBufferSize(qint64 size) { readBufferMaxSize = size; }
    qint64 maxReadBufferSize() const { return readBufferMaxSize; }

    bool isPipeClosed() const { return pipeBroken; }
    qint64 bytesAvailable() const;
    qint64 read(char *data, qint64 maxlen);
    bool canReadLine() const;
    bool waitForReadyRead(int msecs);
    bool waitForPipeClosed(int msecs);

    bool isReadOperationActive() const { return readSequenceStarted; }

Q_SIGNALS:
    void winError(ulong, const QString &);
    void readyRead();
    void pipeClosed();
    void _q_queueReadyRead(QPrivateSignal);

private:
    static void CALLBACK readFileCompleted(DWORD errorCode, DWORD numberOfBytesTransfered,
                                           OVERLAPPED *overlappedBase);
    void notified(DWORD errorCode, DWORD numberOfBytesRead);
    DWORD checkPipeState();
    bool waitForNotification(int timeout);
    void emitPendingReadyRead();

    class Overlapped : public OVERLAPPED
    {
        Q_DISABLE_COPY_MOVE(Overlapped)
    public:
        explicit Overlapped(QWindowsPipeReader *reader);
        void clear();
        QWindowsPipeReader *pipeReader;
    };

    HANDLE handle;
    Overlapped overlapped;
    qint64 readBufferMaxSize;
    QRingBuffer readBuffer;
    qint64 actualReadBufferSize;
    bool stopped;
    bool readSequenceStarted;
    bool notifiedCalled;
    bool pipeBroken;
    bool readyReadPending;
    bool inReadyRead;
};

QT_END_NAMESPACE

#endif // QWINDOWSPIPEREADER_P_H
