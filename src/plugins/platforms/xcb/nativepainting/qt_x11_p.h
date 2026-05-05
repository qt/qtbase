// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#undef register

#if QT_CONFIG(xrender)
#  include "qtessellator_p.h"
#  include <X11/extensions/Xrender.h>
#endif

#if QT_CONFIG(fontconfig)
#include <fontconfig/fontconfig.h>
#endif

#if defined(FT_LCD_FILTER_H)
#include FT_LCD_FILTER_H
#endif

#if defined(FC_LCD_FILTER)

#ifndef FC_LCD_FILTER_NONE
#define FC_LCD_FILTER_NONE FC_LCD_NONE
#endif

#ifndef FC_LCD_FILTER_DEFAULT
#define FC_LCD_FILTER_DEFAULT FC_LCD_DEFAULT
#endif

#ifndef FC_LCD_FILTER_LIGHT
#define FC_LCD_FILTER_LIGHT FC_LCD_LIGHT
#endif

#ifndef FC_LCD_FILTER_LEGACY
#define FC_LCD_FILTER_LEGACY FC_LCD_LEGACY
#endif

#endif

QT_BEGIN_NAMESPACE

// rename a couple of X defines to get rid of name clashes
// resolve the conflict between X11's FocusIn and QEvent::FocusIn
enum {
    XFocusOut = FocusOut,
    XFocusIn = FocusIn,
    XKeyPress = KeyPress,
    XKeyRelease = KeyRelease,
    XNone = None,
    XRevertToParent = RevertToParent,
    XGrayScale = GrayScale,
    XCursorShape = CursorShape,
};
#undef FocusOut
#undef FocusIn
#undef KeyPress
#undef KeyRelease
#undef None
#undef RevertToParent
#undef GrayScale
#undef CursorShape

#ifdef FontChange
#undef FontChange
#endif

Q_DECLARE_TYPEINFO(XPoint, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(XRectangle, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(XChar2b, Q_PRIMITIVE_TYPE);
#if QT_CONFIG(xrender)
Q_DECLARE_TYPEINFO(XGlyphElt32, Q_PRIMITIVE_TYPE);
#endif

struct QX11InfoData;

enum DesktopEnvironment {
    DE_UNKNOWN,
    DE_KDE,
    DE_GNOME,
    DE_CDE,
    DE_MEEGO_COMPOSITOR,
    DE_4DWM
};

struct QXcbX11Data {
    Display *display = nullptr;

    // true if Qt is compiled w/ RENDER support and RENDER is supported on the connected Display
    bool use_xrender = false;
    int xrender_major = 0;
    int xrender_version = 0;

    QX11InfoData *screens = nullptr;
    Visual **argbVisuals = nullptr;
    Colormap *argbColormaps = nullptr;
    int screenCount = 0;
    int defaultScreen = 0;

    // options
    int visual_class = 0;
    int visual_id = 0;
    int color_count = 0;
    bool custom_cmap = false;

    // outside visual/colormap
    Visual *visual = nullptr;
    Colormap colormap = 0;

#if QT_CONFIG(xrender)
    enum { solid_fill_count = 16 };
    struct SolidFills {
        XRenderColor color;
        int screen;
        Picture picture;
    } solid_fills[solid_fill_count];
    enum { pattern_fill_count = 16 };
    struct PatternFills {
        XRenderColor color;
        XRenderColor bg_color;
        int screen;
        int style;
        bool opaque;
        Picture picture;
    } pattern_fills[pattern_fill_count];
    Picture getSolidFill(int screen, const QColor &c);
    XRenderColor preMultiply(const QColor &c);
#endif

    bool fc_antialias = true;
    int fc_hint_style = 0;

    DesktopEnvironment desktopEnvironment = DE_GNOME;
};

extern QXcbX11Data *qt_x11Data;
#define X11 qt_x11Data

struct QX11InfoData {
    int screen;
    int dpiX;
    int dpiY;
    int depth;
    int cells;
    Colormap colormap;
    Visual *visual;
    bool defaultColormap;
    bool defaultVisual;
    int subpixel = 0;
};

template <class T>
constexpr inline int lowest_bit(T v) noexcept
{
    int result = qCountTrailingZeroBits(v);
    return ((result >> 3) == sizeof(T)) ? -1 : result;
}

QT_END_NAMESPACE
