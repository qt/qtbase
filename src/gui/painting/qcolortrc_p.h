// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRC_P_H
#define QCOLORTRC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include "qcolortransferfunction_p.h"
#include "qcolortransfertable_p.h"

QT_BEGIN_NAMESPACE


// Defines an ICC TRC (Tone Reproduction Curve)
class Q_GUI_EXPORT QColorTrc
{
public:
    QColorTrc() noexcept : m_type(Type::Uninitialized)
    { }
    QColorTrc(const QColorTransferFunction &fun) : m_type(Type::Function), m_fun(fun)
    { }
    QColorTrc(const QColorTransferTable &table) : m_type(Type::Table), m_table(table)
    { }

    enum class Type {
        Uninitialized,
        Function,
        Table
    };

    bool isLinear() const
    {
        return m_type == Type::Uninitialized || (m_type == Type::Function && m_fun.isLinear());
    }
    bool isValid() const
    {
        return m_type != Type::Uninitialized;
    }
    float apply(float x) const
    {
        if (m_type == Type::Table)
            return m_table.apply(x);
        if (m_type == Type::Function)
            return m_fun.apply(x);
        return x;
    }
    float applyExtended(float x) const
    {
        if (x >= 0.0f && x <= 1.0f)
            return apply(x);
        if (m_type == Type::Function)
            return std::copysign(m_fun.apply(std::abs(x)), x);
        if (m_type == Type::Table)
            return x < 0.0f ? 0.0f : 1.0f;
        return x;
    }
    float applyInverse(float x) const
    {
        if (m_type == Type::Table)
            return m_table.applyInverse(x);
        if (m_type == Type::Function)
            return m_fun.inverted().apply(x);
        return x;
    }
    float applyInverseExtended(float x) const
    {
        if (x >= 0.0f && x <= 1.0f)
            return applyInverse(x);
        if (m_type == Type::Function)
            return std::copysign(applyInverse(std::abs(x)), x);
        if (m_type == Type::Table)
            return x < 0.0f ? 0.0f : 1.0f;
        return x;
    }

    friend inline bool operator!=(const QColorTrc &o1, const QColorTrc &o2);
    friend inline bool operator==(const QColorTrc &o1, const QColorTrc &o2);

    Type m_type;
    QColorTransferFunction m_fun;
    QColorTransferTable m_table;
};

inline bool operator!=(const QColorTrc &o1, const QColorTrc &o2)
{
    if (o1.m_type != o2.m_type)
        return true;
    if (o1.m_type == QColorTrc::Type::Function)
        return o1.m_fun != o2.m_fun;
    if (o1.m_type == QColorTrc::Type::Table)
        return o1.m_table != o2.m_table;
    return false;
}
inline bool operator==(const QColorTrc &o1, const QColorTrc &o2)
{
    return !(o1 != o2);
}

QT_END_NAMESPACE

#endif // QCOLORTRC
