/****************************************************************************
 **
 ** Copyright (C) 2015 The Qt Company Ltd.
 ** Copyright (C) 2014 Petroules Corporation.
 ** Contact: http://www.qt.io/licensing/
 **
 ** This file is part of the QtCore module of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL21$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and The Qt Company. For licensing terms
 ** and conditions see http://www.qt.io/terms-conditions. For further
 ** information use the contact form at http://www.qt.io/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 or version 3 as published by the Free
 ** Software Foundation and appearing in the file LICENSE.LGPLv21 and
 ** LICENSE.LGPLv3 included in the packaging of this file. Please review the
 ** following information to ensure the GNU Lesser General Public License
 ** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
 ** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** As a special exception, The Qt Company gives you certain additional
 ** rights. These rights are described in The Qt Company LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include <private/qcore_mac_p.h>

#include <qdebug.h>

#ifdef Q_OS_IOS
#import <UIKit/UIKit.h>
#endif

QT_BEGIN_NAMESPACE

typedef qint16 (*GestaltFunction)(quint32 selector, qint32 *response);

NSString *QCFString::toNSString(const QString &string)
{
    // The const cast below is safe: CfStringRef is immutable and so is NSString.
    return [const_cast<NSString *>(reinterpret_cast<const NSString *>(toCFStringRef(string))) autorelease];
}

QString QCFString::toQString(const NSString *nsstr)
{
    return toQString(reinterpret_cast<CFStringRef>(nsstr));
}

// -------------------------------------------------------------------------

QDebug operator<<(QDebug dbg, const NSObject *nsObject)
{
    return dbg << (nsObject ? nsObject.description.UTF8String : "NSObject(0x0)");
}

QDebug operator<<(QDebug dbg, CFStringRef stringRef)
{
    if (!stringRef)
        return dbg << "CFStringRef(0x0)";

    if (const UniChar *chars = CFStringGetCharactersPtr(stringRef))
        dbg << QString::fromRawData(reinterpret_cast<const QChar *>(chars), CFStringGetLength(stringRef));
    else
        dbg << QString::fromCFString(stringRef);

    return dbg;
}

// Prevents breaking the ODR in case we introduce support for more types
// later on, and lets the user override our default QDebug operators.
#define QT_DECLARE_WEAK_QDEBUG_OPERATOR_FOR_CF_TYPE(CFType) \
    __attribute__((weak)) Q_DECLARE_QDEBUG_OPERATOR_FOR_CF_TYPE(CFType)

QT_FOR_EACH_CORE_FOUNDATION_TYPE(QT_DECLARE_WEAK_QDEBUG_OPERATOR_FOR_CF_TYPE);
QT_FOR_EACH_MUTABLE_CORE_FOUNDATION_TYPE(QT_DECLARE_WEAK_QDEBUG_OPERATOR_FOR_CF_TYPE);
QT_FOR_EACH_CORE_GRAPHICS_TYPE(QT_DECLARE_WEAK_QDEBUG_OPERATOR_FOR_CF_TYPE);
QT_FOR_EACH_MUTABLE_CORE_GRAPHICS_TYPE(QT_DECLARE_WEAK_QDEBUG_OPERATOR_FOR_CF_TYPE);

// -------------------------------------------------------------------------

QAppleOperatingSystemVersion qt_apple_os_version()
{
    QAppleOperatingSystemVersion v = {0, 0, 0};
#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_10, __IPHONE_8_0)
    if ([NSProcessInfo instancesRespondToSelector:@selector(operatingSystemVersion)]) {
        NSOperatingSystemVersion osv = NSProcessInfo.processInfo.operatingSystemVersion;
        v.major = osv.majorVersion;
        v.minor = osv.minorVersion;
        v.patch = osv.patchVersion;
        return v;
    }
#endif
    // Use temporary variables so we can return 0.0.0 (unknown version)
    // in case of an error partway through determining the OS version
    qint32 major = 0, minor = 0, patch = 0;
#if defined(Q_OS_IOS)
    @autoreleasepool {
        NSArray *parts = [UIDevice.currentDevice.systemVersion componentsSeparatedByString:@"."];
        major = parts.count > 0 ? [[parts objectAtIndex:0] intValue] : 0;
        minor = parts.count > 1 ? [[parts objectAtIndex:1] intValue] : 0;
        patch = parts.count > 2 ? [[parts objectAtIndex:2] intValue] : 0;
    }
#elif defined(Q_OS_OSX)
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
    v.major = major;
    v.minor = minor;
    v.patch = patch;
    return v;
}

QT_END_NAMESPACE

