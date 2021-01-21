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

#ifndef QTESTASSERT_H
#define QTESTASSERT_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE


#define QTEST_ASSERT(cond) do { if (!(cond)) qt_assert(#cond,__FILE__,__LINE__); } while (false)

#define QTEST_ASSERT_X(cond, where, what) do { if (!(cond)) qt_assert_x(where, what,__FILE__,__LINE__); } while (false)

QT_END_NAMESPACE

#endif
