// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TEXTDUMP_H
#define TEXTDUMP_H

#include <QtCore/QtGlobal>

QT_FORWARD_DECLARE_CLASS(QString)

namespace QtDiag {

QString dumpText(const QString &text);
QString dumpTextAsCode(const QString &text);

} // namespace QtDiag

#endif // TEXTDUMP_H
