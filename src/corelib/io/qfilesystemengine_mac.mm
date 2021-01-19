/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qplatformdefs.h"
#include "qfilesystemengine_p.h"
#include "qfile.h"
#include "qurl.h"

#include <QtCore/private/qcore_mac_p.h>
#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

/*
    This implementation does not enable the "put back" option in Finder
    for the trashed object. The only way to get this is to use Finder automation,
    which would query the user for permission to access Finder using a modal,
    blocking dialog - which we definitely can't have in a console application.

    Using Finder would also play the trash sound, which we don't want either in
    such a core API; applications that want that can play the sound themselves.
*/
//static
bool QFileSystemEngine::moveFileToTrash(const QFileSystemEntry &source,
                                        QFileSystemEntry &newLocation, QSystemError &error)
{
#ifdef Q_OS_MACOS // desktop macOS has a trash can
    QMacAutoReleasePool pool;

    QFileInfo info(source.filePath());
    NSString *filepath = info.filePath().toNSString();
    NSURL *fileurl = [NSURL fileURLWithPath:filepath isDirectory:info.isDir()];
    NSURL *resultingUrl = nil;
    NSError *nserror = nil;
    NSFileManager *fm = [NSFileManager defaultManager];
    if ([fm trashItemAtURL:fileurl resultingItemURL:&resultingUrl error:&nserror] != YES) {
        error = QSystemError(nserror.code, QSystemError::NativeError);
        return false;
    }
    newLocation = QFileSystemEntry(QUrl::fromNSURL(resultingUrl).path());
    return true;
#else // watch, tv, iOS don't have a trash can
    Q_UNUSED(source);
    Q_UNUSED(newLocation);
    Q_UNUSED(error);
    return false;
#endif
}

QT_END_NAMESPACE
