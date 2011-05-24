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

#if !defined(QT_NO_QWS_KEYBOARD) && !defined(QT_NO_QWS_KBD_INTEGRITY)

#include "qkbdintegrity_qws.h"
#include <qwindowsystem_qws.h>
#include <qapplication.h>
#include <qtimer.h>
#include <qthread.h>

#include <INTEGRITY.h>


//===========================================================================

QT_BEGIN_NAMESPACE

//
// INTEGRITY keyboard
//

class QIntKeyboardListenThread;

class QWSIntKbPrivate : public QObject
{
    Q_OBJECT
    friend class QIntKeyboardListenThread;
public:
    QWSIntKbPrivate(QWSKeyboardHandler *, const QString &device);
    ~QWSIntKbPrivate();
    void dataReady(int amount) { emit kbdDataAvailable(amount); }
    uint8_t scancodebuf[32 /* USB_SCANCODE_BUF_LEN */ ];
    uint8_t rxpost;
    uint8_t rxack;

Q_SIGNALS:
    void kbdDataAvailable(int amount);

private Q_SLOTS:
    void readKeyboardData(int amount);

private:
    QWSKeyboardHandler *handler;
    QIntKeyboardListenThread *kbdthread;
};
class QIntKeyboardListenThread : public QThread
{
protected:
    QWSIntKbPrivate *imp;
    bool loop;
public:
    QIntKeyboardListenThread(QWSIntKbPrivate *im) : QThread(), imp(im) {};
    ~QIntKeyboardListenThread() {};
    void run();
    void stoploop() { loop = false; };
};


QWSIntKeyboardHandler::QWSIntKeyboardHandler(const QString &device)
    : QWSKeyboardHandler(device)
{
    d = new QWSIntKbPrivate(this, device);
}

QWSIntKeyboardHandler::~QWSIntKeyboardHandler()
{
    delete d;
}

//void QWSIntKeyboardHandler::processKeyEvent(int keycode, bool isPress,
//                                            bool autoRepeat)
//{
//    QWSKeyboardHandler::processKeyEvent(keycode, isPress, autoRepeat);
//}

void QIntKeyboardListenThread::run(void)
{
    Error E;
    Buffer b;
    Connection kbdc;
    bool waitforresource = true;
    do {
        E = RequestResource((Object*)&kbdc,
                "USBKeyboardClient", "!systempassword");
        if (E == Success) {
            loop = false;
        } else {
            E = RequestResource((Object*)&kbdc,
                    "KeyboardClient", "!systempassword");
            if (E == Success) {
                waitforresource = false;
            }
        }
        if (waitforresource)
            ::sleep(1);
    } while (loop && waitforresource);
    if (!loop)
        return;
    b.BufferType = DataBuffer | LastBuffer;
    b.Length = sizeof(imp->scancodebuf);
    b.TheAddress = (Address)imp->scancodebuf;
    do {
        b.Transferred = 0;
        b.TheAddress = (Address)imp->scancodebuf + imp->rxpost;
        CheckSuccess(SynchronousReceive(kbdc, &b));
        imp->rxpost += b.Transferred;
        if (imp->rxpost >= 32 /* USB_SCANCODE_BUF_LEN */)
            imp->rxpost = 0;
        if (imp->rxpost == (imp->rxack + b.Transferred) % 32 /* USB_SCANCODE_BUF_LEN */) {
            imp->kbdDataAvailable(b.Transferred);
        }
    } while (loop);
}

void QWSIntKbPrivate::readKeyboardData(int amount)
{
    uint16_t keycode;
    do {
        if (scancodebuf[rxack] == 0xe0) {
            keycode = scancodebuf[rxack] << 8;
            rxack++;
            if (rxack >= 32 /* USB_SCANCODE_BUF_LEN */)
                rxack = 0;
        } else {
            keycode = 0;
        }

        handler->processKeycode(keycode + (scancodebuf[rxack] & 0x7f),
                (scancodebuf[rxack] & 0x80) == 0,
                scancodebuf[rxack] == 2);
        rxack++;
        if (rxack >= 32 /* USB_SCANCODE_BUF_LEN */)
            rxack = 0;
    } while (rxack != rxpost);
}

QWSIntKbPrivate::QWSIntKbPrivate(QWSKeyboardHandler *h, const QString &device) : handler(h)
{
    connect(this, SIGNAL(kbdDataAvailable(int)), this, SLOT(readKeyboardData(int)));
    this->handler = handler;
    rxack = rxpost = 0;
    kbdthread = new QIntKeyboardListenThread(this);
    kbdthread->start();
}

QWSIntKbPrivate::~QWSIntKbPrivate()
{
    kbdthread->stoploop();
    kbdthread->wait();
    delete kbdthread;
}


QT_END_NAMESPACE

#include "qkbdintegrity_qws.moc"

#endif // QT_NO_QWS_KEYBOARD || QT_NO_QWS_KBD_TTY
