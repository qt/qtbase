// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPERMISSIONSHELPER_P_H
#define QOHOSPERMISSIONSHELPER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QOhosPermissionsHelper
{
public:
    struct PermissionRequestResult
    {
        bool permissionGranted = false;
        bool dialogShown = false;
    };

    virtual ~QOhosPermissionsHelper() = default;

    virtual QList<Qt::PermissionStatus> checkStatusesOfPermissions(const QStringList &permissionNames) const = 0;

    virtual void requestPermissionsFromUserIfNeeded(
        const QStringList &permissionNames, QObject *resultConsumerContext,
        QOhosConsumer<QList<PermissionRequestResult>> resultConsumer) = 0;

    virtual void requestPermissionsOnSettingIfNeeded(
        const QStringList &permissionNames, QObject *resultConsumerContext,
        QOhosConsumer<QList<bool>> resultConsumer) = 0;

protected:
    QOhosPermissionsHelper() = default;

private:
    Q_DISABLE_COPY_MOVE(QOhosPermissionsHelper)
};

Q_CORE_EXPORT void qt_setQOhosPermissionsHelper(QOhosPermissionsHelper *permissionsHelper);

QT_END_NAMESPACE

#endif
