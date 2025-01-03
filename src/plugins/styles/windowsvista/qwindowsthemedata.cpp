// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsthemedata_p.h"
#include "qwindowsvistastyle_p_p.h"

/* \internal
    Returns \c true if the QWindowsThemeData is valid for use.
*/
bool QWindowsThemeData::isValid()
{
    return QWindowsVistaStylePrivate::useVista() && theme >= 0 && handle();
}

/* \internal
    Returns the theme engine handle to the specific class.
    If the handle hasn't been opened before, it opens the data, and
    adds it to a static map, for caching.
*/
HTHEME QWindowsThemeData::handle()
{
    if (!QWindowsVistaStylePrivate::useVista())
        return nullptr;

    if (!htheme)
        htheme = QWindowsVistaStylePrivate::createTheme(theme, QWindowsVistaStylePrivate::winId(widget));
    return htheme;
}

/* \internal
    Converts a QRect to the native RECT structure.
*/
RECT QWindowsThemeData::toRECT(const QRect &qr)
{
    RECT r;
    r.left = qr.x();
    r.right = qr.x() + qr.width();
    r.top = qr.y();
    r.bottom = qr.y() + qr.height();
    return r;
}

/* \internal
    Returns the native region of a part, if the part is considered
    transparent. The region is scaled to the parts size (rect).
*/
HRGN QWindowsThemeData::mask(QWidget *widget)
{
    if (!IsThemeBackgroundPartiallyTransparent(handle(), partId, stateId))
        return nullptr;

    HRGN hrgn;
    HDC dc = nullptr;
    if (widget)
        dc = QWindowsVistaStylePrivate::hdcForWidgetBackingStore(widget);
    RECT nativeRect = toRECT(rect);
    GetThemeBackgroundRegion(handle(), dc, partId, stateId, &nativeRect, &hrgn);
    return hrgn;
}
