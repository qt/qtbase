// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinpermissionplugin_p_p.h"

#include <AVFoundation/AVFoundation.h>

QT_DEFINE_PERMISSION_STATUS_CONVERTER(AVAuthorizationStatus);

#ifndef BUILDING_PERMISSION_REQUEST

@implementation QDarwinMicrophonePermissionHandler
- (Qt::PermissionStatus)checkPermission:(QPermission)permission
{
    const auto status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
    return nativeStatusToQtStatus(status);
}

- (QStringList)usageDescriptionsFor:(QPermission)permission
{
    Q_UNUSED(permission);
    return { "NSMicrophoneUsageDescription" };
}
@end

#include "moc_qdarwinpermissionplugin_p_p.cpp"

#else // Building request

@implementation QDarwinMicrophonePermissionHandler (Request)
- (void)requestPermission:(QPermission)permission withCallback:(PermissionCallback)callback
{
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted)
    {
        Q_UNUSED(granted); // We use status instead
        const auto status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
        callback(nativeStatusToQtStatus(status));
    }];
}
@end

#endif // BUILDING_PERMISSION_REQUEST
