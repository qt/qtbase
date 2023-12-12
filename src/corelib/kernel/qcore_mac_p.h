// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "private/qglobal_p.h"

#include <QtCore/qoperatingsystemversion.h>

#ifdef Q_OS_MACOS
#include <mach/port.h>
struct mach_header;
typedef int kern_return_t;
typedef mach_port_t io_object_t;
extern "C" {
kern_return_t IOObjectRetain(io_object_t object);
kern_return_t IOObjectRelease(io_object_t object);
}
#endif

#ifndef __IMAGECAPTURE__
#  define __IMAGECAPTURE__
#endif

// --------------------------------------------------------------------------

#if defined(QT_BOOTSTRAPPED)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#include <functional>
#endif

#include "qstring.h"
#include "qscopedpointer.h"
#include "qpair.h"

#if defined( __OBJC__) && defined(QT_NAMESPACE)
#define QT_NAMESPACE_ALIAS_OBJC_CLASS(__KLASS__) @compatibility_alias __KLASS__ QT_MANGLE_NAMESPACE(__KLASS__)
#else
#define QT_NAMESPACE_ALIAS_OBJC_CLASS(__KLASS__)
#endif

#define QT_MAC_WEAK_IMPORT(symbol) extern "C" decltype(symbol) symbol __attribute__((weak_import));

#if defined(__OBJC__)
#define QT_DECLARE_NAMESPACED_OBJC_INTERFACE(classname, definition) \
    @interface QT_MANGLE_NAMESPACE(classname) : \
    definition \
    @end \
    QT_NAMESPACE_ALIAS_OBJC_CLASS(classname);
#else
#define QT_DECLARE_NAMESPACED_OBJC_INTERFACE(classname, definition) \
    Q_FORWARD_DECLARE_OBJC_CLASS(QT_MANGLE_NAMESPACE(classname)); \
    using classname = QT_MANGLE_NAMESPACE(classname);
#endif

#define QT_FORWARD_DECLARE_OBJC_ENUM(name, type) \
    typedef type name;

Q_FORWARD_DECLARE_OBJC_CLASS(NSObject);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);

// @compatibility_alias doesn't work with categories or their methods
#define QtExtras QT_MANGLE_NAMESPACE(QtExtras)

QT_BEGIN_NAMESPACE
template <typename T, typename U, auto RetainFunction, auto ReleaseFunction>
class QAppleRefCounted
{
public:
    Q_NODISCARD_CTOR QAppleRefCounted() : value() {}
    Q_NODISCARD_CTOR QAppleRefCounted(const T &t) : value(t) {}
    Q_NODISCARD_CTOR QAppleRefCounted(T &&t)
            noexcept(std::is_nothrow_move_constructible<T>::value)
        : value(std::move(t)) {}
    Q_NODISCARD_CTOR QAppleRefCounted(QAppleRefCounted &&other)
            noexcept(std::is_nothrow_move_assignable<T>::value &&
                     std::is_nothrow_move_constructible<T>::value)
        : value(std::exchange(other.value, T())) {}
    Q_NODISCARD_CTOR QAppleRefCounted(const QAppleRefCounted &other)
        : value(other.value)
    { if (value) RetainFunction(value); }
    ~QAppleRefCounted() { if (value) ReleaseFunction(value); }
    operator T() const { return value; }
    void swap(QAppleRefCounted &other) noexcept(noexcept(qSwap(value, other.value)))
    { qSwap(value, other.value); }
    QAppleRefCounted &operator=(const QAppleRefCounted &other)
    { QAppleRefCounted copy(other); swap(copy); return *this; }
    QAppleRefCounted &operator=(QAppleRefCounted &&other)
        noexcept(std::is_nothrow_move_assignable<T>::value &&
                 std::is_nothrow_move_constructible<T>::value)
    { QAppleRefCounted moved(std::move(other)); swap(moved); return *this; }
    T *operator&() { return &value; }
protected:
    T value;
};

class QMacAutoReleasePool
{
public:
    Q_NODISCARD_CTOR Q_CORE_EXPORT QMacAutoReleasePool();
    Q_CORE_EXPORT ~QMacAutoReleasePool();
private:
    Q_DISABLE_COPY(QMacAutoReleasePool)
    void *pool;
};

#ifdef Q_OS_MACOS
class QMacRootLevelAutoReleasePool
{
public:
    Q_NODISCARD_CTOR QMacRootLevelAutoReleasePool();
    ~QMacRootLevelAutoReleasePool();
private:
    QScopedPointer<QMacAutoReleasePool> pool;
};
#endif

