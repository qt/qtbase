// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinpermissionplugin_p_p.h"

#include <Contacts/Contacts.h>

@interface QDarwinContactsPermissionHandler ()
@property (nonatomic, retain) CNContactStore *contactStore;
@end

@implementation QDarwinContactsPermissionHandler
- (Qt::PermissionStatus)checkPermission:(QPermission)permission
{
    Q_UNUSED(permission);
    return [self currentStatus];
}

- (Qt::PermissionStatus)currentStatus
{
    const auto status = [CNContactStore authorizationStatusForEntityType:CNEntityTypeContacts];
    switch (status) {
    case CNAuthorizationStatusAuthorized:
#if (defined(Q_OS_IOS) && QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(180000)) || defined(Q_OS_VISIONOS)
    case CNAuthorizationStatusLimited:
#endif
        return Qt::PermissionStatus::Granted;
    case CNAuthorizationStatusDenied:
    case CNAuthorizationStatusRestricted:
        return Qt::PermissionStatus::Denied;
    case CNAuthorizationStatusNotDetermined:
        return Qt::PermissionStatus::Undetermined;
    }
    qCWarning(lcPermissions) << "Unknown permission status" << status << "detected in"
        << QT_STRINGIFY(QT_DARWIN_PERMISSION_PLUGIN);
    return Qt::PermissionStatus::Denied;
}

- (QStringList)usageDescriptionsFor:(QPermission)permission
{
    Q_UNUSED(permission);
    return { "NSContactsUsageDescription" };
}

- (void)requestPermission:(QPermission)permission withCallback:(PermissionCallback)callback
{
    if (!self.contactStore) {
        // Note: Creating the CNContactStore results in warnings in the
        // console about "Attempted to register account monitor for types
        // client is not authorized to access", mentioning CardDAV, LDAP,
        // and Exchange. This seems like a Contacts API bug.
        self.contactStore = [[CNContactStore new] autorelease];
    }

    [self.contactStore requestAccessForEntityType:CNEntityTypeContacts
        completionHandler:^(BOOL granted, NSError * _Nullable error) {
            Q_UNUSED(granted); // We use status instead
            // Permission denied will result in an error, which we don't
            // want to report/log, so we ignore the error and just report
            // the status.
            Q_UNUSED(error);

            callback([self currentStatus]);
        }
    ];
}

@end

#include "moc_qdarwinpermissionplugin_p_p.cpp"
