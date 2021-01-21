/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTSUPPORT_WIDGETS_H
#define QTESTSUPPORT_WIDGETS_H

#include <QtWidgets/qtwidgetsglobal.h>

QT_BEGIN_NAMESPACE

class QWidget;

namespace QTest {
Q_WIDGETS_EXPORT Q_REQUIRED_RESULT bool qWaitForWindowActive(QWidget *widget, int timeout = 5000);
Q_WIDGETS_EXPORT Q_REQUIRED_RESULT bool qWaitForWindowExposed(QWidget *widget, int timeout = 5000);

#if QT_DEPRECATED_SINCE(5, 0)
QT_DEPRECATED Q_REQUIRED_RESULT inline static bool qWaitForWindowShown(QWidget *widget, int timeout = 5000)
{ return QTest::qWaitForWindowExposed(widget, timeout); }
#endif
}

QT_END_NAMESPACE

#endif
