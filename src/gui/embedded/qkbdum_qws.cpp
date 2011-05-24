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

#include "qkbdum_qws.h"
#include "qvfbhdr.h"

#if !defined(QT_NO_QWS_KEYBOARD) && !defined(QT_NO_QWS_KBD_UM)

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <qstring.h>
#include <qwindowsystem_qws.h>
#include <qsocketnotifier.h>
#include "qplatformdefs.h"

QT_BEGIN_NAMESPACE

class QWSUmKeyboardHandlerPrivate : public QObject
{
    Q_OBJECT

public:
    QWSUmKeyboardHandlerPrivate(const QString&);
    ~QWSUmKeyboardHandlerPrivate();

private slots:
    void readKeyboardData();

private:
    int kbdFD;
    int kbdIdx;
    const int kbdBufferLen;
    unsigned char *kbdBuffer;
    QSocketNotifier *notifier;
};

QWSUmKeyboardHandlerPrivate::QWSUmKeyboardHandlerPrivate(const QString &device)
    : kbdFD(-1), kbdIdx(0), kbdBufferLen(sizeof(QVFbKeyData)*5)
{
    kbdBuffer = new unsigned char [kbdBufferLen];

    if ((kbdFD = QT_OPEN((const char *)device.toLocal8Bit(), O_RDONLY | O_NDELAY, 0)) < 0) {
        qDebug("Cannot open %s (%s)", (const char *)device.toLocal8Bit(),
        strerror(errno));
    } else {
        // Clear pending input
        char buf[2];
        while (QT_READ(kbdFD, buf, 1) > 0) { }

        notifier = new QSocketNotifier(kbdFD, QSocketNotifier::Read, this);
        connect(notifier, SIGNAL(activated(int)),this, SLOT(readKeyboardData()));
    }
}

QWSUmKeyboardHandlerPrivate::~QWSUmKeyboardHandlerPrivate()
{
    if (kbdFD >= 0)
        QT_CLOSE(kbdFD);
    delete [] kbdBuffer;
}


void QWSUmKeyboardHandlerPrivate::readKeyboardData()
{
    int n;
    do {
        n  = QT_READ(kbdFD, kbdBuffer+kbdIdx, kbdBufferLen - kbdIdx);
        if (n > 0)
            kbdIdx += n;
    } while (n > 0);

    int idx = 0;
    while (kbdIdx - idx >= (int)sizeof(QVFbKeyData)) {
        QVFbKeyData *kd = (QVFbKeyData *)(kbdBuffer + idx);
        // Qtopia Key filters must still work.
        QWSServer::processKeyEvent(kd->unicode, kd->keycode, kd->modifiers, kd->press, kd->repeat);
        idx += sizeof(QVFbKeyData);
    }

    int surplus = kbdIdx - idx;
    for (int i = 0; i < surplus; i++)
        kbdBuffer[i] = kbdBuffer[idx+i];
    kbdIdx = surplus;
}

QWSUmKeyboardHandler::QWSUmKeyboardHandler(const QString &device)
    : QWSKeyboardHandler()
{
    d = new QWSUmKeyboardHandlerPrivate(device);
}

QWSUmKeyboardHandler::~QWSUmKeyboardHandler()
{
    delete d;
}

QT_END_NAMESPACE

#include "qkbdum_qws.moc"

#endif // QT_NO_QWS_KEYBOARD && QT_NO_QWS_KBD_UM
