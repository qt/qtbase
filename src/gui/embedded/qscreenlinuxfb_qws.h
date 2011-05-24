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

#ifndef QSCREENLINUXFB_QWS_H
#define QSCREENLINUXFB_QWS_H

#include <QtGui/qscreen_qws.h>

struct fb_cmap;
struct fb_var_screeninfo;
struct fb_fix_screeninfo;


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_QWS_LINUXFB

class QLinuxFb_Shared
{
public:
    volatile int lastop;
    volatile int optype;
    volatile int fifocount;   // Accel drivers only
    volatile int fifomax;
    volatile int forecol;     // Foreground colour caching
    volatile unsigned int buffer_offset;   // Destination
    volatile int linestep;
    volatile int cliptop;    // Clip rectangle
    volatile int clipleft;
    volatile int clipright;
    volatile int clipbottom;
    volatile unsigned int rop;

};

class QLinuxFbScreenPrivate;

class Q_GUI_EXPORT QLinuxFbScreen : public QScreen
{
public:
    explicit QLinuxFbScreen(int display_id);
    virtual ~QLinuxFbScreen();

    virtual bool initDevice();
    virtual bool connect(const QString &displaySpec);

    virtual bool useOffscreen();

    enum DriverTypes { GenericDriver, EInk8Track };

    virtual void disconnect();
    virtual void shutdownDevice();
    virtual void setMode(int,int,int);
    virtual void save();
    virtual void restore();
    virtual void blank(bool on);
    virtual void set(unsigned int,unsigned int,unsigned int,unsigned int);
    virtual uchar * cache(int);
    virtual void uncache(uchar *);
    virtual int sharedRamSize(void *);
    virtual void setDirty(const QRect&);

    QLinuxFb_Shared * shared;

protected:

    void deleteEntry(uchar *);

    bool canaccel;
    int dataoffset;
    int cacheStart;

    virtual void fixupScreenInfo(fb_fix_screeninfo &finfo, fb_var_screeninfo &vinfo);
    static void clearCache(QScreen *instance, int);

private:

    void delete_entry(int);
    void insert_entry(int,int,int);
    void setupOffScreen();
    void createPalette(fb_cmap &cmap, fb_var_screeninfo &vinfo, fb_fix_screeninfo &finfo);
    void setPixelFormat(struct fb_var_screeninfo);

    QLinuxFbScreenPrivate *d_ptr;
};

#endif // QT_NO_QWS_LINUXFB

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCREENLINUXFB_QWS_H
