// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FUNCTION_WITH_ATTRIBUTES_H
#define FUNCTION_WITH_ATTRIBUTES_H
#include <qobject.h>

// test support for gcc attributes with functions

#if defined(Q_CC_GNU) || defined(Q_MOC_RUN)
#define DEPRECATED1 __attribute__ ((__deprecated__))
#else
#define DEPRECATED1
#endif

#if defined(Q_CC_MSVC) || defined(Q_MOC_RUN)
#define DEPRECATED2 __declspec(deprecated)
#else
#define DEPRECATED2
#endif

class FunctionWithAttributes : public QObject
{
    Q_OBJECT
public slots:
    DEPRECATED1 void test1() {}
    DEPRECATED2 void test2() {}

};
#endif // FUNCTION_WITH_ATTRIBUTES_H
