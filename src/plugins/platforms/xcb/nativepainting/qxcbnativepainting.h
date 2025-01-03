// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
QList<XRectangle> qt_region_to_xrectangles(const QRegion &r);

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
