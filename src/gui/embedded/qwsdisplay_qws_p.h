/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QWSDISPLAY_QWS_P_H
#define QWSDISPLAY_QWS_P_H

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

#include "qwsdisplay_qws.h"
#include "qwssocket_qws.h"
#include "qwsevent_qws.h"
#include <private/qwssharedmemory_p.h>
#include "qwscommand_qws_p.h"
#include "qwslock_p.h"

QT_BEGIN_NAMESPACE

class QWSDisplay::Data
{
public:
    Data(QObject* parent, bool singleProcess = false);
    ~Data();

    void flush();

    bool queueNotEmpty();
    QWSEvent *dequeue();
    QWSEvent *peek();

    bool directServerConnection();
    void fillQueue();
#ifndef QT_NO_QWS_MULTIPROCESS
    void connectToPipe();
    void waitForConnection();
    void waitForPropertyReply();
    void waitForRegionAck(int winId);
    void waitForRegionEvents(int winId, bool ungrabDisplay);
    bool hasPendingRegionEvents() const;
#endif
    void waitForCreation();
#ifndef QT_NO_COP
    void waitForQCopResponse();
#endif
    void init();
    void reinit( const QString& newAppName );
    void create(int n = 1);

    void flushCommands();
    void sendCommand(QWSCommand & cmd);
    void sendSynchronousCommand(QWSCommand & cmd);

    QWSEvent *readMore();

    int takeId();

    void setMouseFilter(void (*filter)(QWSMouseEvent*));

    //####public data members

//    QWSRegionManager *rgnMan;
    uchar *sharedRam;
#ifndef QT_NO_QWS_MULTIPROCESS
    QWSSharedMemory shm;
#endif
    int sharedRamSize;

#ifndef QT_NO_QWS_MULTIPROCESS
    static QWSLock *clientLock;

    static bool lockClient(QWSLock::LockType, int timeout = -1);
    static void unlockClient(QWSLock::LockType);
    static bool waitClient(QWSLock::LockType, int timeout = -1);
    static QWSLock* getClientLock();
#endif // QT_NO_QWS_MULTIPROCESS

private:
#ifndef QT_NO_QWS_MULTIPROCESS
    QWSSocket *csocket;
#endif
    QList<QWSEvent*> queue;

#if 0
    void debugQueue() {
            for (int i = 0; i < queue.size(); ++i) {
                QWSEvent *e = queue.at(i);
                qDebug( "   ev %d type %d sl %d rl %d", i, e->type, e->simpleLen, e->rawLen);
            }
    }
#endif

    QWSConnectedEvent* connected_event;
    QWSMouseEvent* mouse_event;
    int region_events_count;
    int mouse_state;
    int mouse_winid;
    QPoint region_offset;
    int region_offset_window;
#ifndef QT_NO_COP
    QWSQCopMessageEvent *qcop_response;
#endif
    QWSEvent* current_event;
    QList<int> unused_identifiers;
#ifdef QAPPLICATION_EXTRA_DEBUG
    int mouse_event_count;
#endif
    void (*mouseFilter)(QWSMouseEvent *);

    enum { VariableEvent=-1 };

};

QT_END_NAMESPACE

#endif // QWSDISPLAY_QWS_P_H
