/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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

#include <qbytearray.h>
#include <qobject.h>
#include <qtimer.h>
#include <private/qringbuffer_p.h>

#include <qt_windows.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QWinOverlappedIoNotifier;

class Q_CORE_EXPORT QWindowsPipeReader : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeReader(QObject *parent = 0);
    ~QWindowsPipeReader();

    void setHandle(HANDLE hPipeReadEnd);
    void stop();

    void setMaxReadBufferSize(qint64 size) { readBufferMaxSize = size; }
    qint64 maxReadBufferSize() const { return readBufferMaxSize; }

    bool isPipeClosed() const { return pipeBroken; }
    qint64 bytesAvailable() const;
    qint64 read(char *data, qint64 maxlen);
    bool canReadLine() const;
    bool waitForReadyRead(int msecs);
    bool waitForPipeClosed(int msecs);

    void startAsyncRead();
    bool isReadOperationActive() const { return readSequenceStarted; }

Q_SIGNALS:
    void winError(ulong, const QString &);
    void readyRead();
    void pipeClosed();

private Q_SLOTS:
    void notified(DWORD numberOfBytesRead, DWORD errorCode);

private:
    bool completeAsyncRead(DWORD bytesRead, DWORD errorCode);
    DWORD checkPipeState();

private:
    HANDLE handle;
    OVERLAPPED overlapped;
    QWinOverlappedIoNotifier *dataReadNotifier;
    qint64 readBufferMaxSize;
    QRingBuffer readBuffer;
    int actualReadBufferSize;
    bool readSequenceStarted;
    QTimer *emitReadyReadTimer;
    bool pipeBroken;
    bool readyReadEmitted;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWINDOWSPIPEREADER_P_H
