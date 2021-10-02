/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
