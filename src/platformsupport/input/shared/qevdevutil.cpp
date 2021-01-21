/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qevdevutil_p.h"

QT_BEGIN_NAMESPACE

namespace QEvdevUtil {

ParsedSpecification parseSpecification(const QString &specification)
{
    ParsedSpecification result;

    result.args = specification.splitRef(QLatin1Char(':'));

    for (const QStringRef &arg : qAsConst(result.args)) {
        if (arg.startsWith(QLatin1String("/dev/"))) {
            // if device is specified try to use it
            result.devices.append(arg.toString());
        } else {
            // build new specification without /dev/ elements
            result.spec += arg + QLatin1Char(':');
        }
    }

    if (!result.spec.isEmpty())
        result.spec.chop(1); // remove trailing ':'

    return result;
}

} // namespace QEvdevUtil

QT_END_NAMESPACE
