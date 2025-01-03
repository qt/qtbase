// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBEXPORT_H
#define QXCBEXPORT_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#  if defined(QT_BUILD_XCB_PLUGIN)
#    define Q_XCB_EXPORT Q_DECL_EXPORT
#  else
#    define Q_XCB_EXPORT Q_DECL_IMPORT
#  endif

QT_END_NAMESPACE
#endif //QXCBEXPORT_H

