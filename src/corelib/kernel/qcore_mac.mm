/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Petroules Corporation.
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

#include <private/qcore_mac_p.h>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#include <UIKit/UIKit.h>
#endif

#include <new>
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <objc/runtime.h>
#include <mach-o/dyld.h>
#include <sys/sysctl.h>

#include <qdebug.h>

#include "qendian.h"
#include "qhash.h"
#include "qpair.h"
#include "qmutex.h"
#include "qvarlengtharray.h"
#include "private/qlocking_p.h"

QT_BEGIN_NAMESPACE

// --------------------------------------------------------------------------

static void initializeStandardUserDefaults()
{
    // The standard user defaults are initialized from an ordered list of domains,
    // as documented by NSUserDefaults.standardUserDefaults. This includes e.g.
    // parsing command line arguments, such as -AppleFooBar "baz", as well as
    // global defaults. To ensure that these defaults are available through
    // the lower level Core Foundation preferences APIs, we need to initialize
    // them as early as possible via the Foundation-API, as the lower level APIs
    // do not do this initialization.
    Q_UNUSED(NSUserDefaults.standardUserDefaults);
}
Q_CONSTRUCTOR_FUNCTION(initializeStandardUserDefaults);

// --------------------------------------------------------------------------

QCFString::operator QString() const
{
    if (string.isEmpty() && value)
        const_cast<QCFString*>(this)->string = QString::fromCFString(value);
    return string;
}

QCFString::operator CFStringRef() const
{
    if (!value)
        const_cast<QCFString*>(this)->value = string.toCFString();
    return value;
}

// --------------------------------------------------------------------------

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)

bool AppleUnifiedLogger::willMirrorToStderr()
{
    // When running under Xcode or LLDB, one or more of these variables will
    // be set, which triggers libsystem_trace.dyld to log messages to stderr
    // as well, via_os_log_impl_mirror_to_stderr. Un-setting these variables
    // is not an option, as that would silence normal NSLog or os_log calls,
    // so instead we skip our own stderr output. See rdar://36919139.
    static bool willMirror = qEnvironmentVariableIsSet("OS_ACTIVITY_DT_MODE")
                                 || qEnvironmentVariableIsSet("ACTIVITY_LOG_STDERR")
                                 || qEnvironmentVariableIsSet("CFLOG_FORCE_STDERR");
    return willMirror;
}

QT_MAC_WEAK_IMPORT(_os_log_default);
bool AppleUnifiedLogger::messageHandler(QtMsgType msgType, const QMessageLogContext &context,
                                        const QString &message, const QString &optionalSubsystem)
{
    QString subsystem = optionalSubsystem;
    if (subsystem.isNull()) {
        static QString bundleIdentifier = []() {
            if (CFBundleRef bundle = CFBundleGetMainBundle()) {
                if (CFStringRef identifier = CFBundleGetIdentifier(bundle))
                    return QString::fromCFString(identifier);
            }
            return QString();
        }();
        subsystem = bundleIdentifier;
    }

    const bool isDefault = !context.category || !strcmp(context.category, "default");
    os_log_t log = isDefault ? OS_LOG_DEFAULT :
        cachedLog(subsystem, QString::fromLatin1(context.category));
    os_log_type_t logType = logTypeForMessageType(msgType);

    if (!os_log_type_enabled(log, logType))
        return false;

    // Logging best practices says we should not include symbolication
    // information or source file line numbers in messages, as the system
    // will automatically captures this information. In our case, what
    // the system captures is the call to os_log_with_type below, which
    // isn't really useful, but we still don't want to include the context's
    // info, as that would clutter the logging output. See rdar://35958308.

    // The format must be a string constant, so we can't pass on the
    // message. This means we won't be able to take advantage of the
    // unified logging's custom format specifiers such as %{BOOL}d.
    // We use the 'public' format specifier to prevent the logging
    // system from redacting our log message.
    os_log_with_type(log, logType, "%{public}s", qPrintable(message));

    return willMirrorToStderr();
}

