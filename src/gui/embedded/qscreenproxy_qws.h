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

#ifndef QPROXYSCREEN_QWS_H
#define QPROXYSCREEN_QWS_H

#include <QtGui/qscreen_qws.h>

#ifndef QT_NO_QWS_PROXYSCREEN

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QProxyScreenPrivate;

#ifndef QT_NO_QWS_CURSOR

class QProxyScreenCursorPrivate;

class Q_GUI_EXPORT QProxyScreenCursor : public QScreenCursor
{
public:
    QProxyScreenCursor();
    ~QProxyScreenCursor();

    void setScreenCursor(QScreenCursor *cursor);
    QScreenCursor* screenCursor() const;

    void set(const QImage &image, int hotx, int hoty);
    void move(int x, int y);
    void show();
    void hide();

private:
    void configure();

    QScreenCursor *realCursor;
    QProxyScreenCursorPrivate *d_ptr;
};

#endif // QT_NO_QWS_CURSOR

class Q_GUI_EXPORT QProxyScreen : public QScreen
{
public:
    QProxyScreen(int display_id, ClassId = ProxyClass);
    ~QProxyScreen();

    void setScreen(QScreen *screen);
    QScreen *screen() const;

    QSize mapToDevice(const QSize &s) const;
    QSize mapFromDevice(const QSize &s) const;

    QPoint mapToDevice(const QPoint &, const QSize &) const;
    QPoint mapFromDevice(const QPoint &, const QSize &) const;

    QRect mapToDevice(const QRect &, const QSize &) const;
    QRect mapFromDevice(const QRect &, const QSize &) const;

    QRegion mapToDevice(const QRegion &, const QSize &) const;
    QRegion mapFromDevice(const QRegion &, const QSize &) const;

    bool connect(const QString &displaySpec);
    bool initDevice();
    void shutdownDevice();
    void disconnect();

    void setMode(int width, int height, int depth);
    bool supportsDepth(int) const;

    void save();
    void restore();
    void blank(bool on);

    bool onCard(const unsigned char *) const;
    bool onCard(const unsigned char *, ulong& out_offset) const;

    bool isInterlaced() const;
    bool isTransformed() const;
    int transformOrientation() const;

    int memoryNeeded(const QString&);
    int sharedRamSize(void *);

    void haltUpdates();
    void resumeUpdates();

    void exposeRegion(QRegion r, int changing);
    void blit(const QImage &img, const QPoint &topLeft, const QRegion &region);
    void solidFill(const QColor &color, const QRegion &region);
    void setDirty(const QRect&);

    QWSWindowSurface* createSurface(QWidget *widget) const;
    QWSWindowSurface* createSurface(const QString &key) const;

    QList<QScreen*> subScreens() const;
    QRegion region() const;

private:
    void configure();

    QScreen *realScreen;
    QProxyScreenPrivate *d_ptr;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_PROXYSCREEN
#endif // QPROXYSCREEN_QWS_H
