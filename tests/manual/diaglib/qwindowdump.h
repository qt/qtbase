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

#ifndef _WINDOWDUMP_
#define _WINDOWDUMP_

#include <QtCore/qnamespace.h>

QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QWindow)
QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace QtDiag {

enum FormatWindowOption {
    DontPrintWindowFlags = 0x001,
    PrintSizeConstraints = 0x002
};

Q_DECLARE_FLAGS(FormatWindowOptions, FormatWindowOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(FormatWindowOptions)

void indentStream(QTextStream &s, int indent);
void formatObject(QTextStream &str, const QObject *o);
void formatRect(QTextStream &str, const QRect &geom);
void formatWindowFlags(QTextStream &str, const Qt::WindowFlags flags);

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options = 0);
void dumpAllWindows(FormatWindowOptions options = 0);

} // namespace QtDiag

#endif