os_log_type_t AppleUnifiedLogger::logTypeForMessageType(QtMsgType msgType)
{
    switch (msgType) {
    case QtDebugMsg: return OS_LOG_TYPE_DEBUG;
    case QtInfoMsg: return OS_LOG_TYPE_INFO;
    case QtWarningMsg: return OS_LOG_TYPE_DEFAULT;
    case QtCriticalMsg: return OS_LOG_TYPE_ERROR;
    case QtFatalMsg: return OS_LOG_TYPE_FAULT;
    }

    return OS_LOG_TYPE_DEFAULT;
}

os_log_t AppleUnifiedLogger::cachedLog(const QString &subsystem, const QString &category)
{
    static QBasicMutex mutex;
    const auto locker = qt_scoped_lock(mutex);

    static QHash<QPair<QString, QString>, os_log_t> logs;
    const auto cacheKey = qMakePair(subsystem, category);
    os_log_t log = logs.value(cacheKey);

    if (!log) {
        log = os_log_create(subsystem.toLatin1().constData(),
            category.toLatin1().constData());
        logs.insert(cacheKey, log);

        // Technically we should release the os_log_t resource when done
        // with it, but since we don't know when a category is disabled
        // we keep all cached os_log_t instances until shutdown, where
        // the OS will clean them up for us.
    }

    return log;
}

#endif // QT_USE_APPLE_UNIFIED_LOGGING

// -------------------------------------------------------------------------

QDebug operator<<(QDebug dbg, const NSObject *nsObject)
{
    return dbg << (nsObject ?
            dbg.verbosity() > 2 ?
                nsObject.debugDescription.UTF8String :
                nsObject.description.UTF8String
        : "NSObject(0x0)");
}

QDebug operator<<(QDebug dbg, CFStringRef stringRef)
{
    if (!stringRef)
        return dbg << "CFStringRef(0x0)";

    if (const UniChar *chars = CFStringGetCharactersPtr(stringRef))
        dbg << QStringView(reinterpret_cast<const QChar *>(chars), CFStringGetLength(stringRef));
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

QT_END_NAMESPACE
QT_USE_NAMESPACE
@interface QT_MANGLE_NAMESPACE(QMacAutoReleasePoolTracker) : NSObject
@end

@implementation QT_MANGLE_NAMESPACE(QMacAutoReleasePoolTracker) {
    NSAutoreleasePool **m_pool;
}

- (instancetype)initWithPool:(NSAutoreleasePool **)pool
{
    if ((self = [self init]))
        m_pool = pool;
    return self;
}

- (void)dealloc
{
    if (*m_pool) {
        // The pool is still valid, which means we're not being drained from
        // the corresponding QMacAutoReleasePool (see below).

        // QMacAutoReleasePool has only a single member, the NSAutoreleasePool*
        // so the address of that member is also the QMacAutoReleasePool itself.
        QMacAutoReleasePool *pool = reinterpret_cast<QMacAutoReleasePool *>(m_pool);
        qWarning() << "Premature drain of" << pool << "This can happen if you've allocated"
            << "the pool on the heap, or as a member of a heap-allocated object. This is not a"
            << "supported use of QMacAutoReleasePool, and might result in crashes when objects"
            << "in the pool are deallocated and then used later on under the assumption they"
            << "will be valid until" << pool << "has been drained.";

        // Reset the pool so that it's not drained again later on
        *m_pool = nullptr;
    }

    [super dealloc];
}
@end
QT_NAMESPACE_ALIAS_OBJC_CLASS(QMacAutoReleasePoolTracker);

QT_BEGIN_NAMESPACE

QMacAutoReleasePool::QMacAutoReleasePool()
    : pool([[NSAutoreleasePool alloc] init])
{
    Class trackerClass = [QMacAutoReleasePoolTracker class];

#ifdef QT_DEBUG
    void *poolFrame = nullptr;
    if (__builtin_available(macOS 10.14, iOS 12.0, tvOS 12.0, watchOS 5.0, *)) {
        void *frame;
        if (backtrace_from_fp(__builtin_frame_address(0), &frame, 1))
            poolFrame = frame;
    } else {
        static const int maxFrames = 3;
        void *callstack[maxFrames];
        if (backtrace(callstack, maxFrames) == maxFrames)
            poolFrame = callstack[maxFrames - 1];
    }

    if (poolFrame) {
        Dl_info info;
        if (dladdr(poolFrame, &info) && info.dli_sname) {
            const char *symbolName = info.dli_sname;
            if (symbolName[0] == '_') {
                int status;
                if (char *demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status))
                    symbolName = demangled;
            }

            char *className = nullptr;
            asprintf(&className, "  ^-- allocated in function: %s", symbolName);

            if (Class existingClass = objc_getClass(className))
                trackerClass = existingClass;
            else
                trackerClass = objc_duplicateClass(trackerClass, className, 0);

            free(className);

            if (symbolName != info.dli_sname)
                free((char*)symbolName);
        }
    }
#endif

    [[[trackerClass alloc] initWithPool:
        reinterpret_cast<NSAutoreleasePool **>(&pool)] autorelease];
}

