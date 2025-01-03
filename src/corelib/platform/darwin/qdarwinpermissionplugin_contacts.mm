// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinpermissionplugin_p_p.h"

#include <Contacts/Contacts.h>

QT_DEFINE_PERMISSION_STATUS_CONVERTER(CNAuthorizationStatus);

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
    return nativeStatusToQtStatus(status);
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
