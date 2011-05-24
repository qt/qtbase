/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qvncintegration.h"
#include "../fb_base/fb_base.h"
#include <private/qapplication_p.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtCore/qdebug.h>

#include <qvncserver.h>
#include <QtGui/QPainter>

#include <QtCore/QTimer>
#include "qgenericunixfontdatabase.h"

QVNCScreen::QVNCScreen(QRect screenSize, int screenId)
        : QFbScreen::QFbScreen()
{
    setGeometry(screenSize);
    setDepth(32);
    setFormat(QImage::Format_RGB32);
    setPhysicalSize((geometry().size()*254)/720);


    d_ptr = new QVNCScreenPrivate(this, screenId);

    cursor = new QVNCCursor(d_ptr->vncServer, this);
    d_ptr->vncServer->setCursor(static_cast<QVNCCursor *>(cursor));
}

QVNCDirtyMap *QVNCScreen::dirtyMap()
{
    return d_ptr->dirty;
}

QRegion QVNCScreen::doRedraw()
{
    QRegion touched;
    touched = QFbScreen::doRedraw();

    QVector<QRect> rects = touched.rects();
    for (int i = 0; i < rects.size(); i++)
        d_ptr->setDirty(rects[i]);
    return touched;
}

static inline int defaultWidth() { return 800; }
static inline int defaultHeight() { return 600; }
static inline int defaultDisplay() { return 0; }

static void usage()
{
    qWarning() << "VNC Platform Integration options:";
    qWarning() << "    size=<Width>x<Height> - set the display width and height";
    qWarning() << "         defaults to" << defaultWidth() << "x" << defaultHeight();
    qWarning() << "    display=<ID> - set the VNC display port to ID + 5900";
    qWarning() << "         defaults to" << defaultDisplay();
    qWarning() << "    offset=<X>x<Y> - set the current screens offset";
    qWarning() << "    vnc - start configuration of a new screen";
    qWarning() << "         size and offset are inherited from the previous screen if not set";
    qWarning() << "         display id is incremented from the previous screen if not set";
    qWarning() << "    virtual - manage the set of screens as a virtual desktop";
}

QVNCIntegration::QVNCIntegration(const QStringList& paramList)
    : virtualDesktop(false), fontDb(new QGenericUnixFontDatabase())
{
    int sizeX = defaultWidth();
    int sizeY = defaultHeight();
    int offsetX = 0;
    int offsetY = 0;
    int display = defaultDisplay();
    bool showUsage = false;

    foreach(QString confString, paramList) {
        if (confString.startsWith(QLatin1String("size="))) {
            QString val = confString.section(QLatin1Char('='), 1, 1);
            sizeX = val.section(QLatin1Char('x'), 0, 0).toInt();
            sizeY = val.section(QLatin1Char('x'), 1, 1).toInt();
        }
        else if (confString.startsWith(QLatin1String("display="))) {
            display = confString.section(QLatin1Char('='), 1, 1).toInt();
        }
        else if (confString.startsWith(QLatin1String("offset="))) {
            QString val = confString.section(QLatin1Char('='), 1, 1);
            offsetX = val.section(QLatin1Char('x'), 0, 0).toInt();
            offsetY = val.section(QLatin1Char('x'), 1, 1).toInt();
        }
        else if (confString == QLatin1String("vnc")) {
            QRect screenRect(offsetX, offsetY, sizeX, sizeY);
            QVNCScreen *screen = new QVNCScreen(screenRect, display);
            mScreens.append(screen);
            screen->setObjectName(QString("screen %1").arg(display));
            screen->setDirty(screenRect);
            ++display;
        }
        else if (confString == QLatin1String("virtual")) {
            virtualDesktop = true;
        }
        else {
            qWarning() << "Unknown VNC option:" << confString;
            showUsage = true;
        }
    }

    if (showUsage)
        usage();

    QRect screenRect(offsetX, offsetY, sizeX, sizeY);
    QVNCScreen *screen = new QVNCScreen(screenRect, display);
    mScreens.append(screen);
    mPrimaryScreen = qobject_cast<QVNCScreen *>(mScreens.first());
    screen->setObjectName(QString("screen %1").arg(display));
    screen->setDirty(screenRect);
}

bool QVNCIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}


QPixmapData *QVNCIntegration::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QWindowSurface *QVNCIntegration::createWindowSurface(QWidget *widget, WId) const
{
    QFbWindowSurface * surface;
    surface = new QFbWindowSurface(mPrimaryScreen, widget);
    return surface;
}


QPlatformWindow *QVNCIntegration::createPlatformWindow(QWidget *widget, WId /*winId*/) const
{
    QFbWindow *w = new QFbWindow(widget);
    if (virtualDesktop) {
        QList<QPlatformScreen *>::const_iterator i = mScreens.constBegin();
        QList<QPlatformScreen *>::const_iterator end = mScreens.constEnd();
        QFbScreen *screen;
        while (i != end) {
            screen = static_cast<QFbScreen *>(*i);
            screen->addWindow(w);
            ++i;
        }
    }
    else
        mPrimaryScreen->addWindow(w);
    return w;
}

QPixmap QVNCIntegration::grabWindow(WId window, int x, int y, int width, int height) const
{
//    qDebug() << "QVNCIntegration::grabWindow" << window << x << y << width << height;

    if (window == 0) { //desktop
        QImage *desktopImage = mPrimaryScreen->image();
        if (x==0 && y == 0 && width < 0 && height < 0) {
            return QPixmap::fromImage(*desktopImage);
        }
        if (width < 0)
            width = desktopImage->width() - x;
        if (height < 0)
            height = desktopImage->height() - y;
        int bytesPerPixel = desktopImage->depth()/8; //We don't support 1, 2, or 4 bpp
        QImage img(desktopImage->scanLine(y) + bytesPerPixel*x, width, height, desktopImage->bytesPerLine(), desktopImage->format());
        return QPixmap::fromImage(img);
    }
    QWidget *win = QWidget::find(window);
    if (win) {
        QRect r = win->geometry();
        if (width < 0)
            width = r.width() - x;
        if (height < 0)
            height = r.height() - y;
        QImage *desktopImage = mPrimaryScreen->image();
        int bytesPerPixel = desktopImage->depth()/8; //We don't support 1, 2, or 4 bpp

        QImage img(desktopImage->scanLine(r.top() + y) + bytesPerPixel*(r.left()+x), width, height, desktopImage->bytesPerLine(), desktopImage->format());
          return QPixmap::fromImage(img);
    }
    return QPixmap();
}


void QVNCIntegration::moveToScreen(QWidget *window, int screen)
{
    if (virtualDesktop) {   // all windows exist on all screens in virtual desktop mode
        return;
    }
    if (screen < 0 || screen > mScreens.size())
        return;
    QVNCScreen * newScreen = qobject_cast<QVNCScreen *>(mScreens.at(screen));
    for(int i = 0; i < mScreens.size(); i++) {
        QVNCScreen *oldScreen = qobject_cast<QVNCScreen *>(mScreens.at(i));
        if (oldScreen->windowStack.contains(static_cast<QFbWindow *>(window->platformWindow()))) {
            oldScreen->removeWindow(static_cast<QFbWindow *>(window->platformWindow()));
            break;
        }
    }
    window->platformWindow()->setGeometry(window->geometry());  // this should be unified elsewhere
    newScreen->addWindow(static_cast<QFbWindow *>(window->platformWindow()));
}

QPlatformFontDatabase *QVNCIntegration::fontDatabase() const
{
    return fontDb;
}
