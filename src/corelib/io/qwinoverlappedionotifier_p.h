/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINOVERLAPPEDIONOTIFIER_P_H
#define QWINOVERLAPPEDIONOTIFIER_P_H

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
#include <qt_windows.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QWinIoCompletionPort;

class Q_CORE_EXPORT QWinOverlappedIoNotifier : public QObject
{
    Q_OBJECT
public:
    QWinOverlappedIoNotifier(QObject *parent = 0);
    ~QWinOverlappedIoNotifier();

    void setHandle(HANDLE h);
    HANDLE handle() const { return hHandle; }

    void setEnabled(bool enabled);
    bool waitForNotified(int msecs, OVERLAPPED *overlapped);

Q_SIGNALS:
    void notified(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED *overlapped);
    void _q_notify();

private Q_SLOTS:
    OVERLAPPED *_q_notified();

private:
    void notify(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED *overlapped);

private:
    static QWinIoCompletionPort *iocp;
    static HANDLE iocpInstanceLock;
    static unsigned int iocpInstanceRefCount;
    HANDLE hHandle;
    HANDLE hSemaphore;
    HANDLE hResultsMutex;

    struct IOResult
    {
        IOResult(DWORD n = 0, DWORD e = 0, OVERLAPPED *p = 0)
            : numberOfBytes(n), errorCode(e), overlapped(p)
        {}

        DWORD numberOfBytes;
        DWORD errorCode;
        OVERLAPPED *overlapped;
    };

    QQueue<IOResult> results;

    friend class QWinIoCompletionPort;
};

QT_END_NAMESPACE

#endif // QWINOVERLAPPEDIONOTIFIER_P_H
