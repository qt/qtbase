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

#ifndef QT_NO_QWS_QVFB

#define QTOPIA_QVFB_BRIGHTNESS

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <qvfbhdr.h>
#include <qscreenvfb_qws.h>
#include <qkbdvfb_qws.h>
#include <qmousevfb_qws.h>
#include <qwindowsystem_qws.h>
#include <qsocketnotifier.h>
#include <qapplication.h>
#include <qscreen_qws.h>
#include <qmousedriverfactory_qws.h>
#include <qkbddriverfactory_qws.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QVFbScreenPrivate
{
public:
    QVFbScreenPrivate();
    ~QVFbScreenPrivate();

    bool success;
    unsigned char *shmrgn;
    int brightness;
    bool blank;
    QVFbHeader *hdr;
    QWSMouseHandler *mouse;
#ifndef QT_NO_QWS_KEYBOARD
    QWSKeyboardHandler *keyboard;
#endif
};

QVFbScreenPrivate::QVFbScreenPrivate()
    : mouse(0)

{
#ifndef QT_NO_QWS_KEYBOARD
    keyboard = 0;
#endif
    brightness = 255;
    blank = false;
}

QVFbScreenPrivate::~QVFbScreenPrivate()
{
    delete mouse;
#ifndef QT_NO_QWS_KEYBOARD
    delete keyboard;
#endif
}

/*!
    \internal

    \class QVFbScreen
    \ingroup qws

    \brief The QVFbScreen class implements a screen driver for the
    virtual framebuffer.

    Note that this class is only available in \l{Qt for Embedded Linux}.
    Custom screen drivers can be added by subclassing the
    QScreenDriverPlugin class, using the QScreenDriverFactory class to
    dynamically load the driver into the application, but there should
    only be one screen object per application.

    The Qt for Embedded Linux platform provides a \l{The Virtual
    Framebuffer}{virtual framebuffer} for development and debugging;
    the virtual framebuffer allows Qt for Embedded Linux applications to be
    developed on a desktop machine, without switching between consoles
    and X11.

    \sa QScreen, QScreenDriverPlugin, {Running Applications}
*/

/*!
    \fn bool QVFbScreen::connect(const QString & displaySpec)
    \reimp
*/

/*!
    \fn void QVFbScreen::disconnect()
    \reimp
*/

/*!
    \fn bool QVFbScreen::initDevice()
    \reimp
*/

/*!
    \fn void QVFbScreen::restore()
    \reimp
*/

/*!
    \fn void QVFbScreen::save()
    \reimp
*/

/*!
    \fn void QVFbScreen::setDirty(const QRect & r)
    \reimp
*/

/*!
    \fn void QVFbScreen::setMode(int nw, int nh, int nd)
    \reimp
*/

/*!
    \fn void QVFbScreen::shutdownDevice()
    \reimp
*/

/*!
    \fn QVFbScreen::QVFbScreen(int displayId)

    Constructs a QVNCScreen object. The \a displayId argument
    identifies the Qt for Embedded Linux server to connect to.
*/
QVFbScreen::QVFbScreen(int display_id)
    : QScreen(display_id, VFbClass), d_ptr(new QVFbScreenPrivate)
{
    d_ptr->shmrgn = 0;
    d_ptr->hdr = 0;
    data = 0;
}

/*!
    Destroys this QVFbScreen object.
*/
QVFbScreen::~QVFbScreen()
{
    delete d_ptr;
}

static QVFbScreen *connected = 0;

bool QVFbScreen::connect(const QString &displaySpec)
{
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));
    if (displayArgs.contains(QLatin1String("Gray")))
        grayscale = true;

    key_t key = ftok(QT_VFB_MOUSE_PIPE(displayId).toLocal8Bit(), 'b');

    if (key == -1)
        return false;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#ifndef QT_QWS_FRAMEBUFFER_LITTLE_ENDIAN
    if (displayArgs.contains(QLatin1String("littleendian")))
#endif
        QScreen::setFrameBufferLittleEndian(true);
