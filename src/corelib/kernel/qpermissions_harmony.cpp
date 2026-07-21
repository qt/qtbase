// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpermissions.h"
#include "qpermissions_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qohospermissionshelper_p.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

static QOhosPermissionsHelper *s_permissionsHelper = nullptr;

static QString makeOhosPermissionStr(const char *permissionSubString)
{
    if (permissionSubString == nullptr)
        return QString();

    return QString::fromUtf8("ohos.permission.%1").arg(permissionSubString);
}

static QStringList ohosPermissionStrings(const QPermission &permission)
{
    const auto id = permission.type().id();
    if (id == qMetaTypeId<QCameraPermission>()) {
        return { makeOhosPermissionStr("CAMERA") };
    } else if (id == qMetaTypeId<QBluetoothPermission>()) {
        return { makeOhosPermissionStr("ACCESS_BLUETOOTH") };
    } else if (id == qMetaTypeId<QLocationPermission>()) {
        return { makeOhosPermissionStr("LOCATION") };
    } else if (id == qMetaTypeId<QMicrophonePermission>()) {
        return { makeOhosPermissionStr("MICROPHONE") };
    } else if (id == qMetaTypeId<QCalendarPermission>()) {
        switch (permission.value<QCalendarPermission>()->accessMode()) {
        case QCalendarPermission::AccessMode::ReadOnly:
            return { makeOhosPermissionStr("READ_CALENDAR") };
        case QCalendarPermission::AccessMode::WriteOnly:
            return { makeOhosPermissionStr("WRITE_CALENDAR") };
        case QCalendarPermission::AccessMode::ReadWrite:
            return { makeOhosPermissionStr("READ_CALENDAR"),
                     makeOhosPermissionStr("WRITE_CALENDAR") };
        }
        Q_UNREACHABLE_RETURN({});
    } else if (id == qMetaTypeId<QContactsPermission>()) {
        // The contact permission goes under the category of the restricted permission
        // and this requires special previlege from the application
        switch (permission.value<QContactsPermission>()->accessMode()) {
        case QContactsPermission::AccessMode::ReadOnly:
            return { makeOhosPermissionStr("READ_CONTACTS") };
        case QContactsPermission::AccessMode::ReadWrite:
            return { makeOhosPermissionStr("READ_CONTACTS"),
                     makeOhosPermissionStr("WRITE_CONTACTS") };
        }
        Q_UNREACHABLE_RETURN({});
    }

    return {};
}

static void requestPermissionsFromUser(
    const QStringList &qPermissionNames,
    const QPermissions::Private::PermissionCallback &callback)
{
    auto userPermissionResultConsumer = [=](QList<QOhosPermissionsHelper::PermissionRequestResult> appPermissionResults) {
        Qt::PermissionStatus status = Qt::PermissionStatus::Granted;
        for (const auto &permissionResult : appPermissionResults) {
            if (!permissionResult.permissionGranted) {
                status = permissionResult.dialogShown ? Qt::PermissionStatus::Denied : Qt::PermissionStatus::Undetermined;
                break;
            }
        }
        if (status == Qt::PermissionStatus::Undetermined) {
            auto settingsPermissionResultConsumer = [callback](QList<bool> status) {
                auto granted = std::none_of(status.begin(), status.end(), std::logical_not<>{});
                callback(granted ? Qt::PermissionStatus::Granted : Qt::PermissionStatus::Denied);
            };
            s_permissionsHelper->requestPermissionsOnSettingIfNeeded(
                qPermissionNames, QCoreApplication::instance(), settingsPermissionResultConsumer);
        } else {
            callback(status);
        }
    };

    s_permissionsHelper->requestPermissionsFromUserIfNeeded(
        qPermissionNames, QCoreApplication::instance(), userPermissionResultConsumer);
}

namespace QPermissions::Private
{
    Qt::PermissionStatus checkPermission(const QPermission &permission)
    {
        if (s_permissionsHelper == nullptr)
            return Qt::PermissionStatus::Undetermined;

        const auto permissionStatuses = s_permissionsHelper->checkStatusesOfPermissions(
            ohosPermissionStrings(permission));
        if (permissionStatuses.contains(Qt::PermissionStatus::Denied))
            return Qt::PermissionStatus::Denied;
        if (permissionStatuses.contains(Qt::PermissionStatus::Undetermined))
            return Qt::PermissionStatus::Undetermined;
        return Qt::PermissionStatus::Granted;
    }

    void requestPermission(
        const QPermission &permission,
        const QPermissions::Private::PermissionCallback &callback)
    {
        if (s_permissionsHelper == nullptr) {
            callback(Qt::PermissionStatus::Undetermined);
            return;
        }

        const auto permissionNames = ohosPermissionStrings(permission);
        if (permissionNames.isEmpty())
            callback(Qt::PermissionStatus::Granted);
        else
            requestPermissionsFromUser(permissionNames, callback);
    }
}

void qt_setQOhosPermissionsHelper(QOhosPermissionsHelper *permissionsHelper)
{
    s_permissionsHelper = permissionsHelper;
}

QT_END_NAMESPACE
