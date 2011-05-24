/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwssignalhandler_p.h"

#ifndef QT_NO_QWS_SIGNALHANDLER

#include <sys/types.h>
#ifndef QT_NO_QWS_MULTIPROCESS
#  include <sys/ipc.h>
#  include <sys/sem.h>

#  include <private/qcore_unix_p.h>
#endif
#include <signal.h>

QT_BEGIN_NAMESPACE

class QWSSignalHandlerPrivate : public QWSSignalHandler
{
public:
    QWSSignalHandlerPrivate() : QWSSignalHandler() {}
};


Q_GLOBAL_STATIC(QWSSignalHandlerPrivate, signalHandlerInstance);


QWSSignalHandler* QWSSignalHandler::instance()
{
    return signalHandlerInstance();
}

QWSSignalHandler::QWSSignalHandler()
{
    const int signums[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE,
                            SIGSEGV, SIGTERM, SIGBUS };
    const int n = sizeof(signums)/sizeof(int);

    for (int i = 0; i < n; ++i) {
        const int signum = signums[i];
        qt_sighandler_t old = signal(signum, handleSignal);
        if (old == SIG_IGN) // don't remove shm and semaphores when ignored
            signal(signum, old);
        else
            oldHandlers[signum] = (old == SIG_ERR ? SIG_DFL : old);
    }
}

QWSSignalHandler::~QWSSignalHandler()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    while (!semaphores.isEmpty())
        removeSemaphore(semaphores.last());
#endif
}

#ifndef QT_NO_QWS_MULTIPROCESS
void QWSSignalHandler::removeSemaphore(int semno)
{
    const int index = semaphores.lastIndexOf(semno);
    if (index != -1) {
        qt_semun semval;
        semval.val = 0;
        semctl(semaphores.at(index), 0, IPC_RMID, semval);
        semaphores.remove(index);
    }
}
#endif // QT_NO_QWS_MULTIPROCESS

void QWSSignalHandler::handleSignal(int signum)
{
    QWSSignalHandler *h = instance();

    signal(signum, h->oldHandlers[signum]);

#ifndef QT_NO_QWS_MULTIPROCESS
    qt_semun semval;
    semval.val = 0;
    for (int i = 0; i < h->semaphores.size(); ++i)
        semctl(h->semaphores.at(i), 0, IPC_RMID, semval);
#endif

    h->objects.clear();
    raise(signum);
}

QT_END_NAMESPACE

#endif // QT_QWS_NO_SIGNALHANDLER
