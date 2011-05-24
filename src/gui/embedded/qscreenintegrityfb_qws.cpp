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

#ifndef QT_NO_QWS_INTEGRITYFB

#include <qscreenintegrityfb_qws.h>
#include <qwindowsystem_qws.h>
#include <qsocketnotifier.h>
#include <qapplication.h>
#include <qscreen_qws.h>
#include "qmouseintegrity_qws.h"
#include "qkbdintegrity_qws.h"
#include <qmousedriverfactory_qws.h>
#include <qkbddriverfactory_qws.h>
#include <qdebug.h>

#include <INTEGRITY.h>
#include <device/fbdriver.h>

QT_BEGIN_NAMESPACE

class QIntfbScreenPrivate
{
public:
    QIntfbScreenPrivate();
    ~QIntfbScreenPrivate();

    FBHandle handle;
    struct FBInfoStruct fbinfo;

    QWSMouseHandler *mouse;
#ifndef QT_NO_QWS_KEYBOARD
    QWSKeyboardHandler *keyboard;
#endif
};

QIntfbScreenPrivate::QIntfbScreenPrivate()
    : mouse(0)

{
#ifndef QT_NO_QWS_KEYBOARD
    keyboard = 0;
#endif
}

QIntfbScreenPrivate::~QIntfbScreenPrivate()
{
    delete mouse;
#ifndef QT_NO_QWS_KEYBOARD
    delete keyboard;
#endif
}

/*!
    \internal

    \class QIntfbScreen
    \ingroup qws

    \brief The QIntfbScreen class implements a screen driver for the
    INTEGRITY framebuffer drivers.

    Note that this class is only available in \l{Qt for INTEGRITY}.
    Custom screen drivers can be added by subclassing the
    QScreenDriverPlugin class, using the QScreenDriverFactory class to
    dynamically load the driver into the application, but there should
    only be one screen object per application.

    \sa QScreen, QScreenDriverPlugin, {Running Applications}
*/

/*!
    \fn bool QIntfbScreen::connect(const QString & displaySpec)
    \reimp
*/

/*!
    \fn void QIntfbScreen::disconnect()
    \reimp
*/

/*!
    \fn bool QIntfbScreen::initDevice()
    \reimp
*/

/*!
    \fn void QIntfbScreen::restore()
    \reimp
*/

/*!
    \fn void QIntfbScreen::save()
    \reimp
*/

/*!
    \fn void QIntfbScreen::setDirty(const QRect & r)
    \reimp
*/

/*!
    \fn void QIntfbScreen::setMode(int nw, int nh, int nd)
    \reimp
*/

/*!
    \fn void QIntfbScreen::shutdownDevice()
    \reimp
*/

/*!
    \fn QIntfbScreen::QIntfbScreen(int displayId)

    Constructs a QVNCScreen object. The \a displayId argument
    identifies the Qt for Embedded Linux server to connect to.
*/
QIntfbScreen::QIntfbScreen(int display_id)
    : QScreen(display_id, IntfbClass), d_ptr(new QIntfbScreenPrivate)
{
    d_ptr->handle = 0;
    data = 0;
}

/*!
    Destroys this QIntfbScreen object.
*/
QIntfbScreen::~QIntfbScreen()
{
    delete d_ptr;
}

static QIntfbScreen *connected = 0;

