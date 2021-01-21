/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTESTSUPPORT_GUI_H
#define QTESTSUPPORT_GUI_H

#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

class QWindow;

namespace QTest {
Q_GUI_EXPORT Q_REQUIRED_RESULT bool qWaitForWindowActive(QWindow *window, int timeout = 5000);
Q_GUI_EXPORT Q_REQUIRED_RESULT bool qWaitForWindowExposed(QWindow *window, int timeout = 5000);
}

QT_END_NAMESPACE

#endif
