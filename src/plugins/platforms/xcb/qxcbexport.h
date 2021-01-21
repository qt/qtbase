/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

