/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        Q_DISABLE_COPY(Overlapped)
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
