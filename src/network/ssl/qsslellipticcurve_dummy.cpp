/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qsslellipticcurve.h"

QT_BEGIN_NAMESPACE

QString QSslEllipticCurve::shortName() const
{
    return QString();
}

QString QSslEllipticCurve::longName() const
{
    return QString();
}

QSslEllipticCurve QSslEllipticCurve::fromShortName(const QString &name)
{
    Q_UNUSED(name);
    return QSslEllipticCurve();
}

QSslEllipticCurve QSslEllipticCurve::fromLongName(const QString &name)
{
    Q_UNUSED(name);
    return QSslEllipticCurve();
}

bool QSslEllipticCurve::isTlsNamedCurve() const noexcept
{
    return false;
}

QT_END_NAMESPACE
