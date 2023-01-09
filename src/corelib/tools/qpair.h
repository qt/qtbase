/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPAIR_H
#define QPAIR_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if 0
#pragma qt_class(QPair)
#endif

template <typename T1, typename T2>
using QPair = std::pair<T1, T2>;

template <typename T1, typename T2>
constexpr decltype(auto) qMakePair(T1 &&value1, T2 &&value2)
    noexcept(noexcept(std::make_pair(std::forward<T1>(value1), std::forward<T2>(value2))))
{
    return std::make_pair(std::forward<T1>(value1), std::forward<T2>(value2));
}

QT_END_NAMESPACE

#endif // QPAIR_H
