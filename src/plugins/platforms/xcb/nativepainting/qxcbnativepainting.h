/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
