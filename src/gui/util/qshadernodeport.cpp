/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qshadernodeport_p.h"

QT_BEGIN_NAMESPACE

QShaderNodePort::QShaderNodePort() noexcept
    : direction(Output)
{
}

bool operator==(const QShaderNodePort &lhs, const QShaderNodePort &rhs) noexcept
{
    return lhs.direction == rhs.direction
        && lhs.name == rhs.name;
}

QT_END_NAMESPACE
