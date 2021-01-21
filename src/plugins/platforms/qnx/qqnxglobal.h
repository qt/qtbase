/***************************************************************************
**
** Copyright (C) 2011 - 2014 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXGLOBAL_H
#define QQNXGLOBAL_H

#include <qglobal.h>

QT_BEGIN_NAMESPACE

void qScreenCheckError(int rc, const char *funcInfo, const char *message, bool critical);

#define Q_SCREEN_CHECKERROR(x, message) \
qScreenCheckError(x, Q_FUNC_INFO, message, false)

#define Q_SCREEN_CRITICALERROR(x, message) \
qScreenCheckError(x, Q_FUNC_INFO, message, true)

QT_END_NAMESPACE

#endif // QQNXGLOBAL_H
