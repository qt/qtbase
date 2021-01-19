/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
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

#ifndef QTCONCURRENT_COMPILERTEST_H
#define QTCONCURRENT_COMPILERTEST_H

#include <QtConcurrent/qtconcurrent_global.h>

#ifndef QT_NO_CONCURRENT

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template<class T>
class HasResultType {
    typedef char Yes;
    typedef void *No;
    template<typename U> static Yes test(int, const typename U::result_type * = nullptr);
    template<typename U> static No test(double);
public:
    enum { Value = (sizeof(test<T>(0)) == sizeof(Yes)) };
};

}

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
