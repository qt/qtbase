// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
void formatWindowFlags(QTextStream &str, Qt::WindowFlags flags);

void formatWindow(QTextStream &str, const QWindow *w, FormatWindowOptions options = {});
void dumpAllWindows(FormatWindowOptions options = {});

} // namespace QtDiag

#endif