#endif

    int shmId = shmget(key, 0, 0);
    if (shmId != -1)
        d_ptr->shmrgn = (unsigned char *)shmat(shmId, 0, 0);
    else
        return false;

    if ((long)d_ptr->shmrgn == -1 || d_ptr->shmrgn == 0) {
        qDebug("No shmrgn %ld", (long)d_ptr->shmrgn);
        return false;
    }

    d_ptr->hdr = (QVFbHeader *)d_ptr->shmrgn;
    data = d_ptr->shmrgn + d_ptr->hdr->dataoffset;

    dw = w = d_ptr->hdr->width;
    dh = h = d_ptr->hdr->height;
    d = d_ptr->hdr->depth;

    switch (d) {
    case 1:
        setPixelFormat(QImage::Format_Mono);
        break;
    case 8:
        setPixelFormat(QImage::Format_Indexed8);
        break;
    case 12:
        setPixelFormat(QImage::Format_RGB444);
        break;
    case 15:
        setPixelFormat(QImage::Format_RGB555);
        break;
    case 16:
        setPixelFormat(QImage::Format_RGB16);
        break;
    case 18:
        setPixelFormat(QImage::Format_RGB666);
        break;
    case 24:
        setPixelFormat(QImage::Format_RGB888);
        break;
    case 32:
        setPixelFormat(QImage::Format_ARGB32_Premultiplied);
        break;
    }

    lstep = d_ptr->hdr->linestep;

    // Handle display physical size spec.
    int dimIdxW = -1;
    int dimIdxH = -1;
    for (int i = 0; i < displayArgs.size(); ++i) {
        if (displayArgs.at(i).startsWith(QLatin1String("mmWidth"))) {
            dimIdxW = i;
            break;
        }
    }
    for (int i = 0; i < displayArgs.size(); ++i) {
        if (displayArgs.at(i).startsWith(QLatin1String("mmHeight"))) {
            dimIdxH = i;
            break;
        }
    }
    if (dimIdxW >= 0) {
        bool ok;
        int pos = 7;
        if (displayArgs.at(dimIdxW).at(pos) == QLatin1Char('='))
            ++pos;
        int pw = displayArgs.at(dimIdxW).mid(pos).toInt(&ok);
        if (ok) {
            physWidth = pw;
            if (dimIdxH < 0)
                physHeight = dh*physWidth/dw;
        }
    }
    if (dimIdxH >= 0) {
        bool ok;
        int pos = 8;
        if (displayArgs.at(dimIdxH).at(pos) == QLatin1Char('='))
            ++pos;
        int ph = displayArgs.at(dimIdxH).mid(pos).toInt(&ok);
        if (ok) {
            physHeight = ph;
            if (dimIdxW < 0)
                physWidth = dw*physHeight/dh;
        }
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        const int dpi = 72;
        physWidth = qRound(dw * 25.4 / dpi);
        physHeight = qRound(dh * 25.4 / dpi);
    }

    qDebug("Connected to VFB server %s: %d x %d x %d %dx%dmm (%dx%ddpi)", displaySpec.toLatin1().data(),
        w, h, d, physWidth, physHeight, qRound(dw*25.4/physWidth), qRound(dh*25.4/physHeight) );

    size = lstep * h;
    mapsize = size;
    screencols = d_ptr->hdr->numcols;
    memcpy(screenclut, d_ptr->hdr->clut, sizeof(QRgb) * screencols);

    connected = this;

    if (qgetenv("QT_QVFB_BGR").toInt())
        pixeltype = BGRPixel;

    return true;
}

void QVFbScreen::disconnect()
{
    connected = 0;
    if ((long)d_ptr->shmrgn != -1 && d_ptr->shmrgn) {
        if (qApp->type() == QApplication::GuiServer && d_ptr->hdr->dataoffset >= (int)sizeof(QVFbHeader)) {
            d_ptr->hdr->serverVersion = 0;
        }
        shmdt((char*)d_ptr->shmrgn);
    }
}

bool QVFbScreen::initDevice()
{
#ifndef QT_NO_QWS_MOUSE_QVFB
    const QString mouseDev = QT_VFB_MOUSE_PIPE(displayId);
    d_ptr->mouse = new QVFbMouseHandler(QLatin1String("QVFbMouse"), mouseDev);
    qwsServer->setDefaultMouse("None");
    if (d_ptr->mouse)
        d_ptr->mouse->setScreen(this);
#endif

#if !defined(QT_NO_QWS_KBD_QVFB) && !defined(QT_NO_QWS_KEYBOARD)
    const QString keyboardDev = QT_VFB_KEYBOARD_PIPE(displayId);
    d_ptr->keyboard = new QVFbKeyboardHandler(keyboardDev);
    qwsServer->setDefaultKeyboard("None");
#endif

    if (d_ptr->hdr->dataoffset >= (int)sizeof(QVFbHeader))
        d_ptr->hdr->serverVersion = QT_VERSION;

    if(d==8) {
        screencols=256;
        if (grayscale) {
            // Build grayscale palette
            for(int loopc=0;loopc<256;loopc++) {
                screenclut[loopc]=qRgb(loopc,loopc,loopc);
            }
        } else {
            // 6x6x6 216 color cube
            int idx = 0;
            for(int ir = 0x0; ir <= 0xff; ir+=0x33) {
                for(int ig = 0x0; ig <= 0xff; ig+=0x33) {
                    for(int ib = 0x0; ib <= 0xff; ib+=0x33) {
                        screenclut[idx]=qRgb(ir, ig, ib);
                        idx++;
                    }
                }
            }
            screencols=idx;
        }
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    } else if (d == 4) {
        int val = 0;
        for (int idx = 0; idx < 16; idx++, val += 17) {
            screenclut[idx] = qRgb(val, val, val);
        }
        screencols = 16;
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    } else if (d == 1) {
        screencols = 2;
        screenclut[1] = qRgb(0xff, 0xff, 0xff);
        screenclut[0] = qRgb(0, 0, 0);
        memcpy(d_ptr->hdr->clut, screenclut, sizeof(QRgb) * screencols);
        d_ptr->hdr->numcols = screencols;
    }

#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QVFbScreen::shutdownDevice()
{
}

void QVFbScreen::setMode(int ,int ,int)
{
}

// save the state of the graphics card
// This is needed so that e.g. we can restore the palette when switching
// between linux virtual consoles.
void QVFbScreen::save()
{
    // nothing to do.
}

// restore the state of the graphics card.
void QVFbScreen::restore()
{
}
void QVFbScreen::setDirty(const QRect& rect)
{
    const QRect r = rect.translated(-offset());
    d_ptr->hdr->dirty = true;
    d_ptr->hdr->update = d_ptr->hdr->update.united(r);
}

void QVFbScreen::setBrightness(int b)
{
    if (connected) {
        connected->d_ptr->brightness = b;

        QVFbHeader *hdr = connected->d_ptr->hdr;
        if (hdr->viewerVersion < 0x040400) // brightness not supported
            return;

        const int br = connected->d_ptr->blank ? 0 : b;
        if (hdr->brightness != br) {
            hdr->brightness = br;
            connected->setDirty(connected->region().boundingRect());
        }
    }
}

void QVFbScreen::blank(bool on)
{
    d_ptr->blank = on;
    setBrightness(connected->d_ptr->brightness);
}

#endif // QT_NO_QWS_QVFB

QT_END_NAMESPACE
