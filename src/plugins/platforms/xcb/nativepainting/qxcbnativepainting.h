/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
