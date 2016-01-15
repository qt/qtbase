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

#include "qwidgetdump.h"

#include <QWidget>
#if QT_VERSION > 0x050000
#  include <QtGui/QScreen>
#  include <QtGui/QWindow>
#endif
#include <QApplication>
#include <QtCore/QMetaObject>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>

namespace QtDiag {

static void dumpWidgetRecursion(QTextStream &str, const QWidget *w,
                                FormatWindowOptions options, int depth = 0)
{
    indentStream(str, 2 * depth);
    formatObject(str, w);
    str << ' ' << (w->isVisible() ? "[visible] " : "[hidden] ");
    if (const WId nativeWinId = w->internalWinId())
        str << "[native: " << hex << showbase << nativeWinId << dec << noshowbase << "] ";
    if (w->isWindow())
        str << "[top] ";
    str << (w->testAttribute(Qt::WA_Mapped) ? "[mapped] " : "[not mapped] ");
    if (w->testAttribute(Qt::WA_DontCreateNativeAncestors))
        str << "[NoNativeAncestors] ";
    if (const int states = w->windowState())
        str << "windowState=" << hex << showbase << states << dec << noshowbase << ' ';
    formatRect(str, w->geometry());
    if (!(options & DontPrintWindowFlags)) {
        str << ' ';
        formatWindowFlags(str, w->windowFlags());
    }
    str << '\n';
#if QT_VERSION > 0x050000
    if (const QWindow *win = w->windowHandle()) {
        indentStream(str, 2 * (1 + depth));
        formatWindow(str, win, options);
        str << '\n';
    }
#endif // Qt 5
    foreach (const QObject *co, w->children()) {
        if (co->isWidgetType())
            dumpWidgetRecursion(str, static_cast<const QWidget *>(co), options, depth + 1);
    }
}

void dumpAllWidgets(FormatWindowOptions options)
{
    QString d;
    QTextStream str(&d);
    str << "### QWidgets:\n";
    foreach (QWidget *tw, QApplication::topLevelWidgets())
        dumpWidgetRecursion(str, tw, options);
#if QT_VERSION >= 0x050400
    qDebug().noquote() << d;
#else
    qDebug("%s", qPrintable(d));
#endif
}

} // namespace QtDiag
