/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSCOPEDVALUEROLLBACK_H
#define QSCOPEDVALUEROLLBACK_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

template <typename T>
class
#if QT_HAS_CPP_ATTRIBUTE(nodiscard) && __cplusplus >= 201703L
[[nodiscard]]
#endif
QScopedValueRollback
{
public:
    explicit QScopedValueRollback(T &var)
        : varRef(var), oldValue(var)
    {
    }

    explicit QScopedValueRollback(T &var, T value)
        : varRef(var), oldValue(std::move(var))
    {
        varRef = std::move(value);
    }

    ~QScopedValueRollback()
    {
        varRef = std::move(oldValue);
    }

    void commit()
    {
        oldValue = varRef;
    }

private:
    T &varRef;
    T oldValue;

    Q_DISABLE_COPY(QScopedValueRollback)
};

QT_END_NAMESPACE

#endif // QSCOPEDVALUEROLLBACK_H
