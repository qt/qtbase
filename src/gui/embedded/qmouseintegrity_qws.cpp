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

#ifndef QT_NO_QWS_MOUSE_INTEGRITY

#include "qmouseintegrity_qws.h"
#include <qwindowsystem_qws.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qthread.h>

#include <INTEGRITY.h>


typedef Address MOUSEHandler;
typedef struct MOUSEMessageStruct
{
    Value x;
    Value y;
    Value z;
    Value buttons;
} MOUSEMessage;

static Error MOUSE_Init(MOUSEHandler *handler, Boolean *isabsolute);
static Error MOUSE_SynchronousGetPosition(MOUSEHandler handler, MOUSEMessage *msg,
                                          Boolean absolute);
static Error MOUSE_ShouldFilter(MOUSEHandler handler, Boolean *filter);

QT_BEGIN_NAMESPACE

class QIntMouseListenThread;

class QIntMousePrivate : public QObject
{
    Q_OBJECT
    friend class QIntMouseListenTaskThread;
Q_SIGNALS:
    void mouseDataAvailable(int x, int y, int buttons);
public:
    QIntMousePrivate(QIntMouseHandler *handler);
    ~QIntMousePrivate();
    void dataReady(int x, int y, int buttons) { emit mouseDataAvailable(x, y, buttons); }
    bool calibrated;
    bool waitforread;
    bool suspended;
    QIntMouseListenThread *mousethread;

private:
    QIntMouseHandler *handler;
};

class QIntMouseListenThread : public QThread
{
protected:
    QIntMousePrivate *imp;
    bool loop;
public:
    QIntMouseListenThread(QIntMousePrivate *im) : QThread(), imp(im) {};
    ~QIntMouseListenThread() {};
    void run();
    void stoploop() { loop = false; };
};


QIntMouseHandler::QIntMouseHandler(const QString &driver, const QString &device)
    : QObject(), QWSCalibratedMouseHandler(driver, device)
{
    QPoint test(1,1);
    d = new QIntMousePrivate(this);
    connect(d, SIGNAL(mouseDataAvailable(int, int, int)), this, SLOT(readMouseData(int, int, int)));

    d->calibrated = (test != transform(test));

    d->mousethread->start();
}

QIntMouseHandler::~QIntMouseHandler()
{
    disconnect(d, SIGNAL(mouseDataAvailable(int, int, int)), this, SLOT(readMouseData(int, int, int)));
    delete d;
}

void QIntMouseHandler::resume()
{
    d->suspended = true;
}

void QIntMouseHandler::suspend()
{
    d->suspended = false;
}

void QIntMouseHandler::readMouseData(int x, int y, int buttons)
{
    d->waitforread = false;
    if (d->suspended)
        return;
    if (d->calibrated) {
        sendFiltered(QPoint(x, y), buttons);
    } else {
        QPoint pos;
        pos = transform(QPoint(x, y));
        limitToScreen(pos);
        mouseChanged(pos, buttons, 0);
    }
}

void QIntMouseHandler::clearCalibration()
{
    QWSCalibratedMouseHandler::clearCalibration();
}

void QIntMouseHandler::calibrate(const QWSPointerCalibrationData *data)
{
    QWSCalibratedMouseHandler::calibrate(data);
}

void QIntMouseListenThread::run(void)
{
    MOUSEHandler handler;
    MOUSEMessage msg;
    Boolean filter;
    Boolean isabsolute;
    loop = true;
    CheckSuccess(MOUSE_Init(&handler, &isabsolute));
    CheckSuccess(MOUSE_ShouldFilter(handler, &filter));
    if (!filter)
        imp->calibrated = false;
    imp->waitforread = false;
    do {
        MOUSE_SynchronousGetPosition(handler, &msg, isabsolute);
        imp->dataReady(msg.x, msg.y, msg.buttons);
    } while (loop);
    QThread::exit(0);
}

QIntMousePrivate::QIntMousePrivate(QIntMouseHandler *handler)
    : QObject()
{
    this->handler = handler;
    suspended = false;
    mousethread = new QIntMouseListenThread(this);
}

QIntMousePrivate::~QIntMousePrivate()
{
    mousethread->stoploop();
    mousethread->wait();
    delete mousethread;
}

QT_END_NAMESPACE

#include "qmouseintegrity_qws.moc"

typedef struct USBMouseStruct
{
    Connection mouseconn;
    Buffer mousemsg[2];
    Value x;
    Value y;
} USBMouse;

USBMouse mousedev;

Error MOUSE_Init(MOUSEHandler *handler, Boolean *isabsolute)
{
    Error E;
    bool loop = true;
    memset((void*)&mousedev, 0, sizeof(USBMouse));
    mousedev.mousemsg[0].BufferType = DataImmediate;
    mousedev.mousemsg[1].BufferType = DataImmediate | LastBuffer;
    do {
        E = RequestResource((Object*)&mousedev.mouseconn,
                "MouseClient", "!systempassword");
        if (E == Success) {
            *isabsolute = true;
            loop = false;
        } else {
            E = RequestResource((Object*)&mousedev.mouseconn,
                    "USBMouseClient", "!systempassword");
            if (E == Success) {
                *isabsolute = false;
                loop = false;
            }
        }
        if (loop)
            sleep(1);
    } while (loop);
    *handler = (MOUSEHandler)&mousedev;
    return Success;
}

Error MOUSE_SynchronousGetPosition(MOUSEHandler handler, MOUSEMessage *msg,
        Boolean isabsolute)
{
    signed long x;
    signed long y;
    USBMouse *mdev = (USBMouse *)handler;
    mdev->mousemsg[0].Transferred = 0;
    mdev->mousemsg[1].Transferred = 0;
    SynchronousReceive(mdev->mouseconn, mdev->mousemsg);
    if (isabsolute) {
        x = (signed long)mdev->mousemsg[0].Length;
        y = (signed long)mdev->mousemsg[1].TheAddress;
    } else {
        x = mdev->x + (signed long)mdev->mousemsg[0].Length;
        y = mdev->y + (signed long)mdev->mousemsg[1].TheAddress;
    }
    if (x < 0)
        mdev->x = 0;
    else
        mdev->x = x;
    if (y < 0)
        mdev->y = 0;
    else
        mdev->y = y;
    msg->x = mdev->x;
    msg->y = mdev->y;
    msg->buttons = mdev->mousemsg[0].TheAddress;
    return Success;
}

Error MOUSE_ShouldFilter(MOUSEHandler handler, Boolean *filter)
{
    if (filter == NULL)
        return Failure;
    *filter = false;
    return Success;
}

#endif // QT_NO_QWS_MOUSE_INTEGRITY

