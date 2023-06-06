// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 Petroules Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <spawn.h>

#include <qdebug.h>

#include "qendian.h"
#include "qhash.h"
#include "qpair.h"
#include "qmutex.h"
#include "qvarlengtharray.h"
#include "private/qlocking_p.h"

#if !defined(QT_BOOTSTRAPPED)
#include <thread>
#endif

#if !defined(QT_APPLE_NO_PRIVATE_APIS)
extern "C" {
typedef uint32_t csr_config_t;
extern int csr_get_active_config(csr_config_t *) __attribute__((weak_import));

#ifdef QT_BUILD_INTERNAL
int responsibility_spawnattrs_setdisclaim(posix_spawnattr_t attrs, int disclaim)
__attribute__((availability(macos,introduced=10.14),weak_import));
pid_t responsibility_get_pid_responsible_for_pid(pid_t) __attribute__((weak_import));
char *** _NSGetArgv();
extern char **environ;
#endif
}
#endif

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

bool AppleUnifiedLogger::preventsStderrLogging()
{
    // os_log will mirror to stderr if OS_ACTIVITY_DT_MODE is set,
    // regardless of its value. OS_ACTIVITY_MODE then controls whether
    // to include info and/or debug messages in this mirroring.
    // For some reason, when launched under lldb (via Xcode or not),
    // all levels are included.

    // CFLog will normally log to both stderr, and via os_log.
    // Setting CFLOG_FORCE_DISABLE_STDERR disables the stderr
    // logging. Setting CFLOG_FORCE_STDERR will both duplicate
    // CFLog's output to stderr, and trigger OS_ACTIVITY_DT_MODE,
    // resulting in os_log calls also being mirrored to stderr.
    // Setting ACTIVITY_LOG_STDERR has the same effect.

    // NSLog is plumbed to CFLog, and will respond to the same
    // environment variables as CFLog.

    // We want to disable Qt's default stderr log handler when
    // os_log has already mirrored to stderr.
    static bool willMirror = qEnvironmentVariableIsSet("OS_ACTIVITY_DT_MODE")
                          || qEnvironmentVariableIsSet("ACTIVITY_LOG_STDERR")
                          || qEnvironmentVariableIsSet("CFLOG_FORCE_STDERR");

    // As well as when we suspect that Xcode is going to present os_log
    // as structured log messages.
    static bool disableStderr = qEnvironmentVariableIsSet("CFLOG_FORCE_DISABLE_STDERR");

    return willMirror || disableStderr;
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
        os_log_create(subsystem.toLatin1().constData(), context.category);
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

    return preventsStderrLogging();
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

#endif // QT_USE_APPLE_UNIFIED_LOGGING

// -------------------------------------------------------------------------

QDebug operator<<(QDebug dbg, id obj)
{
    if (!obj) {
        // Match NSLog
        dbg << "(null)";
        return dbg;
    }

    for (Class cls = object_getClass(obj); cls; cls = class_getSuperclass(cls)) {
        if (cls == NSObject.class) {
            dbg << static_cast<NSObject*>(obj);
            return dbg;
        }
    }

    // Match NSObject.debugDescription
    const QDebugStateSaver saver(dbg);
    dbg.nospace() << '<' << object_getClassName(obj) << ": " << static_cast<void*>(obj) << '>';
    return dbg;
}

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

#ifdef QT_DEBUG
@interface QT_MANGLE_NAMESPACE(QMacAutoReleasePoolTracker) : NSObject
@end

@implementation QT_MANGLE_NAMESPACE(QMacAutoReleasePoolTracker)
@end
QT_NAMESPACE_ALIAS_OBJC_CLASS(QMacAutoReleasePoolTracker);
#endif // QT_DEBUG

// Use the direct runtime interface to manage autorelease pools, as it
// has less overhead then allocating NSAutoreleasePools, and allows for
// a future where we use ARC (where NSAutoreleasePool is not allowed).
// https://clang.llvm.org/docs/AutomaticReferenceCounting.html#runtime-support

extern "C" {
void *objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void *pool);
}

QT_BEGIN_NAMESPACE

QMacAutoReleasePool::QMacAutoReleasePool()
    : pool(objc_autoreleasePoolPush())
{
#ifdef QT_DEBUG
    static const bool debugAutoReleasePools = qEnvironmentVariableIsSet("QT_DARWIN_DEBUG_AUTORELEASEPOOLS");
    if (!debugAutoReleasePools)
        return;

    Class trackerClass = [QMacAutoReleasePoolTracker class];

    void *poolFrame = nullptr;
    void *frames[2];
    if (backtrace_from_fp(__builtin_frame_address(0), frames, 2))
        poolFrame = frames[1];

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

    [[trackerClass new] autorelease];
#endif // QT_DEBUG
}

