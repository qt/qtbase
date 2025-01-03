// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qprocess_p.h>

#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

QProcessEnvironment QProcessEnvironment::systemEnvironment()
{
    __block QProcessEnvironment env;
    [[[NSProcessInfo processInfo] environment]
        enumerateKeysAndObjectsUsingBlock:^(NSString *name, NSString *value, BOOL *__unused stop) {
        env.d->vars.insert(
            QProcessEnvironmentPrivate::Key(QString::fromNSString(name).toLocal8Bit()),
            QProcessEnvironmentPrivate::Value(QString::fromNSString(value).toLocal8Bit()));
    }];
    return env;
}

QT_END_NAMESPACE
