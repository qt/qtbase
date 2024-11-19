// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevdevutil_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QEvdevUtil {

ParsedSpecification parseSpecification(const QString &specification)
{
    ParsedSpecification result;

    result.args = QStringView{specification}.split(u':');

    for (const auto &arg : std::as_const(result.args)) {
        if (arg.startsWith("/dev/"_L1)) {
            // if device is specified try to use it
            result.devices.append(arg.toString());
#ifdef Q_OS_VXWORKS
        } else if (arg.startsWith("/input/"_L1)) {
            result.devices.append(arg.toString());
#endif
        } else {
            // build new specification without /dev/ elements
            result.spec += arg + u':';
        }
    }

    if (!result.spec.isEmpty())
        result.spec.chop(1); // remove trailing ':'

    return result;
}

} // namespace QEvdevUtil

QT_END_NAMESPACE
