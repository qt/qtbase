// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPERMISSIONS_P_H
#define QPERMISSIONS_P_H

#include "qpermissions.h"

#include <private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>

#include <functional>

QT_REQUIRE_CONFIG(permissions);

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

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPermissions)

namespace QPermissions::Private
{
    using PermissionCallback = std::function<void(Qt::PermissionStatus)>;

    Qt::PermissionStatus checkPermission(const QPermission &permission);
    void requestPermission(const QPermission &permission, const PermissionCallback &callback);
}

QT_END_NAMESPACE

#endif // QPERMISSIONS_P_H