QMacAutoReleasePool::~QMacAutoReleasePool()
{
    if (!pool) {
        qWarning() << "Prematurely drained pool" << this << "finally drained. Any objects belonging"
            << "to this pool have already been released, and have potentially been invalid since the"
            << "premature drain earlier on.";
        return;
    }

    // Save and reset pool before draining, so that the pool tracker can know
    // that it's being drained by its owning pool.
    NSAutoreleasePool *savedPool = static_cast<NSAutoreleasePool*>(pool);
    pool = nullptr;

    // Drain behaves the same as release, with the advantage that
    // if we're ever used in a garbage-collected environment, the
    // drain acts as a hint to the garbage collector to collect.
    [savedPool drain];
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QMacAutoReleasePool *pool)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QMacAutoReleasePool(" << (const void *)pool << ')';
    return debug;
}

QDebug operator<<(QDebug debug, const QCFString &string)
{
    debug << static_cast<QString>(string);
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

#ifdef Q_OS_MACOS
bool qt_mac_applicationIsInDarkMode()
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_14)
    if (__builtin_available(macOS 10.14, *)) {
        auto appearance = [NSApp.effectiveAppearance bestMatchFromAppearancesWithNames:
                @[ NSAppearanceNameAqua, NSAppearanceNameDarkAqua ]];
        return [appearance isEqualToString:NSAppearanceNameDarkAqua];
    }
#endif
    return false;
}

bool qt_mac_runningUnderRosetta()
{
    int translated = 0;
    auto size = sizeof(translated);
    if (sysctlbyname("sysctl.proc_translated", &translated, &size, nullptr, 0) == 0)
        return translated;
    return false;
}

std::optional<uint32_t> qt_mac_sipConfiguration()
{
    static auto configuration = []() -> std::optional<uint32_t> {
        QIOType<io_registry_entry_t> nvram = IORegistryEntryFromPath(kIOMasterPortDefault, "IODeviceTree:/options");
        if (!nvram) {
            qWarning("Failed to locate NVRAM entry in IO registry");
            return {};
        }

        QCFType<CFTypeRef> csrConfig = IORegistryEntryCreateCFProperty(nvram,
            CFSTR("csr-active-config"), kCFAllocatorDefault, IOOptionBits{});
        if (!csrConfig) {
            qWarning("Failed to locate SIP config in NVRAM");
            return {};
        }

        if (auto type = CFGetTypeID(csrConfig); type != CFDataGetTypeID()) {
            qWarning() << "Unexpected SIP config type" << CFCopyTypeIDDescription(type);
            return {};
        }

        QByteArray data = QByteArray::fromRawCFData(csrConfig.as<CFDataRef>());
        if (data.size() != sizeof(uint32_t)) {
            qWarning() << "Unexpected SIP config size" << data.size();
            return {};
        }

        return qFromLittleEndian<uint32_t>(data.constData());
    }();
    return configuration;
}
#endif

bool qt_apple_isApplicationExtension()
{
    static bool isExtension = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSExtension"];
    return isExtension;
}

