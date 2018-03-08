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

#include "qxcbconnection.h"
#include "qcolormap_x11_p.h"
#include "qxcbnativepainting.h"
#include "qt_x11_p.h"

QT_BEGIN_NAMESPACE

QXcbX11Data *qt_x11Data = nullptr;

void qt_xcb_native_x11_info_init(QXcbConnection *conn)
{
    qt_x11Data = new QXcbX11Data;
    X11->display = static_cast<Display *>(conn->xlib_display());
    X11->defaultScreen = DefaultScreen(X11->display);
    X11->screenCount = ScreenCount(X11->display);

    X11->screens = new QX11InfoData[X11->screenCount];
    X11->argbVisuals = new Visual *[X11->screenCount];
    X11->argbColormaps = new Colormap[X11->screenCount];

    for (int s = 0; s < X11->screenCount; s++) {
        QX11InfoData *screen = X11->screens + s;
        //screen->ref = 1; // ensures it doesn't get deleted
        screen->screen = s;

        int widthMM = DisplayWidthMM(X11->display, s);
        if (widthMM != 0) {
            screen->dpiX = (DisplayWidth(X11->display, s) * 254 + widthMM * 5) / (widthMM * 10);
        } else {
            screen->dpiX = 72;
        }

        int heightMM = DisplayHeightMM(X11->display, s);
        if (heightMM != 0) {
            screen->dpiY = (DisplayHeight(X11->display, s) * 254 + heightMM * 5) / (heightMM * 10);
        } else {
            screen->dpiY = 72;
        }

        X11->argbVisuals[s] = 0;
        X11->argbColormaps[s] = 0;
    }

    X11->use_xrender = conn->hasXRender() && !qEnvironmentVariableIsSet("QT_XCB_NATIVE_PAINTING_NO_XRENDER");

#if QT_CONFIG(xrender)
    memset(X11->solid_fills, 0, sizeof(X11->solid_fills));
    for (int i = 0; i < X11->solid_fill_count; ++i)
        X11->solid_fills[i].screen = -1;
    memset(X11->pattern_fills, 0, sizeof(X11->pattern_fills));
    for (int i = 0; i < X11->pattern_fill_count; ++i)
        X11->pattern_fills[i].screen = -1;
#endif

    QXcbColormap::initialize();

#if QT_CONFIG(xrender)
    if (X11->use_xrender) {
        // XRender is supported, let's see if we have a PictFormat for the
        // default visual
        XRenderPictFormat *format =
            XRenderFindVisualFormat(X11->display,
                                    (Visual *) QXcbX11Info::appVisual(X11->defaultScreen));

        if (!format) {
            X11->use_xrender = false;
        }
    }
#endif // QT_CONFIG(xrender)
}

QVector<XRectangle> qt_region_to_xrectangles(const QRegion &r)
{
    const int numRects = r.rectCount();
    const auto input = r.begin();
    QVector<XRectangle> output(numRects);
    for (int i = 0; i < numRects; ++i) {
        const QRect &in = input[i];
        XRectangle &out = output[i];
        out.x = qMax(SHRT_MIN, in.x());
        out.y = qMax(SHRT_MIN, in.y());
        out.width = qMin((int)USHRT_MAX, in.width());
        out.height = qMin((int)USHRT_MAX, in.height());
    }
    return output;
}

class QXcbX11InfoData : public QSharedData, public QX11InfoData
{};

QXcbX11Info::QXcbX11Info()
    : d(nullptr)
{}

QXcbX11Info::~QXcbX11Info()
{}

QXcbX11Info::QXcbX11Info(const QXcbX11Info &other)
    : d(other.d)
{}

QXcbX11Info &QXcbX11Info::operator=(const QXcbX11Info &other)
{
    d = other.d;
    return *this;
}

QXcbX11Info QXcbX11Info::fromScreen(int screen)
{
    QXcbX11InfoData *xd = new QXcbX11InfoData;
    xd->screen = screen;
    xd->depth = QXcbX11Info::appDepth(screen);
    xd->cells = QXcbX11Info::appCells(screen);
    xd->colormap = QXcbX11Info::appColormap(screen);
    xd->defaultColormap = QXcbX11Info::appDefaultColormap(screen);
    xd->visual = (Visual *)QXcbX11Info::appVisual(screen);
    xd->defaultVisual = QXcbX11Info::appDefaultVisual(screen);

    QXcbX11Info info;
    info.d = xd;
    return info;
}

