/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QXCBNATIVEPAINTING_H
#define QXCBNATIVEPAINTING_H

#include <QSharedDataPointer>
#include "qt_x11_p.h"

typedef struct _FcPattern   FcPattern;
typedef unsigned long XID;
typedef XID Colormap;
typedef XID Window;
typedef struct _XDisplay Display;

QT_BEGIN_NAMESPACE

class QXcbConnection;
class QPixmap;

void qt_xcb_native_x11_info_init(QXcbConnection *conn);
QVector<XRectangle> qt_region_to_xrectangles(const QRegion &r);

class QXcbX11InfoData;
class QXcbX11Info
{
public:
    QXcbX11Info();
    ~QXcbX11Info();
    QXcbX11Info(const QXcbX11Info &other);
    QXcbX11Info &operator=(const QXcbX11Info &other);

    static QXcbX11Info fromScreen(int screen);
    static Display *display();

    int depth() const;
    void setDepth(int depth);

    int screen() const;
    Colormap colormap() const;

    void *visual() const;
    void setVisual(void *visual);

    static int appScreen();
    static int appDepth(int screen = -1);
    static int appCells(int screen = -1);
    static Colormap appColormap(int screen = -1);
    static void *appVisual(int screen = -1);
    static Window appRootWindow(int screen = -1);
    static bool appDefaultColormap(int screen = -1);
    static bool appDefaultVisual(int screen = -1);
    static int appDpiX(int screen = -1);
    static int appDpiY(int screen = -1);

private:
    QSharedDataPointer<QXcbX11InfoData> d;

    friend class QX11PaintEngine;
    friend class QX11PlatformPixmap;
    friend void qt_x11SetScreen(QPixmap &pixmap, int screen);
};

QT_END_NAMESPACE

#endif // QXCBNATIVEPAINTING_H