bool QIntfbScreen::connect(const QString &displaySpec)
{
    FBDriver *fbdev;

    CheckSuccess(gh_FB_get_driver(0, &fbdev));
    CheckSuccess(gh_FB_init_device(fbdev, 0, &d_ptr->handle));
    CheckSuccess(gh_FB_get_info(d_ptr->handle, &d_ptr->fbinfo));

    data = (uchar *)d_ptr->fbinfo.start;

    d = d_ptr->fbinfo.bitsperpixel;
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
#ifdef QT_QWS_DEPTH_GENERIC
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
            qt_set_generic_blit(this, 24,
                    d_ptr->fbinfo.redbits,
                    d_ptr->fbinfo.greenbits,
                    d_ptr->fbinfo.bluebits,
                    d_ptr->fbinfo.alphabits,
                    d_ptr->fbinfo.redoffset,
                    d_ptr->fbinfo.greenoffset,
                    d_ptr->fbinfo.blueoffset,
                    d_ptr->fbinfo.alphaoffset);
#else
            qt_set_generic_blit(this, 24,
                    d_ptr->fbinfo.redbits,
                    d_ptr->fbinfo.greenbits,
                    d_ptr->fbinfo.bluebits,
                    d_ptr->fbinfo.alphabits,
                    16 - d_ptr->fbinfo.redoffset,
                    16 - d_ptr->fbinfo.greenoffset,
                    16 - d_ptr->fbinfo.blueoffset,
                    d_ptr->fbinfo.alphaoffset);
#endif
#endif
        break;
    case 32:
        setPixelFormat(QImage::Format_ARGB32_Premultiplied);
#ifdef QT_QWS_DEPTH_GENERIC
#if Q_BYTE_ORDER != Q_BIG_ENDIAN
            qt_set_generic_blit(this, 32,
                    d_ptr->fbinfo.redbits,
                    d_ptr->fbinfo.greenbits,
                    d_ptr->fbinfo.bluebits,
                    d_ptr->fbinfo.alphabits,
                    d_ptr->fbinfo.redoffset,
                    d_ptr->fbinfo.greenoffset,
                    d_ptr->fbinfo.blueoffset,
                    d_ptr->fbinfo.alphaoffset);
#else
            qt_set_generic_blit(this, 32,
                    d_ptr->fbinfo.redbits,
                    d_ptr->fbinfo.greenbits,
                    d_ptr->fbinfo.bluebits,
                    d_ptr->fbinfo.alphabits,
                    24 - d_ptr->fbinfo.redoffset,
                    24 - d_ptr->fbinfo.greenoffset,
                    24 - d_ptr->fbinfo.blueoffset,
                    d_ptr->fbinfo.alphaoffset ? 24 - d_ptr->fbinfo.alphaoffset : 0);
#endif
#endif
        break;
    }

    dw = w = d_ptr->fbinfo.width;
    dh = h = d_ptr->fbinfo.height;

    /* assumes no padding */
    lstep = w * ((d + 7) >> 3);

    mapsize = size =  h * lstep;

    /* default values */
    int dpi = 72;
    physWidth = qRound(dw * 25.4 / dpi);
    physHeight = qRound(dh * 25.4 / dpi);

    qDebug("Connected to INTEGRITYfb server: %d x %d x %d %dx%dmm (%dx%ddpi)",
        w, h, d, physWidth, physHeight, qRound(dw*25.4/physWidth), qRound(dh*25.4/physHeight) );


    QWSServer::setDefaultMouse("integrity");
    QWSServer::setDefaultKeyboard("integrity");

    connected = this;

    return true;
}

void QIntfbScreen::disconnect()
{
    connected = 0;
}

bool QIntfbScreen::initDevice()
{

    CheckSuccess(gh_FB_set_info(d_ptr->handle, &d_ptr->fbinfo, false));
    CheckSuccess(gh_FB_get_info(d_ptr->handle, &d_ptr->fbinfo));
    data = (uchar *)d_ptr->fbinfo.start;
    d = d_ptr->fbinfo.bitsperpixel;
    dw = w = d_ptr->fbinfo.width;
    dh = h = d_ptr->fbinfo.height;
    mapsize = d_ptr->fbinfo.length;
    /* assumes no padding */
    lstep = w * ((d + 7) >> 3);

    mapsize = size =  h * lstep;

    data = (uchar *)d_ptr->fbinfo.start;

    d = d_ptr->fbinfo.bitsperpixel;
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
#ifdef QT_QWS_DEPTH_GENERIC
#if defined(__BIG_ENDIAN__)
       qt_set_generic_blit(this, d,
               d_ptr->fbinfo.redbits,
               d_ptr->fbinfo.greenbits,
               d_ptr->fbinfo.bluebits,
               d_ptr->fbinfo.alphabits,
               24 - d_ptr->fbinfo.redoffset,
               24 - d_ptr->fbinfo.greenoffset,
               24 - d_ptr->fbinfo.blueoffset,
               d_ptr->fbinfo.alphaoffset ? 24 - d_ptr->fbinfo.alphaoffset : 0);
#else
       qt_set_generic_blit(this, d,
               d_ptr->fbinfo.redbits,
               d_ptr->fbinfo.greenbits,
               d_ptr->fbinfo.bluebits,
               d_ptr->fbinfo.alphabits,
               d_ptr->fbinfo.redoffset,
               d_ptr->fbinfo.greenoffset,
               d_ptr->fbinfo.blueoffset,
               d_ptr->fbinfo.alphaoffset);
#endif
#endif

#ifndef QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QIntfbScreen::shutdownDevice()
{
    gh_FB_close(d_ptr->handle);
}

void QIntfbScreen::setMode(int ,int ,int)
{
}

// save the state of the graphics card
// This is needed so that e.g. we can restore the palette when switching
// between linux virtual consoles.
void QIntfbScreen::save()
{
    // nothing to do.
}

// restore the state of the graphics card.
void QIntfbScreen::restore()
{
}
void QIntfbScreen::setDirty(const QRect& rect)
{
    FBRect fbrect;
    fbrect.dx = rect.x();
    fbrect.dy = rect.y();
    fbrect.width = rect.width();
    fbrect.height = rect.height();
    gh_FB_expose(d_ptr->handle, &fbrect);
}

void QIntfbScreen::setBrightness(int b)
{
    if (connected) {
    }
}

void QIntfbScreen::blank(bool on)
{
}

#endif // QT_NO_QWS_INTEGRITYFB

QT_END_NAMESPACE