#if !defined(QT_BOOTSTRAPPED) && !defined(Q_OS_WATCHOS)
AppleApplication *qt_apple_sharedApplication()
{
    // Application extensions are not allowed to access the shared application
    if (qt_apple_isApplicationExtension()) {
        qWarning() << "accessing the shared" << [AppleApplication class]
            << "is not allowed in application extensions";

        // In practice the application is actually available, but the App
        // review process will likely catch uses of it, so we return nil just
        // in case, unless we don't care about being App Store compliant.
#if QT_CONFIG(appstore_compliant)
        return nil;
#endif
    }

    // We use performSelector so that building with -fapplication-extension will
    // not mistakenly think we're using the shared application in extensions.
    return [[AppleApplication class] performSelector:@selector(sharedApplication)];
}
#endif

bool qt_apple_isSandboxed()
{
#if defined(Q_OS_MACOS)
    static bool isSandboxed = []() {
        QCFType<SecStaticCodeRef> staticCode = nullptr;
        NSURL *executableUrl = NSBundle.mainBundle.executableURL;
        if (SecStaticCodeCreateWithPath((__bridge CFURLRef)executableUrl,
            kSecCSDefaultFlags, &staticCode) != errSecSuccess)
            return false;

        QCFType<SecRequirementRef> sandboxRequirement;
        if (SecRequirementCreateWithString(CFSTR("entitlement[\"com.apple.security.app-sandbox\"] exists"),
            kSecCSDefaultFlags, &sandboxRequirement) != errSecSuccess)
            return false;

        if (SecStaticCodeCheckValidityWithErrors(staticCode,
            kSecCSBasicValidateOnly, sandboxRequirement, nullptr) != errSecSuccess)
            return false;

        return true;
    }();
    return isSandboxed;
#else
    return true; // All other Apple platforms
#endif
}

#if !defined(QT_BOOTSTRAPPED)
QT_END_NAMESPACE
@implementation NSObject (QtSandboxHelpers)
- (id)qt_valueForPrivateKey:(NSString *)key
{
    if (qt_apple_isSandboxed())
        return nil;

    return [self valueForKey:key];
}
@end
QT_BEGIN_NAMESPACE
#endif

#ifdef Q_OS_MACOS
/*
    Ensure that Objective-C objects auto-released in main(), directly or indirectly,
    after QCoreApplication construction, are released when the app goes out of scope.
    The memory will be reclaimed by the system either way when the process exits,
    but by having a root level pool we ensure that the objects get their dealloc
    methods called, which is useful for debugging object ownership graphs, etc.
*/

QT_END_NAMESPACE
#define ROOT_LEVEL_POOL_MARKER QT_ROOT_LEVEL_POOL__THESE_OBJECTS_WILL_BE_RELEASED_WHEN_QAPP_GOES_OUT_OF_SCOPE
@interface QT_MANGLE_NAMESPACE(ROOT_LEVEL_POOL_MARKER) : NSObject @end
@implementation QT_MANGLE_NAMESPACE(ROOT_LEVEL_POOL_MARKER) @end
QT_NAMESPACE_ALIAS_OBJC_CLASS(ROOT_LEVEL_POOL_MARKER);
QT_BEGIN_NAMESPACE

const char ROOT_LEVEL_POOL_DISABLE_SWITCH[] = "QT_DISABLE_ROOT_LEVEL_AUTORELEASE_POOL";

QMacRootLevelAutoReleasePool::QMacRootLevelAutoReleasePool()
{
    if (qEnvironmentVariableIsSet(ROOT_LEVEL_POOL_DISABLE_SWITCH))
        return;

    pool.reset(new QMacAutoReleasePool);

    [[[ROOT_LEVEL_POOL_MARKER alloc] init] autorelease];

    if (qstrcmp(qgetenv("OBJC_DEBUG_MISSING_POOLS"), "YES") == 0) {
        qDebug("QCoreApplication root level NSAutoreleasePool in place. Break on ~%s and use\n" \
            "'p [NSAutoreleasePool showPools]' to show leaked objects, or set %s",
            __FUNCTION__, ROOT_LEVEL_POOL_DISABLE_SWITCH);
    }
}

QMacRootLevelAutoReleasePool::~QMacRootLevelAutoReleasePool()
{
}
#endif

// -------------------------------------------------------------------------