/*
    Helper class that automates reference counting for CFtypes.
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
    using Base = QAppleRefCounted<T, CFTypeRef, CFRetain, CFRelease>;
public:
    using Base::Base;
    Q_NODISCARD_CTOR explicit QCFType(CFTypeRef r) : Base(static_cast<T>(r)) {}
    template <typename X> X as() const { return reinterpret_cast<X>(this->value); }
    static QCFType constructFromGet(const T &t)
    {
        if (t)
            CFRetain(t);
        return QCFType<T>(t);
    }
};

#ifdef Q_OS_MACOS
template <typename T>
class QIOType : public QAppleRefCounted<T, io_object_t, IOObjectRetain, IOObjectRelease>
{
    using QAppleRefCounted<T, io_object_t, IOObjectRetain, IOObjectRelease>::QAppleRefCounted;
};
#endif

class QCFString : public QCFType<CFStringRef>
{
public:
    using QCFType<CFStringRef>::QCFType;
    Q_NODISCARD_CTOR QCFString(const QString &str) : QCFType<CFStringRef>(0), string(str) {}
    Q_NODISCARD_CTOR QCFString(const CFStringRef cfstr = 0) : QCFType<CFStringRef>(cfstr) {}
    Q_NODISCARD_CTOR QCFString(const QCFType<CFStringRef> &other) : QCFType<CFStringRef>(other) {}
    Q_CORE_EXPORT operator QString() const;
    Q_CORE_EXPORT operator CFStringRef() const;

private:
    QString string;
};

#ifdef Q_OS_MACOS
Q_CORE_EXPORT bool qt_mac_applicationIsInDarkMode();
Q_CORE_EXPORT bool qt_mac_runningUnderRosetta();
Q_CORE_EXPORT std::optional<uint32_t> qt_mac_sipConfiguration();
#ifdef QT_BUILD_INTERNAL
Q_AUTOTEST_EXPORT void qt_mac_ensureResponsible();
#endif
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QMacAutoReleasePool *pool);
Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QCFString &string);
#endif

Q_CORE_EXPORT bool qt_apple_isApplicationExtension();

#if !defined(QT_BOOTSTRAPPED)
Q_CORE_EXPORT bool qt_apple_isSandboxed();

#if defined(__OBJC__)
QT_END_NAMESPACE
@interface NSObject (QtExtras)
- (id)qt_valueForPrivateKey:(NSString *)key;
@end
QT_BEGIN_NAMESPACE
#endif
#endif // !QT_BOOTSTRAPPED

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

#if !defined(QT_BOOTSTRAPPED)
#define QT_USE_APPLE_UNIFIED_LOGGING

QT_END_NAMESPACE
#include <os/log.h>
QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT AppleUnifiedLogger
{
public:
    static bool messageHandler(QtMsgType msgType, const QMessageLogContext &context,
                               const QString &message)
    { return messageHandler(msgType, context, message, QString()); }
    static bool messageHandler(QtMsgType msgType, const QMessageLogContext &context,
                               const QString &message, const QString &subsystem);
    static bool preventsStderrLogging();
private:
    static os_log_type_t logTypeForMessageType(QtMsgType msgType);
    static os_log_t cachedLog(const QString &subsystem, const QString &category);
};

#endif

// --------------------------------------------------------------------------

#if !defined(QT_BOOTSTRAPPED)

QT_END_NAMESPACE
#include <os/activity.h>
QT_BEGIN_NAMESPACE

template <typename T> using QAppleOsType = QAppleRefCounted<T, void *, os_retain, os_release>;

class Q_CORE_EXPORT QAppleLogActivity
{
public:
    QAppleLogActivity() : activity(nullptr) {}
    QAppleLogActivity(os_activity_t activity) : activity(activity) {}
    ~QAppleLogActivity() { if (activity) leave(); }

    Q_DISABLE_COPY(QAppleLogActivity)

    QAppleLogActivity(QAppleLogActivity &&other)
        : activity(std::exchange(other.activity, nullptr)), state(other.state)
    {
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QAppleLogActivity)

    QAppleLogActivity &&enter()
    {
        if (activity)
            os_activity_scope_enter(static_cast<os_activity_t>(*this), &state);
        return std::move(*this);
    }

    void leave()
    {
        if (activity)
            os_activity_scope_leave(&state);
    }

    operator os_activity_t()
    {
        return reinterpret_cast<os_activity_t>(static_cast<void *>(activity));
    }

    void swap(QAppleLogActivity &other)
    {
        activity.swap(other.activity);
        std::swap(state, other.state);
    }

private:
    // Work around API_AVAILABLE not working for templates by using void*
    QAppleOsType<void *> activity;
    os_activity_scope_state_s state;
};

#define QT_APPLE_LOG_ACTIVITY_CREATE(condition, description, parent) []() { \
        if (!(condition)) \
            return QAppleLogActivity(); \
        return QAppleLogActivity(os_activity_create(description, parent, OS_ACTIVITY_FLAG_DEFAULT)); \
    }()

#define QT_APPLE_LOG_ACTIVITY_WITH_PARENT_3(condition, description, parent) QT_APPLE_LOG_ACTIVITY_CREATE(condition, description, parent)
#define QT_APPLE_LOG_ACTIVITY_WITH_PARENT_2(description, parent) QT_APPLE_LOG_ACTIVITY_WITH_PARENT_3(true, description, parent)
#define QT_APPLE_LOG_ACTIVITY_WITH_PARENT(...) QT_OVERLOADED_MACRO(QT_APPLE_LOG_ACTIVITY_WITH_PARENT, __VA_ARGS__)

QT_MAC_WEAK_IMPORT(_os_activity_current);
#define QT_APPLE_LOG_ACTIVITY_2(condition, description) QT_APPLE_LOG_ACTIVITY_CREATE(condition, description, OS_ACTIVITY_CURRENT)
#define QT_APPLE_LOG_ACTIVITY_1(description) QT_APPLE_LOG_ACTIVITY_2(true, description)
#define QT_APPLE_LOG_ACTIVITY(...) QT_OVERLOADED_MACRO(QT_APPLE_LOG_ACTIVITY, __VA_ARGS__)

#define QT_APPLE_SCOPED_LOG_ACTIVITY(...) QAppleLogActivity scopedLogActivity = QT_APPLE_LOG_ACTIVITY(__VA_ARGS__).enter();

#endif // !defined(QT_BOOTSTRAPPED)

// -------------------------------------------------------------------------

class Q_CORE_EXPORT QMacNotificationObserver
{
public:
    QMacNotificationObserver() {}

#if defined( __OBJC__)
    template<typename Functor>
    QMacNotificationObserver(NSObject *object, NSNotificationName name, Functor callback) {
        observer = [[NSNotificationCenter defaultCenter] addObserverForName:name
            object:object queue:nil usingBlock:^(NSNotification *) {
                callback();
            }
        ];
    }
#endif

    QMacNotificationObserver(const QMacNotificationObserver &other) = delete;
    QMacNotificationObserver(QMacNotificationObserver &&other)
        : observer(std::exchange(other.observer, nullptr))
    {
    }

    QMacNotificationObserver &operator=(const QMacNotificationObserver &other) = delete;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QMacNotificationObserver)

    void swap(QMacNotificationObserver &other) noexcept
    {
        qt_ptr_swap(observer, other.observer);
    }

    void remove();
    ~QMacNotificationObserver() { remove(); }

private:
    NSObject *observer = nullptr;
};

QT_END_NAMESPACE
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(KeyValueObserver, NSObject)
QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QMacKeyValueObserver
{
public:
    using Callback = std::function<void()>;

    QMacKeyValueObserver() = default;

#if defined( __OBJC__)
    // Note: QMacKeyValueObserver must not outlive the object observed!
    QMacKeyValueObserver(NSObject *object, NSString *keyPath, Callback callback,
        NSKeyValueObservingOptions options = NSKeyValueObservingOptionNew)
        : object(object), keyPath(keyPath), callback(new Callback(callback))
    {
        addObserver(options);
    }
#endif

    QMacKeyValueObserver(const QMacKeyValueObserver &other);

    QMacKeyValueObserver(QMacKeyValueObserver &&other) noexcept { swap(other); }

    ~QMacKeyValueObserver() { removeObserver(); }

    QMacKeyValueObserver &operator=(const QMacKeyValueObserver &other)
    {
        QMacKeyValueObserver tmp(other);
        swap(tmp);
        return *this;
    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QMacKeyValueObserver)

    void removeObserver();

    void swap(QMacKeyValueObserver &other) noexcept
    {
        qt_ptr_swap(object, other.object);
        qt_ptr_swap(keyPath, other.keyPath);
        callback.swap(other.callback);
    }

private:
#if defined( __OBJC__)
    void addObserver(NSKeyValueObservingOptions options);
#endif

    NSObject *object = nullptr;
    NSString *keyPath = nullptr;
    std::unique_ptr<Callback> callback;

    static KeyValueObserver *observer;
};

// -------------------------------------------------------------------------

class Q_CORE_EXPORT QMacVersion
{
public:
    enum VersionTarget {
        ApplicationBinary,
        QtLibraries
    };

    static QOperatingSystemVersion buildSDK(VersionTarget target = ApplicationBinary);
    static QOperatingSystemVersion deploymentTarget(VersionTarget target = ApplicationBinary);
    static QOperatingSystemVersion currentRuntime();

private:
    QMacVersion() = default;
    using VersionTuple = std::pair<QOperatingSystemVersion, QOperatingSystemVersion>;
    static VersionTuple versionsForImage(const mach_header *machHeader);
    static VersionTuple applicationVersion();
    static VersionTuple libraryVersion();
};

// -------------------------------------------------------------------------

#ifdef __OBJC__
template <typename T>
typename std::enable_if<std::is_pointer<T>::value, T>::type
qt_objc_cast(id object)
{
    if ([object isKindOfClass:[typename std::remove_pointer<T>::type class]])
        return static_cast<T>(object);

    return nil;
}
#endif

// -------------------------------------------------------------------------

QT_END_NAMESPACE

#endif // QCORE_MAC_P_H
