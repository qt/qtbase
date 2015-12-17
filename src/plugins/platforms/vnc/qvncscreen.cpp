/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvncscreen.h"
#include "qvnc_p.h"
#include <QtPlatformSupport/private/qfbwindow_p.h>
#include <QtPlatformSupport/private/qfbcursor_p.h>

#include <QtGui/QPainter>


QT_BEGIN_NAMESPACE


QVncScreen::QVncScreen(const QStringList &args)
    : mArgs(args)
{
    initialize();
}

QVncScreen::~QVncScreen()
{
    if (clientCursor)
        delete clientCursor;
}

bool QVncScreen::initialize()
{
    mGeometry = QRect(0, 0, 1024, 768);
    mFormat = QImage::Format_ARGB32_Premultiplied;
    mDepth = 32;
    mPhysicalSize = QSizeF(mGeometry.width()/96.*25.4, mGeometry.height()/96.*25.4);

    QFbScreen::initializeCompositor();
    QT_VNC_DEBUG() << "QVncScreen::init" << geometry();

    disableClientCursor();

    switch (depth()) {
    case 32:
        dirty = new QVncDirtyMapOptimized<quint32>(this);
        break;
    case 16:
        dirty = new QVncDirtyMapOptimized<quint16>(this);
        break;
    case 8:
        dirty = new QVncDirtyMapOptimized<quint8>(this);
        break;
    default:
        qWarning("QVNCScreen::initDevice: No support for screen depth %d",
                 depth());
        dirty = 0;
        return false;
    }

    setPowerState(PowerStateOff);

    return true;
}

QRegion QVncScreen::doRedraw()
{
    QRegion touched = QFbScreen::doRedraw();
    QT_VNC_DEBUG() << "qvncscreen::doRedraw()" << touched.rectCount();

    if (touched.isEmpty())
        return touched;
    dirtyRegion += touched;

    vncServer->setDirty();
    return touched;
}

void QVncScreen::enableClientCursor()
{
    delete mCursor;
    mCursor = 0;
    clientCursor = new QVncClientCursor(vncServer);
}

void QVncScreen::disableClientCursor()
{
    if (vncServer && clientCursor) {
        delete clientCursor;
        clientCursor = 0;
    }
    mCursor = new QFbCursor(this);
}

QPlatformCursor *QVncScreen::cursor() const
{
    return mCursor ? static_cast<QPlatformCursor *>(mCursor) : static_cast<QPlatformCursor *>(clientCursor);
}

// grabWindow() grabs "from the screen" not from the backingstores.
// In linuxfb's case it will also include the mouse cursor.
QPixmap QVncScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid) {
        if (width < 0)
            width = mScreenImage->width() - x;
        if (height < 0)
            height = mScreenImage->height() - y;
        return QPixmap::fromImage(*mScreenImage).copy(x, y, width, height);
    }

    QFbWindow *window = windowForId(wid);
    if (window) {
        const QRect geom = window->geometry();
        if (width < 0)
            width = geom.width() - x;
        if (height < 0)
            height = geom.height() - y;
        QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
        rect &= window->geometry();
        return QPixmap::fromImage(*mScreenImage).copy(rect);
    }

    return QPixmap();
}

QT_END_NAMESPACE