QMacAutoReleasePool::~QMacAutoReleasePool()
{
    objc_autoreleasePoolPop(pool);
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
    auto appearance = [NSApp.effectiveAppearance bestMatchFromAppearancesWithNames:
            @[ NSAppearanceNameAqua, NSAppearanceNameDarkAqua ]];
    return [appearance isEqualToString:NSAppearanceNameDarkAqua];
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
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
        csr_config_t config;
        if (csr_get_active_config && csr_get_active_config(&config) == 0)
            return config;
#endif

        QIOType<io_registry_entry_t> nvram = IORegistryEntryFromPath(kIOMasterPortDefault, "IODeviceTree:/options");
        if (!nvram) {
            qWarning("Failed to locate NVRAM entry in IO registry");
            return {};
        }

        QCFType<CFTypeRef> csrConfig = IORegistryEntryCreateCFProperty(nvram,
            CFSTR("csr-active-config"), kCFAllocatorDefault, IOOptionBits{});
        if (!csrConfig)
            return {}; // SIP config is not available

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

#define CHECK_SPAWN(expr) \
    if (int err = (expr)) { \
        posix_spawnattr_destroy(&attr); \
        return; \
    }

#ifdef QT_BUILD_INTERNAL
void qt_mac_ensureResponsible()
{
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
    if (!responsibility_get_pid_responsible_for_pid || !responsibility_spawnattrs_setdisclaim)
        return;

    auto pid = getpid();
    if (responsibility_get_pid_responsible_for_pid(pid) == pid)
        return; // Already responsible

    posix_spawnattr_t attr = {};
    CHECK_SPAWN(posix_spawnattr_init(&attr));

    // Behave as exec
    short flags = POSIX_SPAWN_SETEXEC;

    // Reset signal mask
    sigset_t no_signals;
    sigemptyset(&no_signals);
    CHECK_SPAWN(posix_spawnattr_setsigmask(&attr, &no_signals));
    flags |= POSIX_SPAWN_SETSIGMASK;

    // Reset all signals to their default handlers
    sigset_t all_signals;
    sigfillset(&all_signals);
    CHECK_SPAWN(posix_spawnattr_setsigdefault(&attr, &all_signals));
    flags |= POSIX_SPAWN_SETSIGDEF;

    CHECK_SPAWN(posix_spawnattr_setflags(&attr, flags));

    CHECK_SPAWN(responsibility_spawnattrs_setdisclaim(&attr, 1));

    char **argv = *_NSGetArgv();
    posix_spawnp(&pid, argv[0], nullptr, &attr, argv, environ);
    posix_spawnattr_destroy(&attr);
#endif
}
#endif // QT_BUILD_INTERNAL

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

#if !defined(QT_BOOTSTRAPPED)

#if defined(Q_OS_MACOS)
namespace {
struct SandboxChecker
{
    SandboxChecker() : m_thread([this]{
            m_isSandboxed = []{
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
        })
    {}
    ~SandboxChecker() {
        std::scoped_lock lock(m_mutex);
        if (m_thread.joinable())
            m_thread.detach();
    }
    bool isSandboxed() const {
        std::scoped_lock lock(m_mutex);
        if (m_thread.joinable())
            m_thread.join();
        return m_isSandboxed;
    }
private:
    bool m_isSandboxed;
    mutable std::thread m_thread;
    mutable std::mutex m_mutex;
};
} // namespace
static SandboxChecker sandboxChecker;
#endif // Q_OS_MACOS

bool qt_apple_isSandboxed()
{
#if defined(Q_OS_MACOS)
    return sandboxChecker.isSandboxed();
#else
    return true; // All other Apple platforms
#endif
}

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
#endif // !QT_BOOTSTRAPPED

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

    const auto required = QVersionNumber(version / 10000, version / 100 % 100, version % 100);
    const auto current = QOperatingSystemVersion::current().version();

#if defined(Q_OS_MACOS)
    // Check for compatibility version, in which case we can't do a
    // comparison to the deployment target, which might be e.g. 11.0
    if (current.majorVersion() == 10 && current.minorVersion() >= 16)
        return;
#endif

    if (current < required) {
        NSDictionary *plist = NSBundle.mainBundle.infoDictionary;
        NSString *applicationName = plist[@"CFBundleDisplayName"];
        if (!applicationName)
            applicationName = plist[@"CFBundleName"];
        if (!applicationName)
            applicationName = NSProcessInfo.processInfo.processName;

        fprintf(stderr, "Sorry, \"%s\" cannot be run on this version of %s. "
            "Qt requires %s %ld.%ld.%ld or later, you have %s %ld.%ld.%ld.\n",
            applicationName.UTF8String, os,
            os, long(required.majorVersion()), long(required.minorVersion()), long(required.microVersion()),
            os, long(current.majorVersion()), long(current.minorVersion()), long(current.microVersion()));

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
        } else if (loadCommand->cmd == LC_BUILD_VERSION) {
            auto versionCommand = reinterpret_cast<build_version_command *>(loadCommand);
            return makeVersionTuple(versionCommand->minos, versionCommand->sdk, osForPlatform(versionCommand->platform));
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

