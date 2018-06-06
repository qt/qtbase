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

#ifndef QCORE_MAC_P_H
#define QCORE_MAC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef __IMAGECAPTURE__
#  define __IMAGECAPTURE__
#endif

#if defined(QT_BOOTSTRAPPED)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "private/qglobal_p.h"

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#endif

#include "qstring.h"
#include "qscopedpointer.h"

#if defined( __OBJC__) && defined(QT_NAMESPACE)
#define QT_NAMESPACE_ALIAS_OBJC_CLASS(__KLASS__) @compatibility_alias __KLASS__ QT_MANGLE_NAMESPACE(__KLASS__)
#else
#define QT_NAMESPACE_ALIAS_OBJC_CLASS(__KLASS__)
#endif

QT_BEGIN_NAMESPACE
template <typename T, typename U, U (*RetainFunction)(U), void (*ReleaseFunction)(U)>
class QAppleRefCounted
{
public:
    QAppleRefCounted(const T &t = T()) : value(t) {}
    QAppleRefCounted(QAppleRefCounted &&other) : value(other.value) { other.value = T(); }
    QAppleRefCounted(const QAppleRefCounted &other) : value(other.value) { if (value) RetainFunction(value); }
    ~QAppleRefCounted() { if (value) ReleaseFunction(value); }
    operator T() { return value; }
    void swap(QAppleRefCounted &other) Q_DECL_NOEXCEPT_EXPR(noexcept(qSwap(value, other.value)))
    { qSwap(value, other.value); }
    QAppleRefCounted &operator=(const QAppleRefCounted &other)
    { QAppleRefCounted copy(other); swap(copy); return *this; }
    QAppleRefCounted &operator=(QAppleRefCounted &&other)
    { QAppleRefCounted moved(std::move(other)); swap(moved); return *this; }
    T *operator&() { return &value; }
protected:
    T value;
};


#ifdef Q_OS_MACOS
class QMacRootLevelAutoReleasePool
{
public:
    QMacRootLevelAutoReleasePool();
    ~QMacRootLevelAutoReleasePool();
private:
    QScopedPointer<QMacAutoReleasePool> pool;
};
#endif

/*
    Helper class that automates refernce counting for CFtypes.
    After constructing the QCFType object, it can be copied like a
    value-based type.

    Note that you must own the object you are wrapping.
    This is typically the case if you get the object from a Core
    Foundation function with the word "Create" or "Copy" in it. If
    you got the object from a "Get" function, either retain it or use
    constructFromGet(). One exception to this rule is the
    HIThemeGet*Shape functions, which in reality are "Copy" functions.
*/
template <typename T>
class QCFType : public QAppleRefCounted<T, CFTypeRef, CFRetain, CFRelease>
{
public:
    using QAppleRefCounted<T, CFTypeRef, CFRetain, CFRelease>::QAppleRefCounted;
    template <typename X> X as() const { return reinterpret_cast<X>(this->value); }
    static QCFType constructFromGet(const T &t)
    {
        if (t)
            CFRetain(t);
        return QCFType<T>(t);
    }
};

class Q_CORE_EXPORT QCFString : public QCFType<CFStringRef>
{
public:
    inline QCFString(const QString &str) : QCFType<CFStringRef>(0), string(str) {}
    inline QCFString(const CFStringRef cfstr = 0) : QCFType<CFStringRef>(cfstr) {}
    inline QCFString(const QCFType<CFStringRef> &other) : QCFType<CFStringRef>(other) {}
    operator QString() const;
    operator CFStringRef() const;

private:
    QString string;
};

#ifdef Q_OS_OSX
Q_CORE_EXPORT QChar qt_mac_qtKey2CocoaKey(Qt::Key key);
Q_CORE_EXPORT Qt::Key qt_mac_cocoaKey2QtKey(QChar keyCode);
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QMacAutoReleasePool *pool);
#endif

Q_CORE_EXPORT void qt_apple_check_os_version();
Q_CORE_EXPORT bool qt_apple_isApplicationExtension();

#if defined(Q_OS_MACOS) && !defined(QT_BOOTSTRAPPED)
Q_CORE_EXPORT bool qt_apple_isSandboxed();
# ifdef __OBJC__
QT_END_NAMESPACE
@interface NSObject (QtSandboxHelpers)
- (id)qt_valueForPrivateKey:(NSString *)key;
@end
QT_BEGIN_NAMESPACE
# endif
#endif

#if !defined(QT_BOOTSTRAPPED) && !defined(Q_OS_WATCHOS)
QT_END_NAMESPACE
# if defined(Q_OS_MACOS)
Q_FORWARD_DECLARE_OBJC_CLASS(NSApplication);
using AppleApplication = NSApplication;
# else
Q_FORWARD_DECLARE_OBJC_CLASS(UIApplication);
using AppleApplication = UIApplication;
# endif
QT_BEGIN_NAMESPACE
Q_CORE_EXPORT AppleApplication *qt_apple_sharedApplication();
#endif

// --------------------------------------------------------------------------

#if !defined(QT_BOOTSTRAPPED) && (QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_12) || !defined(Q_OS_MACOS))
#define QT_USE_APPLE_UNIFIED_LOGGING

QT_END_NAMESPACE
#include <os/log.h>

// The compiler isn't smart enough to realize that we're calling these functions
// guarded by __builtin_available, so we need to also tag each function with the
// runtime requirements.
#include <os/availability.h>
#define OS_LOG_AVAILABILITY API_AVAILABLE(macos(10.12), ios(10.0), tvos(10.0), watchos(3.0))
QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT AppleUnifiedLogger
{
public:
    static bool messageHandler(QtMsgType msgType, const QMessageLogContext &context, const QString &message,
        const QString &subsystem = QString()) OS_LOG_AVAILABILITY;
private:
    static os_log_type_t logTypeForMessageType(QtMsgType msgType) OS_LOG_AVAILABILITY;
    static os_log_t cachedLog(const QString &subsystem, const QString &category) OS_LOG_AVAILABILITY;
};

#undef OS_LOG_AVAILABILITY

#endif

// --------------------------------------------------------------------------

QT_END_NAMESPACE

#endif // QCORE_MAC_P_H
