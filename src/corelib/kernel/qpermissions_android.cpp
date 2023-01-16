// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpermissions.h"
#include "qpermissions_p.h"

#include <QtCore/qstringlist.h>
#include <QtCore/qfuture.h>
#include <QtCore/qhash.h>

#include "private/qandroidextras_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QStringList nativeLocationPermission(const QLocationPermission &permission)
{
    QStringList nativeLocationPermissionList;
    const int sdkVersion = QtAndroidPrivate::androidSdkVersion();
    static QString backgroundLocation = u"android.permission.ACCESS_BACKGROUND_LOCATION"_s;
    static QString fineLocation = u"android.permission.ACCESS_FINE_LOCATION"_s;
    static QString coarseLocation = u"android.permission.ACCESS_COARSE_LOCATION"_s;

    // Since Android API 30, background location cannot be requested along
    // with fine or coarse location, but it should be requested separately after
    // the latter have been granted, see
    // https://developer.android.com/training/location/permissions
    if (sdkVersion < 30 || permission.availability() == QLocationPermission::WhenInUse) {
        if (permission.accuracy() == QLocationPermission::Approximate) {
            nativeLocationPermissionList << coarseLocation;
        } else {
            nativeLocationPermissionList << fineLocation;
            // Since Android API 31, if precise location is requested, it's advised
            // to request both fine and coarse location permissions, see
            // https://developer.android.com/training/location/permissions#approximate-request
            if (sdkVersion >= 31)
                nativeLocationPermissionList << coarseLocation;
        }
    }

    // NOTE: before Android API 29, background permission doesn't exist yet.

    // Keep the background permission in front to be able to use first()
    // on the list in checkPermission() because it takes single permission.
    if (sdkVersion >= 29 && permission.availability() == QLocationPermission::Always)
        nativeLocationPermissionList.prepend(backgroundLocation);

    return nativeLocationPermissionList;
}

static QStringList nativeStringsFromPermission(const QPermission &permission)
{
    const auto id = permission.type().id();
    if (id == qMetaTypeId<QLocationPermission>()) {
        return nativeLocationPermission(*permission.value<QLocationPermission>());
    } else if (id == qMetaTypeId<QCameraPermission>()) {
        return { u"android.permission.CAMERA"_s };
    } else if (id == qMetaTypeId<QMicrophonePermission>()) {
        return { u"android.permission.RECORD_AUDIO"_s };
    } else if (id == qMetaTypeId<QBluetoothPermission>()) {
        // TODO: handle Android 12 new bluetooth permissions
        return { u"android.permission.BLUETOOTH"_s };
    } else if (id == qMetaTypeId<QContactsPermission>()) {
        const auto readContactsString = u"android.permission.READ_CONTACTS"_s;
        switch (permission.value<QContactsPermission>()->accessMode()) {
        case QContactsPermission::AccessMode::ReadOnly:
            return { readContactsString };
        case QContactsPermission::AccessMode::ReadWrite:
            return { readContactsString, u"android.permission.WRITE_CONTACTS"_s };
        }
        Q_UNREACHABLE_RETURN({});
    } else if (id == qMetaTypeId<QCalendarPermission>()) {
        const auto readContactsString = u"android.permission.READ_CALENDAR"_s;
        switch (permission.value<QCalendarPermission>()->accessMode()) {
        case QCalendarPermission::AccessMode::ReadOnly:
            return { readContactsString };
        case QCalendarPermission::AccessMode::ReadWrite:
            return { readContactsString, u"android.permission.WRITE_CALENDAR"_s };
        }
        Q_UNREACHABLE_RETURN({});
    }

    return {};
}

static Qt::PermissionStatus
permissionStatusForAndroidResult(QtAndroidPrivate::PermissionResult result)
{
    switch (result) {
    case QtAndroidPrivate::PermissionResult::Authorized: return Qt::PermissionStatus::Granted;
    case QtAndroidPrivate::PermissionResult::Denied: return Qt::PermissionStatus::Denied;
    default: return Qt::PermissionStatus::Undetermined;
    }
}

using PermissionStatusHash = QHash<int, Qt::PermissionStatus>;
Q_GLOBAL_STATIC_WITH_ARGS(PermissionStatusHash, g_permissionStatusHash, ({
        { qMetaTypeId<QCameraPermission>(), Qt::PermissionStatus::Undetermined },
        { qMetaTypeId<QMicrophonePermission>(), Qt::PermissionStatus::Undetermined },
        { qMetaTypeId<QBluetoothPermission>(), Qt::PermissionStatus::Undetermined },
        { qMetaTypeId<QContactsPermission>(), Qt::PermissionStatus::Undetermined },
        { qMetaTypeId<QCalendarPermission>(), Qt::PermissionStatus::Undetermined },
        { qMetaTypeId<QLocationPermission>(), Qt::PermissionStatus::Undetermined }
}));

namespace QPermissions::Private
{
    Qt::PermissionStatus checkPermission(const QPermission &permission)
    {
        const auto nativePermissionList = nativeStringsFromPermission(permission);
        if (nativePermissionList.isEmpty())
            return Qt::PermissionStatus::Granted;

        const auto result = QtAndroidPrivate::checkPermission(nativePermissionList.first()).result();
        const auto status = permissionStatusForAndroidResult(result);
        const auto it = g_permissionStatusHash->constFind(permission.type().id());
        const bool foundStatus = (it != g_permissionStatusHash->constEnd());
        const bool itUndetermined = foundStatus && (*it) == Qt::PermissionStatus::Undetermined;
        if (status == Qt::PermissionStatus::Denied && itUndetermined)
            return Qt::PermissionStatus::Undetermined;
        return status;
    }

    void requestPermission(const QPermission &permission,
                           const QPermissions::Private::PermissionCallback &callback)
    {
        const auto nativePermissionList = nativeStringsFromPermission(permission);
        if (nativePermissionList.isEmpty()) {
            callback(Qt::PermissionStatus::Granted);
            return;
        }

        QtAndroidPrivate::requestPermissions(nativePermissionList).then(qApp,
            [callback, permission](QFuture<QtAndroidPrivate::PermissionResult> future) {
                const auto result = future.isValid() ? future.result() : QtAndroidPrivate::Denied;
                const auto status = permissionStatusForAndroidResult(result);
                g_permissionStatusHash->insert(permission.type().id(), status);
                callback(status);
            }
        );
    }
}

QT_END_NAMESPACE
