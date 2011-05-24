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

#ifndef QGRAPHICSSYSTEM_LINUXFB_H
#define QGRAPHICSSYSTEM_LINUXFB_H

#include <QPlatformIntegration>
#include "../fb_base/fb_base.h"

QT_BEGIN_NAMESPACE

class QLinuxFbScreen : public QFbScreen
{
    Q_OBJECT
public:
    QLinuxFbScreen(uchar * d, int w, int h, int lstep, QImage::Format screenFormat);
    void setGeometry(QRect rect);
    void setFormat(QImage::Format format);

public slots:
    QRegion doRedraw();

private:
    QImage * mFbScreenImage;
    uchar * data;
    int bytesPerLine;

    QPainter *compositePainter;
};

class QLinuxFbIntegrationPrivate;
struct fb_cmap;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;

class QLinuxFbIntegration : public QPlatformIntegration
{
public:
    QLinuxFbIntegration();
    ~QLinuxFbIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;

    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
    QPlatformWindow *createPlatformWindow(QWidget *widget, WId WinId) const;
    QWindowSurface *createWindowSurface(QWidget *widget, WId WinId) const;

    QList<QPlatformScreen *> screens() const { return mScreens; }

    QPlatformFontDatabase *fontDatabase() const;

private:
    QLinuxFbScreen *mPrimaryScreen;
    QList<QPlatformScreen *> mScreens;
    QLinuxFbIntegrationPrivate *d_ptr;

    enum PixelType { NormalPixel, BGRPixel };

    QRgb screenclut[256];
    int screencols;

    uchar * data;

    QImage::Format screenFormat;
    int w;
    int lstep;
    int h;
    int d;
    PixelType pixeltype;
    bool grayscale;

    int dw;
    int dh;

    int size;               // Screen size
    int mapsize;       // Total mapped memory

    int displayId;

    int physWidth;
    int physHeight;

    bool canaccel;
    int dataoffset;
    int cacheStart;

    bool connect(const QString &displaySpec);
    bool initDevice();
    void setPixelFormat(struct fb_var_screeninfo);
    void createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo);
    void blank(bool on);
    QPlatformFontDatabase *fontDb;
};

QT_END_NAMESPACE

#endif
