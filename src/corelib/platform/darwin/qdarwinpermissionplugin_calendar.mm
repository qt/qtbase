// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinpermissionplugin_p_p.h"

#include <EventKit/EventKit.h>

QT_DEFINE_PERMISSION_STATUS_CONVERTER(EKAuthorizationStatus);

@interface QDarwinCalendarPermissionHandler ()
@property (nonatomic, retain) EKEventStore *eventStore;
@end

@implementation QDarwinCalendarPermissionHandler
- (Qt::PermissionStatus)checkPermission:(QPermission)permission
{
    Q_UNUSED(permission);
    return [self currentStatus];
}

- (Qt::PermissionStatus)currentStatus
{
    const auto status = [EKEventStore authorizationStatusForEntityType:EKEntityTypeEvent];
    return nativeStatusToQtStatus(status);
}

- (QStringList)usageDescriptionsFor:(QPermission)permission
{
    Q_UNUSED(permission);
    return { "NSCalendarsUsageDescription" };
}

- (void)requestPermission:(QPermission)permission withCallback:(PermissionCallback)callback
{
    if (!self.eventStore) {
        // Note: Creating the EKEventStore results in warnings in the
        // console about "An error occurred in the persistent store".
        // This seems like a EventKit API bug.
        self.eventStore = [[EKEventStore new] autorelease];
    }

    [self.eventStore requestAccessToEntityType:EKEntityTypeEvent
        completion:^(BOOL granted, NSError * _Nullable error) {
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
