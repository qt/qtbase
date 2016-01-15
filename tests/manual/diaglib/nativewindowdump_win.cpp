/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "nativewindowdump.h"
#include "qwindowdump.h"

#include <QtCore/QTextStream>
#include <QtCore/QSharedPointer>
#include <QtCore/QDebug>
#include <QtCore/QVector>

#include <QtCore/qt_windows.h>

#ifndef WS_EX_NOREDIRECTIONBITMAP
#  define WS_EX_NOREDIRECTIONBITMAP 0x00200000L
#endif

namespace QtDiag {

struct DumpContext {
    DumpContext() : indentation(0), parent(0) {}

    int indentation;
    HWND parent;
    QSharedPointer<QTextStream> stream;
};

#define debugWinStyle(str, style, styleConstant) \
if (style & styleConstant) \
    str << ' ' << #styleConstant;

static void formatNativeWindow(HWND hwnd, QTextStream &str)
{
    str << hex << showbase << quintptr(hwnd) << noshowbase << dec;
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        str << ' ' << (rect.right - rect.left) << 'x' << (rect.bottom - rect.top)
            << forcesign << rect.left << rect.top << noforcesign;
    }
    if (IsWindowVisible(hwnd))
        str << " [visible]";

    wchar_t buf[512];
    if (GetWindowText(hwnd, buf, sizeof(buf)/sizeof(buf[0])) && buf[0])
        str << " title=\"" << QString::fromWCharArray(buf) << "\"/";
    else
        str << ' ';
    if (GetClassName(hwnd, buf, sizeof(buf)/sizeof(buf[0])))
        str << '"' << QString::fromWCharArray(buf) << '"';

    str << hex << showbase;
    if (const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE)) {
        str << " style=" << style;
        debugWinStyle(str, style, WS_OVERLAPPED)
        debugWinStyle(str, style, WS_POPUP)
        debugWinStyle(str, style, WS_MINIMIZE)
        debugWinStyle(str, style, WS_CHILD)
        debugWinStyle(str, style, WS_VISIBLE)
        debugWinStyle(str, style, WS_DISABLED)
        debugWinStyle(str, style, WS_CLIPSIBLINGS)
        debugWinStyle(str, style, WS_CLIPCHILDREN)
        debugWinStyle(str, style, WS_MAXIMIZE)
        debugWinStyle(str, style, WS_CAPTION)
        debugWinStyle(str, style, WS_BORDER)
        debugWinStyle(str, style, WS_DLGFRAME)
        debugWinStyle(str, style, WS_VSCROLL)
        debugWinStyle(str, style, WS_HSCROLL)
        debugWinStyle(str, style, WS_SYSMENU)
        debugWinStyle(str, style, WS_THICKFRAME)
        debugWinStyle(str, style, WS_GROUP)
        debugWinStyle(str, style, WS_TABSTOP)
        debugWinStyle(str, style, WS_MINIMIZEBOX)
        debugWinStyle(str, style, WS_MAXIMIZEBOX)
    }
    if (const LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE)) {
        str << " exStyle=" << exStyle;
        debugWinStyle(str, exStyle, WS_EX_DLGMODALFRAME)
        debugWinStyle(str, exStyle, WS_EX_NOPARENTNOTIFY)
        debugWinStyle(str, exStyle, WS_EX_TOPMOST)
        debugWinStyle(str, exStyle, WS_EX_ACCEPTFILES)
        debugWinStyle(str, exStyle, WS_EX_TRANSPARENT)
        debugWinStyle(str, exStyle, WS_EX_MDICHILD)
        debugWinStyle(str, exStyle, WS_EX_TOOLWINDOW)
        debugWinStyle(str, exStyle, WS_EX_WINDOWEDGE)
        debugWinStyle(str, exStyle, WS_EX_CLIENTEDGE)
        debugWinStyle(str, exStyle, WS_EX_CONTEXTHELP)
        debugWinStyle(str, exStyle, WS_EX_RIGHT)
        debugWinStyle(str, exStyle, WS_EX_LEFT)
        debugWinStyle(str, exStyle, WS_EX_RTLREADING)
        debugWinStyle(str, exStyle, WS_EX_LTRREADING)
        debugWinStyle(str, exStyle, WS_EX_LEFTSCROLLBAR)
        debugWinStyle(str, exStyle, WS_EX_RIGHTSCROLLBAR)
        debugWinStyle(str, exStyle, WS_EX_CONTROLPARENT)
        debugWinStyle(str, exStyle, WS_EX_STATICEDGE)
        debugWinStyle(str, exStyle, WS_EX_APPWINDOW)
        debugWinStyle(str, exStyle, WS_EX_LAYERED)
        debugWinStyle(str, exStyle, WS_EX_NOINHERITLAYOUT)
        debugWinStyle(str, exStyle, WS_EX_NOREDIRECTIONBITMAP)
        debugWinStyle(str, exStyle, WS_EX_LAYOUTRTL)
        debugWinStyle(str, exStyle, WS_EX_COMPOSITED)
        debugWinStyle(str, exStyle, WS_EX_NOACTIVATE)
    }

    if (const ULONG_PTR classStyle = GetClassLongPtr(hwnd, GCL_STYLE)) {
        str << " classStyle=" << classStyle;
        debugWinStyle(str, classStyle, CS_BYTEALIGNCLIENT)
        debugWinStyle(str, classStyle, CS_BYTEALIGNWINDOW)
        debugWinStyle(str, classStyle, CS_CLASSDC)
        debugWinStyle(str, classStyle, CS_DBLCLKS)
        debugWinStyle(str, classStyle, CS_DROPSHADOW)
        debugWinStyle(str, classStyle, CS_GLOBALCLASS)
        debugWinStyle(str, classStyle, CS_HREDRAW)
        debugWinStyle(str, classStyle, CS_NOCLOSE)
        debugWinStyle(str, classStyle, CS_OWNDC)
        debugWinStyle(str, classStyle, CS_PARENTDC)
        debugWinStyle(str, classStyle, CS_SAVEBITS)
        debugWinStyle(str, classStyle, CS_VREDRAW)
    }

    if (const ULONG_PTR wndProc = GetClassLongPtr(hwnd, GCLP_WNDPROC))
        str << " wndProc=" << wndProc;

    str << noshowbase << dec;

    if (GetWindowModuleFileName(hwnd, buf, sizeof(buf)/sizeof(buf[0])))
        str << " module=\"" << QString::fromWCharArray(buf) << '"';

    str << '\n';
}

