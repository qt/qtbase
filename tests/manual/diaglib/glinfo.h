// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef _GLINFO_
#define _GLINFO_

#include <QtCore/QtGlobal>

QT_FORWARD_DECLARE_CLASS(QObject)
QT_FORWARD_DECLARE_CLASS(QString)

namespace QtDiag {

QString glInfo(const QObject *o);

} // namespace QtDiag

#endif
