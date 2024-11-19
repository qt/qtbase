// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwindowdump.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QRect>
#include <QtCore/QTextStream>

#include <qpa/qplatformwindow.h>
#include <private/qwindow_p.h>
#include <private/qhighdpiscaling_p.h>

namespace QtDiag {

void indentStream(QTextStream &s, int indent)
{
    for (int i = 0; i < indent; ++i)
        s << ' ';
}

void formatObject(QTextStream &str, const QObject *o)
{
    str << o->metaObject()->className();
    const QString on = o->objectName();
    if (!on.isEmpty())
        str << "/\"" << on << '"';
}

void formatRect(QTextStream &str, const QRect &geom)
{
    str << geom.width() << 'x' << geom.height() << Qt::forcesign
        << geom.x() << geom.y() << Qt::noforcesign;
}

#define debugType(s, type, typeConstant) \
if ((type & typeConstant) == typeConstant) \
    s << ' ' << #typeConstant;

#define debugFlag(s, flags, flagConstant) \
if (flags & flagConstant) \
    s << ' ' << #flagConstant;

void formatWindowFlags(QTextStream &str, Qt::WindowFlags flags)
{
    str << Qt::showbase << Qt::hex << unsigned(flags) << Qt::dec << Qt::noshowbase;
    const Qt::WindowFlags windowType = flags & Qt::WindowType_Mask;
    debugFlag(str, flags, Qt::Window)
    debugType(str, windowType, Qt::Dialog)
    debugType(str, windowType, Qt::Sheet)
    debugType(str, windowType, Qt::Drawer)
    debugType(str, windowType, Qt::Popup)
    debugType(str, windowType, Qt::Tool)
    debugType(str, windowType, Qt::ToolTip)
    debugType(str, windowType, Qt::SplashScreen)
    debugType(str, windowType, Qt::Desktop)
    debugType(str, windowType, Qt::SubWindow)
    debugType(str, windowType, Qt::ForeignWindow)
    debugType(str, windowType, Qt::CoverWindow)
    debugFlag(str, flags, Qt::MSWindowsFixedSizeDialogHint)
    debugFlag(str, flags, Qt::MSWindowsOwnDC)
    debugFlag(str, flags, Qt::X11BypassWindowManagerHint)
    debugFlag(str, flags, Qt::FramelessWindowHint)
    debugFlag(str, flags, Qt::WindowTitleHint)
    debugFlag(str, flags, Qt::WindowSystemMenuHint)
    debugFlag(str, flags, Qt::WindowMinimizeButtonHint)
    debugFlag(str, flags, Qt::WindowMaximizeButtonHint)
    debugFlag(str, flags, Qt::WindowContextHelpButtonHint)
    debugFlag(str, flags, Qt::WindowShadeButtonHint)
    debugFlag(str, flags, Qt::WindowStaysOnTopHint)
    debugFlag(str, flags, Qt::CustomizeWindowHint)
    debugFlag(str, flags, Qt::WindowTransparentForInput)
    debugFlag(str, flags, Qt::WindowOverridesSystemGestures)
    debugFlag(str, flags, Qt::WindowDoesNotAcceptFocus)
    debugFlag(str, flags, Qt::NoDropShadowWindowHint)
    debugFlag(str, flags, Qt::WindowFullscreenButtonHint)
    debugFlag(str, flags, Qt::WindowStaysOnBottomHint)
    debugFlag(str, flags, Qt::BypassGraphicsProxyWidget)
}

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options)
{
    const QPlatformWindow *pw = w->handle();
    formatObject(str, w);
    str << ' ' << (w->isVisible() ? "[visible] " : "[hidden] ");
    if (const WId nativeWinId = pw ? pw->winId() : WId(0))
        str << "[native: " << Qt::hex << Qt::showbase << nativeWinId << Qt::dec << Qt::noshowbase
            << "] ";
    if (w->isTopLevel())
        str << "[top] ";
    if (w->isExposed())
        str << "[exposed] ";
    if (w->surfaceClass() == QWindow::Offscreen)
        str << "[offscreen] ";
    str << "surface=" << w->surfaceType() << ' ';
    if (const Qt::WindowState state = w->windowState())
        str << "windowState=" << state << ' ';
    formatRect(str, w->geometry());
    if (w->isTopLevel()) {
        str << " \"" << w->screen()->name() << "\" ";
        if (QHighDpiScaling::isActive())
            str << "factor=" << QHighDpiScaling::factor(w) << " dpr=" << w->devicePixelRatio();
    }
    if (!(options & DontPrintWindowFlags)) {
        str << ' ';
        formatWindowFlags(str, w->flags());
    }
    if (options & PrintSizeConstraints) {
        str << ' ';
        const QSize minimumSize = w->minimumSize();
        if (minimumSize.width() > 0 || minimumSize.height() > 0)
            str << "minimumSize=" << minimumSize.width() << 'x' << minimumSize.height() << ' ';
        const QSize maximumSize = w->maximumSize();
        if (maximumSize.width() < QWINDOWSIZE_MAX || maximumSize.height() < QWINDOWSIZE_MAX)
            str << "maximumSize=" << maximumSize.width() << 'x' << maximumSize.height() << ' ';
    }
    str << '\n';
}

static void dumpWindowRecursion(QTextStream &str, const QWindow *w,
                                FormatWindowOptions options = {}, int depth = 0)
{
    indentStream(str, 2 * depth);
    formatWindow(str, w, options);
    for (const QObject *co : w->children()) {
        if (co->isWindowType())
            dumpWindowRecursion(str, static_cast<const QWindow *>(co), options, depth + 1);
    }
}

void dumpAllWindows(FormatWindowOptions options)
{
    QString d;
    QTextStream str(&d);
    str << "### QWindows:\n";
    for (QWindow *w : QGuiApplication::topLevelWindows())
        dumpWindowRecursion(str, w, options);
    qDebug().noquote() << d;
}

} // namespace QtDiag
