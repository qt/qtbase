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

#include "qwindowsscreen.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"
#include "pixmaputils.h"
#include "qwindowscursor.h"

#include "qtwindows_additional.h"

#include <QtGui/QPixmap>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

QWindowsScreenData::QWindowsScreenData() :
    dpi(96, 96),
    depth(32),
    format(QImage::Format_ARGB32_Premultiplied), primary(false)
{
}

static inline QDpi deviceDPI(HDC hdc)
{
    return QDpi(GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));
}

static inline QSizeF deviceSizeMM(const QSize &pixels, const QDpi &dpi)
{
    const qreal inchToMM = 25.4;
    const qreal h = qreal(pixels.width())  / qreal(dpi.first)  * inchToMM;
    const qreal v = qreal(pixels.height()) / qreal(dpi.second) * inchToMM;
    return QSizeF(h, v);
}

static inline QDpi deviceDPI(const QSize &pixels, const QSizeF &physicalSizeMM)
{
    const qreal inchToMM = 25.4;
    const qreal h = qreal(pixels.width())  / (qreal(physicalSizeMM.width())  / inchToMM);
    const qreal v = qreal(pixels.height()) / (qreal(physicalSizeMM.height()) / inchToMM);
    return QDpi(h, v);
}

typedef QList<QWindowsScreenData> WindowsScreenDataList;

// from QDesktopWidget, taking WindowsScreenDataList as LPARAM
BOOL QT_WIN_CALLBACK monitorEnumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM p)
{
    MONITORINFOEX info;
    memset(&info, 0, sizeof(MONITORINFOEX));
    info.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(hMonitor, &info) == FALSE)
        return TRUE;

    WindowsScreenDataList *result = reinterpret_cast<WindowsScreenDataList *>(p);
    QWindowsScreenData data;
    data.geometry = QRect(QPoint(info.rcMonitor.left, info.rcMonitor.top), QPoint(info.rcMonitor.right - 1, info.rcMonitor.bottom - 1));
    if (HDC hdc = CreateDC(info.szDevice, NULL, NULL, NULL)) {
        data.dpi = deviceDPI(hdc);
        data.physicalSizeMM = QSizeF(GetDeviceCaps(hdc, HORZSIZE), GetDeviceCaps(hdc, VERTSIZE));
        DeleteDC(hdc);
    } else {
        qWarning("%s: Unable to obtain handle for monitor '%s', defaulting to %d DPI.",
                 __FUNCTION__, qPrintable(QString::fromWCharArray(info.szDevice)),
                 data.dpi.first);
    }
    data.geometry = QRect(QPoint(info.rcMonitor.left, info.rcMonitor.top), QPoint(info.rcMonitor.right - 1, info.rcMonitor.bottom - 1));
    data.availableGeometry = QRect(QPoint(info.rcWork.left, info.rcWork.top), QPoint(info.rcWork.right - 1, info.rcWork.bottom - 1));
    data.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    result->append(data);
    return TRUE;
}

/*!
    \class QWindowsScreen
    \brief Windows screen.
    \ingroup qt-lighthouse-win
*/

QWindowsScreen::QWindowsScreen(const QWindowsScreenData &data) :
    m_data(data), m_cursor(this)
{
}

QList<QPlatformScreen *> QWindowsScreen::screens()
{
    // Retrieve monitors and add static depth information to each.
    WindowsScreenDataList data;
    EnumDisplayMonitors(0, 0, monitorEnumCallback, (LPARAM)&data);

    const int depth = QWindowsContext::instance()->screenDepth();
    const QImage::Format format = depth == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
    QList<QPlatformScreen *> result;

    const WindowsScreenDataList::const_iterator scend = data.constEnd();
    for (WindowsScreenDataList::const_iterator it = data.constBegin(); it != scend; ++it) {
        QWindowsScreenData d = *it;
        d.depth = depth;
        d.format = format;
        if (QWindowsContext::verboseIntegration)
            qDebug() << "Screen" << d.geometry << d.availableGeometry << d.primary
                     << " physical " << d.physicalSizeMM << " DPI" << d.dpi
                     << "Depth: " << d.depth << " Format: " << d.format;
        result.append(new QWindowsScreen(d));
    }
    return result;
}

QPixmap QWindowsScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    if (QWindowsContext::verboseIntegration)
        qDebug() << __FUNCTION__ << window << x << y << width << height;
    RECT r;
    HWND hwnd = (HWND)window;
    GetClientRect(hwnd, &r);

    if (width < 0) width = r.right - r.left;
    if (height < 0) height = r.bottom - r.top;

    // Create and setup bitmap
    HDC display_dc = GetDC(0);
    HDC bitmap_dc = CreateCompatibleDC(display_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(display_dc, width, height);
    HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    // copy data
    HDC window_dc = GetDC(hwnd);
    BitBlt(bitmap_dc, 0, 0, width, height, window_dc, x, y, SRCCOPY | CAPTUREBLT);

    // clean up all but bitmap
    ReleaseDC(hwnd, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    const QPixmap pixmap = qPixmapFromWinHBITMAP(bitmap, HBitmapNoAlpha);

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    return pixmap;
}

/*!
    \brief Find a top level window taking the flags of ChildWindowFromPointEx.
*/

QWindow *QWindowsScreen::findTopLevelAt(const QPoint &point, unsigned flags)
{
    QWindow* result = 0;
    if (QPlatformWindow *bw = QWindowsContext::instance()->
            findPlatformWindowAt(GetDesktopWindow(), point, flags))
        result = QWindowsWindow::topLevelOf(bw->window());
    if (QWindowsContext::verboseWindows)
        qDebug() << __FUNCTION__ << point << flags << result;
    return result;
}

QWindow *QWindowsScreen::windowAt(const QPoint &screenPoint, unsigned flags)
{
    QWindow* result = 0;
    if (QPlatformWindow *bw = QWindowsContext::instance()->
            findPlatformWindowAt(GetDesktopWindow(), screenPoint, flags))
        result = bw->window();
    if (QWindowsContext::verboseWindows)
        qDebug() << __FUNCTION__ << screenPoint << " returns " << result;
    return result;
}

QWindow *QWindowsScreen::windowUnderMouse(unsigned flags)
{
    return QWindowsScreen::windowAt(QWindowsCursor::mousePosition(), flags);
}

QWindowsScreen *QWindowsScreen::screenOf(const QWindow *w)
{
    if (w)
        if (const QScreen *s = w->screen())
            if (QPlatformScreen *pscr = s->handle())
                return static_cast<QWindowsScreen *>(pscr);
    if (const QScreen *ps = QGuiApplication::primaryScreen())
        if (QPlatformScreen *ppscr = ps->handle())
            return static_cast<QWindowsScreen *>(ppscr);
    return 0;
}

QT_END_NAMESPACE
