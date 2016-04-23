/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qoperatingsystemversion_p.h"
#import <Foundation/Foundation.h>

#ifdef Q_OS_IOS
#import <UIKit/UIKit.h>
#endif

QT_BEGIN_NAMESPACE

typedef qint16 (*GestaltFunction)(quint32 selector, qint32 *response);

QOperatingSystemVersion QOperatingSystemVersion::current()
{
    QOperatingSystemVersion v;
    v.m_os = currentType();
    v.m_major = -1;
    v.m_minor = -1;
    v.m_micro = -1;
#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_10, __IPHONE_8_0) || defined(Q_OS_TVOS) || defined(Q_OS_WATCHOS)
    if ([NSProcessInfo instancesRespondToSelector:@selector(operatingSystemVersion)]) {
        NSOperatingSystemVersion osv = NSProcessInfo.processInfo.operatingSystemVersion;
        v.m_major = osv.majorVersion;
        v.m_minor = osv.minorVersion;
        v.m_micro = osv.patchVersion;
        return v;
    }
#endif
    // Use temporary variables so we can return 0.0.0 (unknown version)
    // in case of an error partway through determining the OS version
    qint32 major = 0, minor = 0, patch = 0;
#if QT_MACOS_IOS_DEPLOYMENT_TARGET_BELOW(__MAC_10_10, __IPHONE_8_0)
#if defined(Q_OS_IOS)
    @autoreleasepool {
        NSArray *parts = [UIDevice.currentDevice.systemVersion componentsSeparatedByString:@"."];
        major = parts.count > 0 ? [[parts objectAtIndex:0] intValue] : 0;
        minor = parts.count > 1 ? [[parts objectAtIndex:1] intValue] : 0;
        patch = parts.count > 2 ? [[parts objectAtIndex:2] intValue] : 0;
    }
#elif defined(Q_OS_MACOS)
    static GestaltFunction pGestalt = 0;
    if (!pGestalt) {
        CFBundleRef b = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.CoreServices"));
        pGestalt = reinterpret_cast<GestaltFunction>(CFBundleGetFunctionPointerForName(b,
                                                     CFSTR("Gestalt")));
    }
    if (!pGestalt)
        return v;
    if (pGestalt('sys1', &major) != 0)
        return v;
    if (pGestalt('sys2', &minor) != 0)
        return v;
    if (pGestalt('sys3', &patch) != 0)
        return v;
#endif
#endif
    v.m_major = major;
    v.m_minor = minor;
    v.m_micro = patch;
    return v;
}

QT_END_NAMESPACE