void QXcbX11Info::setDepth(int depth)
{
    if (!d)
        *this = fromScreen(appScreen());

    d->depth = depth;
}

Display *QXcbX11Info::display()
{
    return X11 ? X11->display : 0;
}

int QXcbX11Info::screen() const
{
    return d ? d->screen : QXcbX11Info::appScreen();
}

int QXcbX11Info::depth() const
{
    return d ? d->depth : QXcbX11Info::appDepth();
}

Colormap QXcbX11Info::colormap() const
{
    return d ? d->colormap : QXcbX11Info::appColormap();
}

void *QXcbX11Info::visual() const
{
    return d ? d->visual : QXcbX11Info::appVisual();
}

void QXcbX11Info::setVisual(void *visual)
{
    if (!d)
        *this = fromScreen(appScreen());

    d->visual = (Visual *) visual;
}

int QXcbX11Info::appScreen()
{
    return X11 ? X11->defaultScreen : 0;
}

int QXcbX11Info::appDepth(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].depth : 32;
}

int QXcbX11Info::appCells(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].cells : 0;
}

Colormap QXcbX11Info::appColormap(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].colormap : 0;
}

void *QXcbX11Info::appVisual(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].visual : 0;
}

Window QXcbX11Info::appRootWindow(int screen)
{
    return X11 ? RootWindow(X11->display, screen == -1 ? X11->defaultScreen : screen) : 0;
}

bool QXcbX11Info::appDefaultColormap(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].defaultColormap : true;
}

bool QXcbX11Info::appDefaultVisual(int screen)
{
    return X11 ? X11->screens[screen == -1 ? X11->defaultScreen : screen].defaultVisual : true;
}

int QXcbX11Info::appDpiX(int screen)
{
    if (!X11)
        return 75;
    if (screen < 0)
        screen = X11->defaultScreen;
    if (screen > X11->screenCount)
        return 0;
    return X11->screens[screen].dpiX;
}

int QXcbX11Info::appDpiY(int screen)
{
    if (!X11)
        return 75;
    if (screen < 0)
        screen = X11->defaultScreen;
    if (screen > X11->screenCount)
        return 0;
    return X11->screens[screen].dpiY;
}

#if QT_CONFIG(xrender)
Picture QXcbX11Data::getSolidFill(int screen, const QColor &c)
{
    if (!X11->use_xrender)
        return XNone;

    XRenderColor color = preMultiply(c);
    for (int i = 0; i < X11->solid_fill_count; ++i) {
        if (X11->solid_fills[i].screen == screen
            && X11->solid_fills[i].color.alpha == color.alpha
            && X11->solid_fills[i].color.red == color.red
            && X11->solid_fills[i].color.green == color.green
            && X11->solid_fills[i].color.blue == color.blue)
            return X11->solid_fills[i].picture;
    }
    // none found, replace one
    int i = qrand() % 16;

    if (X11->solid_fills[i].screen != screen && X11->solid_fills[i].picture) {
        XRenderFreePicture (X11->display, X11->solid_fills[i].picture);
        X11->solid_fills[i].picture = 0;
    }

    if (!X11->solid_fills[i].picture) {
        Pixmap pixmap = XCreatePixmap (X11->display, RootWindow (X11->display, screen), 1, 1, 32);
        XRenderPictureAttributes attrs;
        attrs.repeat = True;
        X11->solid_fills[i].picture = XRenderCreatePicture (X11->display, pixmap,
                                                            XRenderFindStandardFormat(X11->display, PictStandardARGB32),
                                                            CPRepeat, &attrs);
        XFreePixmap (X11->display, pixmap);
    }

    X11->solid_fills[i].color = color;
    X11->solid_fills[i].screen = screen;
    XRenderFillRectangle (X11->display, PictOpSrc, X11->solid_fills[i].picture, &color, 0, 0, 1, 1);
    return X11->solid_fills[i].picture;
}

XRenderColor QXcbX11Data::preMultiply(const QColor &c)
{
    XRenderColor color;
    const uint A = c.alpha(),
            R = c.red(),
            G = c.green(),
            B = c.blue();
    color.alpha = (A | A << 8);
    color.red   = (R | R << 8) * color.alpha / 0x10000;
    color.green = (G | G << 8) * color.alpha / 0x10000;
    color.blue  = (B | B << 8) * color.alpha / 0x10000;
    return color;
}
#endif // QT_CONFIG(xrender)


QT_END_NAMESPACE