void qt_apple_check_os_version()
{
#if defined(__WATCH_OS_VERSION_MIN_REQUIRED)
    const char *os = "watchOS";
    const int version = __WATCH_OS_VERSION_MIN_REQUIRED;
#elif defined(__TV_OS_VERSION_MIN_REQUIRED)
    const char *os = "tvOS";
    const int version = __TV_OS_VERSION_MIN_REQUIRED;
#elif defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
    const char *os = "iOS";
    const int version = __IPHONE_OS_VERSION_MIN_REQUIRED;
#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
    const char *os = "macOS";
    const int version = __MAC_OS_X_VERSION_MIN_REQUIRED;
#endif
    const NSOperatingSystemVersion required = (NSOperatingSystemVersion){
        version / 10000, version / 100 % 100, version % 100};
    const NSOperatingSystemVersion current = NSProcessInfo.processInfo.operatingSystemVersion;
    if (![NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:required]) {
        NSDictionary *plist = NSBundle.mainBundle.infoDictionary;
        NSString *applicationName = plist[@"CFBundleDisplayName"];
        if (!applicationName)
            applicationName = plist[@"CFBundleName"];
        if (!applicationName)
            applicationName = NSProcessInfo.processInfo.processName;

        fprintf(stderr, "Sorry, \"%s\" cannot be run on this version of %s. "
            "Qt requires %s %ld.%ld.%ld or later, you have %s %ld.%ld.%ld.\n",
            applicationName.UTF8String, os,
            os, long(required.majorVersion), long(required.minorVersion), long(required.patchVersion),
            os, long(current.majorVersion), long(current.minorVersion), long(current.patchVersion));

        exit(1);
    }
}
Q_CONSTRUCTOR_FUNCTION(qt_apple_check_os_version);

// -------------------------------------------------------------------------

void QMacNotificationObserver::remove()
{
    if (observer)
        [[NSNotificationCenter defaultCenter] removeObserver:observer];
    observer = nullptr;
}

// -------------------------------------------------------------------------

QMacKeyValueObserver::QMacKeyValueObserver(const QMacKeyValueObserver &other)
    : QMacKeyValueObserver(other.object, other.keyPath, *other.callback.get())
{
}

void QMacKeyValueObserver::addObserver(NSKeyValueObservingOptions options)
{
    [object addObserver:observer forKeyPath:keyPath options:options context:callback.get()];
}

void QMacKeyValueObserver::removeObserver() {
    if (object)
        [object removeObserver:observer forKeyPath:keyPath context:callback.get()];
    object = nil;
}

KeyValueObserver *QMacKeyValueObserver::observer = [[KeyValueObserver alloc] init];

QT_END_NAMESPACE
@implementation QT_MANGLE_NAMESPACE(KeyValueObserver)
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object
        change:(NSDictionary<NSKeyValueChangeKey, id> *)change context:(void *)context
{
    Q_UNUSED(keyPath);
    Q_UNUSED(object);
    Q_UNUSED(change);

    (*reinterpret_cast<QMacKeyValueObserver::Callback*>(context))();
}
@end
QT_BEGIN_NAMESPACE

// -------------------------------------------------------------------------

QOperatingSystemVersion QMacVersion::buildSDK(VersionTarget target)
{
    switch (target) {
    case ApplicationBinary: return applicationVersion().second;
    case QtLibraries: return libraryVersion().second;
    }
    Q_UNREACHABLE();
}

QOperatingSystemVersion QMacVersion::deploymentTarget(VersionTarget target)
{
    switch (target) {
    case ApplicationBinary: return applicationVersion().first;
    case QtLibraries: return libraryVersion().first;
    }
    Q_UNREACHABLE();
}

QOperatingSystemVersion QMacVersion::currentRuntime()
{
    return QOperatingSystemVersion::current();
}

// Mach-O platforms
enum Platform {
   macOS = 1,
   iOS = 2,
   tvOS = 3,
   watchOS = 4,
   bridgeOS = 5,
   macCatalyst = 6,
   iOSSimulator = 7,
   tvOSSimulator = 8,
   watchOSSimulator = 9
};

