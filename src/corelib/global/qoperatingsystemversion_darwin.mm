// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qoperatingsystemversion_p.h"
#import <Foundation/Foundation.h>

QT_BEGIN_NAMESPACE

QOperatingSystemVersionBase QOperatingSystemVersionBase::current_impl()
{
    NSOperatingSystemVersion osv = NSProcessInfo.processInfo.operatingSystemVersion;
    QOperatingSystemVersionBase v;
    v.m_os = currentType();
    v.m_major = osv.majorVersion;
    v.m_minor = osv.minorVersion;
    v.m_micro = osv.patchVersion;
    return v;
}

QT_END_NAMESPACE
