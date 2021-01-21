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

#ifndef QTTESTGLOBAL_H
#define QTTESTGLOBAL_H

#include <QtCore/qglobal.h>
#include <QtTest/qttestlib-config.h>

QT_BEGIN_NAMESPACE

#if defined(QT_STATIC)
# define Q_TESTLIB_EXPORT
#else
# ifdef QT_BUILD_TESTLIB_LIB
#  define Q_TESTLIB_EXPORT Q_DECL_EXPORT
# else
#  define Q_TESTLIB_EXPORT Q_DECL_IMPORT
# endif
#endif

#if (defined Q_CC_HPACC) && (defined __ia64)
# ifdef Q_TESTLIB_EXPORT
#  undef Q_TESTLIB_EXPORT
# endif
# define Q_TESTLIB_EXPORT
#endif

#define QTEST_VERSION     QT_VERSION
#define QTEST_VERSION_STR QT_VERSION_STR

namespace QTest
{
    enum TestFailMode { Abort = 1, Continue = 2 };
}

QT_END_NAMESPACE

#endif