QMacVersion::VersionTuple QMacVersion::versionsForImage(const mach_header *machHeader)
{
    static auto osForLoadCommand = [](uint32_t cmd) {
        switch (cmd) {
        case LC_VERSION_MIN_MACOSX: return QOperatingSystemVersion::MacOS;
        case LC_VERSION_MIN_IPHONEOS: return QOperatingSystemVersion::IOS;
        case LC_VERSION_MIN_TVOS: return QOperatingSystemVersion::TvOS;
        case LC_VERSION_MIN_WATCHOS: return QOperatingSystemVersion::WatchOS;
        default: return QOperatingSystemVersion::Unknown;
        }
    };

    static auto osForPlatform = [](uint32_t platform) {
        switch (platform) {
        case Platform::macOS:
            return QOperatingSystemVersion::MacOS;
        case Platform::iOS:
        case Platform::iOSSimulator:
            return QOperatingSystemVersion::IOS;
        case Platform::tvOS:
        case Platform::tvOSSimulator:
            return QOperatingSystemVersion::TvOS;
        case Platform::watchOS:
        case Platform::watchOSSimulator:
            return QOperatingSystemVersion::WatchOS;
        default:
            return QOperatingSystemVersion::Unknown;
        }
    };

    static auto makeVersionTuple = [](uint32_t dt, uint32_t sdk, QOperatingSystemVersion::OSType osType) {
        return qMakePair(
            QOperatingSystemVersion(osType, dt >> 16 & 0xffff, dt >> 8 & 0xff, dt & 0xff),
            QOperatingSystemVersion(osType, sdk >> 16 & 0xffff, sdk >> 8 & 0xff, sdk & 0xff)
        );
    };

    const bool is64Bit = machHeader->magic == MH_MAGIC_64 || machHeader->magic == MH_CIGAM_64;
    auto commandCursor = uintptr_t(machHeader) + (is64Bit ? sizeof(mach_header_64) : sizeof(mach_header));

    for (uint32_t i = 0; i < machHeader->ncmds; ++i) {
        load_command *loadCommand = reinterpret_cast<load_command *>(commandCursor);
        if (loadCommand->cmd == LC_VERSION_MIN_MACOSX || loadCommand->cmd == LC_VERSION_MIN_IPHONEOS
            || loadCommand->cmd == LC_VERSION_MIN_TVOS || loadCommand->cmd == LC_VERSION_MIN_WATCHOS) {
            auto versionCommand = reinterpret_cast<version_min_command *>(loadCommand);
            return makeVersionTuple(versionCommand->version, versionCommand->sdk, osForLoadCommand(loadCommand->cmd));
#if QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_13, __IPHONE_11_0, __TVOS_11_0, __WATCHOS_4_0)
        } else if (loadCommand->cmd == LC_BUILD_VERSION) {
            auto versionCommand = reinterpret_cast<build_version_command *>(loadCommand);
            return makeVersionTuple(versionCommand->minos, versionCommand->sdk, osForPlatform(versionCommand->platform));
#endif
        }
        commandCursor += loadCommand->cmdsize;
    }
    Q_ASSERT_X(false, "QMacVersion", "Could not find any version load command");
    Q_UNREACHABLE();
}

QMacVersion::VersionTuple QMacVersion::applicationVersion()
{
    static VersionTuple version = []() {
        const mach_header *executableHeader = nullptr;
        for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
            auto header = _dyld_get_image_header(i);
            if (header->filetype == MH_EXECUTE) {
                executableHeader = header;
                break;
            }
        }
        Q_ASSERT_X(executableHeader, "QMacVersion", "Failed to resolve Mach-O header of executable");
        return versionsForImage(executableHeader);
    }();
    return version;
}

QMacVersion::VersionTuple QMacVersion::libraryVersion()
{
    static VersionTuple version = []() {
        Dl_info qtCoreImage;
        dladdr((const void *)&QMacVersion::libraryVersion, &qtCoreImage);
        Q_ASSERT_X(qtCoreImage.dli_fbase, "QMacVersion", "Failed to resolve Mach-O header of QtCore");
        return versionsForImage(static_cast<mach_header*>(qtCoreImage.dli_fbase));
    }();
    return version;
}

// -------------------------------------------------------------------------

QT_END_NAMESPACE