static void dumpNativeWindowRecursion(HWND hwnd, DumpContext *dc);

BOOL CALLBACK dumpWindowEnumChildProc(HWND hwnd, LPARAM lParam)
{
    DumpContext *dumpContext = reinterpret_cast<DumpContext *>(lParam);
    // EnumChildWindows enumerates grand children as well, skip these to
    // get the hierarchical formatting right.
    if (GetAncestor(hwnd, GA_PARENT) == dumpContext->parent)
        dumpNativeWindowRecursion(hwnd, dumpContext);
    return TRUE;
}

static void dumpNativeWindowRecursion(HWND hwnd, DumpContext *dc)
{
    indentStream(*dc->stream, dc->indentation);
    formatNativeWindow(hwnd, *dc->stream);
    DumpContext nextLevel = *dc;
    nextLevel.indentation += 2;
    nextLevel.parent = hwnd;
    EnumChildWindows(hwnd, dumpWindowEnumChildProc, reinterpret_cast<LPARAM>(&nextLevel));
}

/* recurse by Z order, same result */
static void dumpNativeWindowRecursionByZ(HWND hwnd, DumpContext *dc)
{
    indentStream(*dc->stream, dc->indentation);
    formatNativeWindow(hwnd, *dc->stream);
    if (const HWND topChild = GetTopWindow(hwnd)) {
        DumpContext nextLevel = *dc;
        nextLevel.indentation += 2;
        for (HWND child = topChild; child ; child = GetNextWindow(child, GW_HWNDNEXT))
            dumpNativeWindowRecursionByZ(child, &nextLevel);
    }
}

typedef QVector<WId> WIdVector;

static void dumpNativeWindows(const WIdVector& wins)
{
    DumpContext dc;
    QString s;
    dc.stream = QSharedPointer<QTextStream>(new QTextStream(&s));
    foreach (WId win, wins)
        dumpNativeWindowRecursion(reinterpret_cast<HWND>(win), &dc);
#if QT_VERSION >= 0x050400
    qDebug().noquote() << s;
#else
    qDebug("%s", qPrintable(s));
#endif
}

void dumpNativeWindows(WId rootIn)
{
    const WId root = rootIn ? rootIn : reinterpret_cast<WId>(GetDesktopWindow());
    dumpNativeWindows(WIdVector(1, root));
}

BOOL CALLBACK findQtTopLevelEnumChildProc(HWND hwnd, LPARAM lParam)
{
    WIdVector *v = reinterpret_cast<WIdVector *>(lParam);
    wchar_t buf[512];
    if (GetClassName(hwnd, buf, sizeof(buf)/sizeof(buf[0]))) {
        if (wcsstr(buf, L"Qt"))
            v->append(reinterpret_cast<WId>(hwnd));
    }
    return TRUE;
}

static WIdVector findQtTopLevels()
{
    WIdVector result;
    EnumChildWindows(GetDesktopWindow(),
                     findQtTopLevelEnumChildProc,
                     reinterpret_cast<LPARAM>(&result));
    return result;
}

void dumpNativeQtTopLevels()
{
    dumpNativeWindows(findQtTopLevels());
}

} // namespace QtDiag
