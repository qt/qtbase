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

#include "qwindowdump.h"

#if QT_VERSION > 0x050000
#  include <QtGui/QGuiApplication>
#  include <QtGui/QScreen>
#  include <QtGui/QWindow>
#  include <qpa/qplatformwindow.h>
#  include <private/qwindow_p.h>
#  if QT_VERSION >= 0x050600
#    include <private/qhighdpiscaling_p.h>
#  endif
#endif
#include <QtCore/QMetaObject>
#include <QtCore/QRect>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

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
    str << geom.width() << 'x' << geom.height()
        << forcesign << geom.x() << geom.y() << noforcesign;
}

#define debugType(s, type, typeConstant) \
if ((type & typeConstant) == typeConstant) \
    s << ' ' << #typeConstant;

#define debugFlag(s, flags, flagConstant) \
if (flags & flagConstant) \
    s << ' ' << #flagConstant;

void formatWindowFlags(QTextStream &str, const Qt::WindowFlags flags)
{
    str << showbase << hex << unsigned(flags) << dec << noshowbase;
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
#if QT_VERSION > 0x050000
    debugType(str, windowType, Qt::ForeignWindow)
    debugType(str, windowType, Qt::CoverWindow)
#endif
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
#if QT_VERSION > 0x050000
    debugFlag(str, flags, Qt::WindowTransparentForInput)
    debugFlag(str, flags, Qt::WindowOverridesSystemGestures)
    debugFlag(str, flags, Qt::WindowDoesNotAcceptFocus)
    debugFlag(str, flags, Qt::NoDropShadowWindowHint)
    debugFlag(str, flags, Qt::WindowFullscreenButtonHint)
#endif
    debugFlag(str, flags, Qt::WindowStaysOnBottomHint)
    debugFlag(str, flags, Qt::MacWindowToolBarButtonHint)
    debugFlag(str, flags, Qt::BypassGraphicsProxyWidget)
}

#if QT_VERSION > 0x050000

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options)
{
    const QPlatformWindow *pw = w->handle();
    formatObject(str, w);
    str << ' ' << (w->isVisible() ? "[visible] " : "[hidden] ");
    if (const WId nativeWinId = pw ? pw->winId() : WId(0))
        str << "[native: " << hex << showbase << nativeWinId << dec << noshowbase << "] ";
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
#if QT_VERSION >= 0x050600
        if (QHighDpiScaling::isActive())
            str << "factor=" << QHighDpiScaling::factor(w) << " dpr=" << w->devicePixelRatio();
#endif
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
                                FormatWindowOptions options = 0, int depth = 0)
{
    indentStream(str, 2 * depth);
    formatWindow(str, w, options);
    foreach (const QObject *co, w->children()) {
        if (co->isWindowType())
            dumpWindowRecursion(str, static_cast<const QWindow *>(co), options, depth + 1);
    }
}

void dumpAllWindows(FormatWindowOptions options)
{
    QString d;
    QTextStream str(&d);
    str << "### QWindows:\n";
    foreach (QWindow *w, QGuiApplication::topLevelWindows())
        dumpWindowRecursion(str, w, options);
#if QT_VERSION >= 0x050400
    qDebug().noquote() << d;
#else
    qDebug() << d;
#endif
}

#else // Qt 5
class QWindow {};

void formatWindow(QTextStream &, const QWindow *, FormatWindowOptions)
{
}

void dumpAllWindows(FormatWindowOptions options)
{
}

#endif // Qt 4

} // namespace QtDiag
