/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QTCONCURRENT_COMPILERTEST_H
#define QTCONCURRENT_COMPILERTEST_H

#include <QtConcurrent/qtconcurrent_global.h>

#ifndef QT_NO_CONCURRENT

QT_BEGIN_NAMESPACE

namespace QtPrivate {

    template <class T, typename = void>
    struct IsIterable : std::false_type {};
    template <class T>
    struct IsIterable<T, std::void_t<decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end())>>
        : std::true_type
    { };

    template <class T>
    inline constexpr bool IsIterableValue = IsIterable<T>::value;

    template <class T, typename = void>
    struct IsDereferenceable : std::false_type {};
    template <class T>
    struct IsDereferenceable<T, std::void_t<decltype(*std::declval<T>())>>
        : std::true_type
    { };

    template <class T>
    inline constexpr bool IsDereferenceableValue = IsDereferenceable<T>::value;
}

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
