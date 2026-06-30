// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

#include "qdir.h"
#include "qstringlist.h"
#include "qfile.h"
#if QT_CONFIG(settings)
#include "qresource.h"
#include "qsettings.h"
#endif
#include "qlibraryinfo.h"
#include "qlibraryinfo_p.h"

#include "qcoreapplication.h"

#include "private/qcoreapplication_p.h"
#include "private/qfilesystementry_p.h"
#include "archdetect.cpp"
#include "qconfig.cpp"

#if defined(Q_OS_APPLE)
#  include "private/qcore_mac_p.h"
#endif

#if QT_CONFIG(relocatable) && QT_CONFIG(dlopen)
#    include <dlfcn.h>
#endif

#if QT_CONFIG(relocatable) && defined(Q_OS_WIN)
#    include <qt_windows.h>
#endif

#include <memory>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern void qDumpCPUFeatures(); // in qsimd.cpp

static QString fromRawStringView(QStringView string)
{
    return QString::fromRawData(string.data(), string.size());
}

static bool pathIsRelative(const QString &path)
{
    using FromInternalPath = QFileSystemEntry::FromInternalPath;
    return !path.startsWith(':'_L1) && QFileSystemEntry(path, FromInternalPath{}).isRelative();
}

// Resolves and caches the Qt and application prefix roots. Resolution
// may be costly and is queried repeatedly during startup, so results
// are memoized. Wrapped in a Q_GLOBAL_STATIC (prefixCache) so lookups
// during static destruction fall back to resolving from scratch.
struct QLibraryPrefixes
{
    static QString qtPrefix();
    static QString appPrefix();

private:
    // Pure resolvers, without caching, so they're usable even without a
    // cache instance during static destruction. resolveAppPrefix() returns an
    // empty string when no stable source is available yet; appPrefix()
    // then falls back to the (uncached) working directory.
    static QString resolveQtPrefix();
    static QString resolveAppPrefix();

    // The directory the application prefix is rooted at: the app bundle on
    // Apple platforms, or the application directory once a QCoreApplication
    // exists. Empty when no stable source is available yet.
    static QString resolveAppDir();

    // The qt.conf "Prefix" entry the application prefix is rooted at,
    // or its "." default when there's no qt.conf (or settings) to read.
    static QString qtConfPrefix();

    // Memoizes resolve() in the given member. A resolver may return an
    // empty string to signal that the result is not yet stable and should
    // not be cached, in which case it is resolved anew on each call. During
    // static destruction prefixCache() returns nullptr and resolve() is
    // used directly.
    static QString cached(QString QLibraryPrefixes::*member, QString (*resolve)());

    QString m_qt;
    QString m_app;
};
Q_GLOBAL_STATIC(QLibraryPrefixes, prefixCache)

QString QLibraryPrefixes::cached(QString QLibraryPrefixes::*member, QString (*resolve)())
{
    QLibraryPrefixes *cache = prefixCache();
    if (cache && !(cache->*member).isEmpty())
        return cache->*member;
    QString prefix = resolve();
    if (cache && !prefix.isEmpty())
        cache->*member = prefix;
    return prefix;
}

#if QT_CONFIG(settings)

static std::unique_ptr<QSettings> findConfiguration();

struct QLibrarySettings
{
    QLibrarySettings();
    void load();
    bool havePaths();
    QSettings *configuration();
    QVariant value(QLibraryInfo::LibraryPath location);
    QStringList paths(QLibraryInfo::LibraryPath location);

    std::unique_ptr<QSettings> settings;
    bool configHasPaths;
    bool reloadOnQAppAvailable;
};
Q_GLOBAL_STATIC(QLibrarySettings, qt_library_settings)

QLibrarySettings::QLibrarySettings() : configHasPaths(false), reloadOnQAppAvailable(false)
{
    load();
}

QSettings *QLibrarySettings::configuration()
{
    if (reloadOnQAppAvailable && QCoreApplication::instanceExists())
        load();
    return settings.get();
}

bool QLibrarySettings::havePaths()
{
    if (reloadOnQAppAvailable && QCoreApplication::instanceExists())
        load();
    return configHasPaths;
}

void QLibrarySettings::load()
{
    // If we get any settings here, those won't change when the application shows up.
    settings = findConfiguration();
    reloadOnQAppAvailable = !settings && !QCoreApplication::instanceExists();

    if (settings) {
        // This code needs to be in the regular library, as otherwise a qt.conf that
        // works for qmake would break things for dynamically built Qt tools.
        QStringList children = settings->childGroups();
        configHasPaths = !children.contains("Platforms"_L1)
                         || children.contains("Paths"_L1);
    } else {
        configHasPaths = false;
    }
}

/*!
    Returns the value for a given \a location from \c qt.conf

    If no \c qt.conf is found, or the configuration doesn't
    specify a value for the given \a path a null-variant is
    returned.

    \internal
*/
QVariant QLibrarySettings::value(QLibraryInfo::LibraryPath location)
{
    const auto locationInfo = QLibraryInfoPrivate::locationInfo(location);
    if (locationInfo.key.isNull())
        return {};

    QSettings *config = configuration();
    if (!settings || !configHasPaths)
        return {};

    config->beginGroup("Paths"_L1);
    auto cleanup = qScopeGuard([&]() { config->endGroup(); });
    QVariant value = config->value(locationInfo.key);
    if (!value.isValid() && !locationInfo.fallbackKey.isNull())
        value = config->value(locationInfo.fallbackKey);
    return value;
}

QStringList QLibrarySettings::paths(QLibraryInfo::LibraryPath location)
{
    QStringList ret;
    if (QVariant v = value(location); v.isValid()) {
        if (auto *asList = get_if<QStringList>(&v))
            ret = std::move(*asList);
        else
            ret << v.toString();

        for (qsizetype i = 0, end = ret.size(); i < end; ++i)
            ret[i] = QLibraryInfoPrivate::expandEnvVariables(ret[i]);
    }
    return ret;
}

namespace {
const QString *qtconfManualPath = nullptr;
}

void QLibraryInfoPrivate::setQtconfManualPath(const QString *path)
{
    qtconfManualPath = path;
}

static std::unique_ptr<QSettings> findConfiguration()
{
    if (qtconfManualPath)
        return std::make_unique<QSettings>(*qtconfManualPath, QSettings::IniFormat);

    QString qtconfig = QStringLiteral(":/qt/etc/qt.conf");
    if (QResource(qtconfig, QLocale::c()).isValid())
        return std::make_unique<QSettings>(qtconfig, QSettings::IniFormat);
#if defined(Q_OS_APPLE)
    CFBundleRef bundleRef = CFBundleGetMainBundle();
    if (bundleRef) {
        QCFType<CFURLRef> urlRef = CFBundleCopyResourceURL(bundleRef,
                                                           QCFString("qt.conf"_L1),
                                                           0,
                                                           0);
        if (urlRef) {
            QCFString path = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
            qtconfig = QDir::cleanPath(path);
            if (QFile::exists(qtconfig))
                return std::make_unique<QSettings>(qtconfig, QSettings::IniFormat);
        }
    }
#endif
    if (QCoreApplication::instanceExists()) {
        QString pwd = QCoreApplication::applicationDirPath();
        qtconfig = pwd + u"/qt" QT_STRINGIFY(QT_VERSION_MAJOR) ".conf"_s;
        if (QFile::exists(qtconfig))
            return std::make_unique<QSettings>(qtconfig, QSettings::IniFormat);
        qtconfig = pwd + u"/qt.conf";
        if (QFile::exists(qtconfig))
            return std::make_unique<QSettings>(qtconfig, QSettings::IniFormat);
    }
    return nullptr;     //no luck
}

QSettings *QLibraryInfoPrivate::configuration()
{
    QLibrarySettings *ls = qt_library_settings();
    return ls ? ls->configuration() : nullptr;
}
#endif // settings

void QLibraryInfoPrivate::reload()
{
#if QT_CONFIG(settings)
    if (qt_library_settings.exists())
        qt_library_settings->load();
#endif
    if (prefixCache.exists())
        *prefixCache = {};
}

/*!
    \class QLibraryInfo
    \inmodule QtCore
    \brief The QLibraryInfo class provides information about the Qt libraries.

    This class provides an abstraction for accessing information about the
    Qt libraries which the application is using, such as run-time paths,
    or the build configuration of the Qt libraries.

    The default run-time paths depend on the Qt build configuration, as well
    as whether the Qt libraries have been relocated, and possibly bundled along
    with the application.

    You can use a \c qt.conf file to override the default paths,
    in case your deployment situation does not match the defaults.
    For more information, see the \l {Using qt.conf} documentation.

    \sa QSysInfo, {Using qt.conf}
*/

/*!
    \internal

   You cannot create a QLibraryInfo, instead only the static functions are available to query
   information.
*/

QLibraryInfo::QLibraryInfo()
{ }

#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
#  define COMPILER_STRING __VERSION__       /* already includes the compiler's name */
#elif defined(Q_CC_GHS)
#  define COMPILER_STRING "GHS " QT_STRINGIFY(__GHS_VERSION_NUMBER)
#elif defined(Q_CC_GNU)
#  define COMPILER_STRING "GCC " __VERSION__
#elif defined(Q_CC_MSVC)
#  if _MSC_VER < 1910
#    define COMPILER_STRING "MSVC 2015"
#  elif _MSC_VER < 1917
#    define COMPILER_STRING "MSVC 2017"
#  elif _MSC_VER < 1930
#    define COMPILER_STRING "MSVC 2019"
#  elif _MSC_VER < 1950
#    define COMPILER_STRING "MSVC 2022"
#  elif _MSC_VER < 2000
#    define COMPILER_STRING "MSVC 2026"
#  else
#    define COMPILER_STRING "MSVC _MSC_VER " QT_STRINGIFY(_MSC_VER)
#  endif
#else
#  define COMPILER_STRING "<unknown compiler>"
#endif
#ifdef QT_NO_DEBUG
#  define DEBUG_STRING " release"
#else
#  define DEBUG_STRING " debug"
#endif
#ifdef QT_SHARED
#  define SHARED_STRING " shared (dynamic)"
#else
#  define SHARED_STRING " static"
#endif
static const char *qt_build_string() noexcept
{
    return "Qt " QT_VERSION_STR " (" ARCH_FULL SHARED_STRING DEBUG_STRING " build; by " COMPILER_STRING ")";
}

/*!
  Returns a string describing how this version of Qt was built.

  \internal

  \since 5.3
*/

const char *QLibraryInfo::build() noexcept
{
    return qt_build_string();
}

/*!
    \since 5.0
    Returns \c true if this build of Qt was built with debugging enabled, or
    false if it was built in release mode.
*/
bool
QLibraryInfo::isDebugBuild() noexcept
{
#ifdef QT_DEBUG
    return true;
#else
    return false;
#endif
}

/*!
    \since 6.5
    Returns \c true if this is a shared (dynamic) build of Qt.
*/
bool QLibraryInfo::isSharedBuild() noexcept
{
#ifdef QT_SHARED
    return true;
#else
    return false;
#endif
}

/*!
    \since 5.8
    Returns the version of the Qt library.

    \sa qVersion()
*/
QVersionNumber QLibraryInfo::version() noexcept
{
    return QVersionNumber(QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);
}

/*!
    \internal

    Returns the qt.conf "Prefix" entry the application prefix is rooted at,
    falling back to its "." default when there's no qt.conf with paths (or
    settings support), so the prefix then resolves to the application directory.
*/
QString QLibraryPrefixes::qtConfPrefix()
{
#if QT_CONFIG(settings)
    QLibrarySettings *settings = qt_library_settings();
    if (settings && settings->havePaths()) {
        const QString prefix = settings->paths(QLibraryInfo::PrefixPath).value(0);
        if (!prefix.isEmpty())
            return prefix;
    }
#endif
    return u"."_s;
}

/*!
    \internal

    The directory the application prefix is rooted at: the app bundle on
    Apple platforms, or the application directory once a QCoreApplication
    exists. Returns an empty string when no stable source is available yet.
*/
QString QLibraryPrefixes::resolveAppDir()
{
#if defined(Q_OS_APPLE)
    // Resolve the app prefix from the app bundle instead of the
    // executable, as that correctly handles both the main executable
    // as well as possibly deeply nested helper tools, and lets us
    // root the app prefix at the base of the bundle. Note that
    // CFBundleGetMainBundle returns a bundle representation even
    // for unbundled apps, so we verify the URL is actually a bundle.
    if (CFBundleRef bundleRef = CFBundleGetMainBundle()) {
        if (QCFType<CFURLRef> urlRef = CFBundleCopyBundleURL(bundleRef)) {
            const QString bundlePath = QCFString(CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle));
            if (qt_apple_bundleType(bundlePath)) {
#if defined(Q_OS_MACOS)
                // On macOS the prefix is rooted at the bundle's Contents directory.
                return QFileInfo(bundlePath + "/Contents"_L1).canonicalFilePath();
#else
                return QFileInfo(bundlePath).canonicalFilePath(); // iOS
#endif // Q_OS_MACOS
            }
        }
    }
#endif // Q_OS_APPLE

    if (QCoreApplication::instanceExists()) {
        // We make the prefix path absolute to the executable's directory.
        return QCoreApplication::applicationDirPath();
    }

    // Without an application instance there's no stable source; return an
    // empty string so the result isn't cached, and let appPrefix() fall back
    // to the (volatile) working directory until a stable source is available.
    return {};
}

/*!
    \internal

    The fully resolved application prefix: the qt.conf "Prefix" entry
    (explicit, or the "." default) rooted at the application directory.
    An absolute Prefix is used as-is, independent of the application
    directory. Without a qt.conf the prefix is the application directory
    itself, which is also what static builds use as their Qt prefix.

    Returns an empty string when the result would depend on a not-yet-stable
    application directory, so it isn't cached; appPrefix() then roots the
    prefix at the (volatile) working directory until a stable source exists.
*/
QString QLibraryPrefixes::resolveAppPrefix()
{
    const QString prefix = qtConfPrefix();
    // An absolute qt.conf Prefix is stable on its own, regardless of appDir.
    if (!pathIsRelative(prefix))
        return prefix;
    const QString appDir = resolveAppDir();
    if (appDir.isEmpty())
        return {};
    return QDir::cleanPath(appDir + u'/' + prefix);
}

QString QLibraryPrefixes::appPrefix()
{
    QString prefix = cached(&QLibraryPrefixes::m_app, &QLibraryPrefixes::resolveAppPrefix);
    if (!prefix.isNull())
        return prefix;

    // No stable source yet; root the possible qt.conf Prefix at the volatile
    // working directory, without caching, until one is available.
    return QDir::cleanPath(QDir::currentPath() + u'/' + qtConfPrefix());
}

#if QT_CONFIG(relocatable)
#if !defined(QT_STATIC) && (QT_CONFIG(dlopen) || defined(Q_OS_WIN))
static QString prefixFromQtCoreLibraryHelper(const QString &qtCoreLibraryPath)
{
    const QString qtCoreLibrary = QDir::fromNativeSeparators(qtCoreLibraryPath);
    QString libDir = QFileInfo(qtCoreLibrary).absolutePath();

#if QT_CONFIG(framework)
# if defined(Q_OS_MACOS)
    // The library in a macOS framework lives in a `Versions/A/` subdirectory
    libDir += "/../.."_L1;
# endif
    // And both macOS and iOS frameworks are `.framework` bundle directories
    libDir += "/.."_L1;
#endif

    const QString prefixDir = libDir + "/" QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH;
    return QDir::cleanPath(prefixDir);
}
#endif

#if defined(Q_OS_WIN)
static HMODULE getWindowsModuleHandle()
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)&QLibraryInfo::isDebugBuild, &hModule);
    return hModule;
}
#endif // Q_OS_WIN

static QString relocatablePrefixFromQt()
{
    QString prefixPath;

    // For static builds, the prefix will be the app directory, unless it's a non-sandboxed Apple app.
    // For regular builds, the prefix will be relative to the location of the QtCore shared library.
#if defined(QT_STATIC)
# if defined(Q_OS_APPLE)
    // The default value for QT_CONFIG(relocatable) for static builds
    // used to be false, giving end users QT_CONFIGURE_PREFIX_PATH as
    // the prefix path. We've since changed the default to true for
    // Apple platforms, since sandboxed applications are always
    // relocated, even for static builds. To avoid regressions for
    // those that shipped non-sandboxed apps with static Qt relying
    // on the previous behavior, we fall back to the Qt configure
    // prefix for non-sandboxed apps.
    if (!qt_apple_isSandboxed())
        return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
# endif
    prefixPath = QLibraryPrefixes::appPrefix();
#elif defined(Q_OS_WASM)
    // Emscripten expects to find shared libraries at the root of the in-memory
    // file system when resolving dependencies for for dlopen() calls. So that's
    // where libqt6core.so would be.
    prefixPath = QStringLiteral("/");
#elif QT_CONFIG(dlopen)
    Dl_info info;
    int result = dladdr(reinterpret_cast<void *>(&QLibraryInfo::isDebugBuild), &info);
    if (result > 0 && info.dli_fname)
        prefixPath = prefixFromQtCoreLibraryHelper(QString::fromLocal8Bit(info.dli_fname));
#elif defined(Q_OS_WIN)
    HMODULE hModule = getWindowsModuleHandle();
    const int kBufferSize = 4096;
    wchar_t buffer[kBufferSize];
    DWORD pathSize = GetModuleFileName(hModule, buffer, kBufferSize);
    const QString qtCoreFilePath = QString::fromWCharArray(buffer, int(pathSize));
    const QString qtCoreDirPath = QFileInfo(qtCoreFilePath).absolutePath();

    hModule = reinterpret_cast<HMODULE>(QCoreApplicationPrivate::mainInstanceHandle);
    pathSize = GetModuleFileName(hModule, buffer, kBufferSize);
    const QString exeDirPath = QFileInfo(QString::fromWCharArray(buffer, int(pathSize))).absolutePath();
    if (QFileInfo(exeDirPath) == QFileInfo(qtCoreDirPath)) {
        // QtCore DLL is next to the executable. This is either a windeployqt'ed executable or an
        // executable within the QT_HOST_BIN directory. We're detecting the latter case by checking
        // whether there's an import library corresponding to our QtCore DLL in PREFIX/lib.
        QStringView libdir = qt_configure_strs.viewAt(QLibraryInfo::LibrariesPath - 1);
        const QLatin1Char slash('/');
#if defined(Q_CC_MINGW)
        const QString implibPrefix = QStringLiteral("lib");
        const QString implibSuffix = QStringLiteral(".a");
#else
        const QString implibPrefix;
        const QString implibSuffix = QStringLiteral(".lib");
#endif
        const QString qtCoreImpLibFileName = implibPrefix
                + QFileInfo(qtCoreFilePath).completeBaseName() + implibSuffix;
        const QString qtCoreImpLibPath = qtCoreDirPath
                + slash + QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH
                + slash + libdir
                + slash + qtCoreImpLibFileName;
        if (!QFileInfo::exists(qtCoreImpLibPath)) {
            // We did not find a corresponding import library and conclude that this is a
            // windeployqt'ed executable.
            return exeDirPath;
        }
    }
    if (!qtCoreFilePath.isEmpty())
        prefixPath = prefixFromQtCoreLibraryHelper(qtCoreFilePath);
#else
#error "The chosen platform / config does not support querying for a dynamic prefix."
#endif

#if defined(Q_OS_LINUX) && !defined(QT_STATIC) && defined(__GLIBC__)
    // QTBUG-78948: libQt5Core.so may be located in subdirectories below libdir.
    // See "Hardware capabilities" in the ld.so documentation and the Qt 5.3.0
    // changelog regarding SSE2 support.
    QString libdir = fromRawStringView(qt_configure_strs.viewAt(QLibraryInfo::LibrariesPath - 1));
    QDir prefixDir(prefixPath);
    while (!prefixDir.exists(libdir)) {
        prefixDir.cdUp();
        prefixPath = prefixDir.absolutePath();
        if (prefixDir.isRoot()) {
            prefixPath.clear();
            break;
        }
    }
#endif

    Q_ASSERT_X(!prefixPath.isEmpty(), "relocatablePrefixFromQt",
                                      "Failed to find the Qt prefix path.");
    return prefixPath;
}
#endif

QString QLibraryPrefixes::resolveQtPrefix()
{
#if QT_CONFIG(relocatable)
    return relocatablePrefixFromQt();
#else
    return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
#endif
}

QString QLibraryPrefixes::qtPrefix()
{
#if defined(QT_STATIC)
    // In static builds the Qt prefix resolves via the app directory (see
    // relocatablePrefixFromQt), which is not stable until an application
    // instance exists, and is cached and invalidated by appPrefix().
    // Don't cache it a second time here, to avoid freezing a transient
    // working-directory value.
    return resolveQtPrefix();
#else
    return cached(&QLibraryPrefixes::m_qt, &QLibraryPrefixes::resolveQtPrefix);
#endif
}

QLibraryInfoPrivate::LocationInfo QLibraryInfoPrivate::locationInfo(QLibraryInfo::LibraryPath location)
{
    /*
     * To add a new entry in QLibraryInfo::LibraryPath, add it to the enum
     * in qtbase/src/corelib/global/qlibraryinfo.h and:
     * - add its relative path in the qtConfEntries[] array below
     *   (the key is what appears in a qt.conf file)
     */
    static constexpr auto qtConfEntries = qOffsetStringArray(
        "Prefix", ".",
        "Documentation", "doc", // should be ${Data}/doc
        "Headers", "include",
        "Libraries", "lib",
#ifdef Q_OS_WIN
        "LibraryExecutables", "bin",
#else
        "LibraryExecutables", "libexec", // should be ${ArchData}/libexec
#endif
        "Binaries", "bin",
        "Plugins", "plugins", // should be ${ArchData}/plugins

        "QmlImports", "qml", // should be ${ArchData}/qml

        "ArchData", ".",
        "Data", ".",
        "Translations", "translations", // should be ${Data}/translations
        "Examples", "examples",
        "Tests", "tests"
    );
    [[maybe_unused]]
    constexpr QByteArrayView dot{"."};

    LocationInfo result;

    if (int(location) < qtConfEntries.count()) {
        result.key = QLatin1StringView(qtConfEntries.viewAt(location * 2));
        result.defaultValue = QLatin1StringView(qtConfEntries.viewAt(location * 2 + 1));
        if (result.key == u"QmlImports")
            result.fallbackKey = u"Qml2Imports"_s;
#if !defined(Q_OS_WIN) // On Windows we use the registry
    } else if (location == QLibraryInfo::SettingsPath) {
        result.key = "Settings"_L1;
        result.defaultValue = QLatin1StringView(dot);
#endif
    }

    return result;
}

#if defined(Q_OS_APPLE)
/*!
    \internal

    Relative suffixes for the modern Apple bundle layout, reported in addition to
    the unixy scheme when a prefix is a bundle. Root-agnostic: the same suffixes
    apply whether rooted at the app or the Qt prefix. Mirrors the conventions that
    macdeployqt and the CMake deployment machinery follow. Returns empty for
    locations with no modern equivalent (only the unixy suffix is then reported).
*/
static QStringList appleBundleSuffixes(QLibraryInfo::LibraryPath location)
{
    constexpr bool isMacOS =
        QOperatingSystemVersion::currentType() == QOperatingSystemVersion::MacOS;
    constexpr QStringView resourcesDir = isMacOS ? u"Resources/" : u"./";
    constexpr QStringView sharedSupportDir = u"SharedSupport/";

    switch (location) {
    case QLibraryInfo::BinariesPath:
        return { isMacOS ? u"MacOS"_s : u"."_s };
    case QLibraryInfo::PluginsPath:
        return { u"PlugIns"_s };
    case QLibraryInfo::LibrariesPath:
        return { u"Frameworks"_s };
    case QLibraryInfo::HeadersPath:
        return { u"Headers"_s };
    case QLibraryInfo::LibraryExecutablesPath:
        return { u"Helpers"_s };
    case QLibraryInfo::DataPath:
        return { resourcesDir.toString() };
    case QLibraryInfo::QmlImportsPath:
        return { resourcesDir + u"qml"_s };
    case QLibraryInfo::ArchDataPath:
        return { resourcesDir + QStringLiteral(ARCH_PROCESSOR) };
    case QLibraryInfo::TranslationsPath:
        return { resourcesDir + u"translations"_s };
    case QLibraryInfo::DocumentationPath:
        return { sharedSupportDir + u"doc"_s };
    case QLibraryInfo::ExamplesPath:
        return { sharedSupportDir + u"examples"_s };
    case QLibraryInfo::TestsPath:
        return { sharedSupportDir + u"tests"_s };
    case QLibraryInfo::PrefixPath:
    case QLibraryInfo::SettingsPath:
        return {};
    }

    Q_UNREACHABLE_RETURN({});
}
#endif

/*! \fn QString QLibraryInfo::location(LibraryLocation loc)
    \deprecated [6.0] Use path() instead.

    Returns the path specified by \a loc.
*/

/*!
    \since 6.0
    Returns the path specified by \a p.

    If there is more than one path listed in qt.conf, it will
    only return the first one.
    \sa paths
*/
QString QLibraryInfo::path(LibraryPath p)
{
    return QLibraryInfoPrivate::path(p);
}

/*!
    \since 6.8
    Returns all paths specificied by \a p.

    \sa path
 */
QStringList QLibraryInfo::paths(LibraryPath p)
{
    return QLibraryInfoPrivate::paths(p);
}

/*!
    Returns whether we have a user-written qt.conf with explicit paths.

    A user-written qt.conf is an expression of exclusivity: the
    application declares the layout it follows, so its paths replace the Qt
    ones, and the stable fallback-values from locationInfo() are rooted at the
    application prefix for the locations the qt.conf leaves out.

    A qt.conf with MergeQtConf = true is an artifact generated by the
    Qt QML CMake machinery to support development builds, and does not
    qualify as a user-written qt.conf.

    \internal
*/
static bool haveUserQtConf()
{
#if QT_CONFIG(settings)
    if (QLibrarySettings *qtConf = qt_library_settings()) {
        if (!qtConf->havePaths())
            return false;

        if (qtConf->configuration()->value("Config/MergeQtConf", false).toBool())
            return false;

        return true;
    }
#endif
    return false;
}

#if QT_CONFIG(settings)
QString QLibraryInfoPrivate::expandEnvVariables(QString ret)
{
    qsizetype startIndex = 0;
    /* We support placeholders of the form $(<ENV_VAR>) in qt.conf.
       The loop below tries to find all such placeholders, and replaces
       them with the actual value of the ENV_VAR environment variable
     */
    while (true) {
        startIndex = ret.indexOf(u'$', startIndex);
        if (startIndex < 0)
            break;
        if (ret.size() < startIndex + 3)
            break;
        if (ret.at(startIndex + 1) != u'(') {
            startIndex++;
            continue;
        }
        qsizetype endIndex = ret.indexOf(u')', startIndex + 2);
        if (endIndex < 0)
            break;
        auto envVarName = QStringView{ret}.sliced(startIndex + 2, endIndex - startIndex - 2);
        QString value = qEnvironmentVariable(envVarName.toLocal8Bit().constData());
        ret.replace(startIndex, endIndex - startIndex + 1, value);
        startIndex += value.size();
    }
    return QDir::fromNativeSeparators(ret);
}

#endif // settings

/*!
    \internal

    Roots any relative entries in \a paths at the directory returned by
    \a resolveBaseDir, which is resolved lazily (only when there's actually a
    relative path to root, and at most once).
*/
static QStringList makeAbsolute(QStringList paths, QString (*resolveBaseDir)())
{
    QString baseDir;
    for (QString &path : paths) {
        if (pathIsRelative(path)) {
            if (baseDir.isNull())
                baseDir = resolveBaseDir();
            path = QDir::cleanPath(baseDir + u'/' + std::move(path));
        }
    }
    return paths;
}

#if defined(Q_OS_APPLE)
/*!
    \internal

    Whether a resolved \a prefix is rooted at an Apple bundle. Path-based, so it
    works for any prefix (the app or the Qt prefix), not just the main bundle.
*/
static bool prefixIsBundle(const QString &prefix)
{
    QString bundlePath = prefix;
#if defined(Q_OS_MACOS)
    // On macOS the prefix is rooted at the bundle's Contents directory.
    // On iOS/visionOS the prefix already is the bundle directory.
    const QFileSystemEntry prefixInfo(prefix, QFileSystemEntry::FromInternalPath());
    if (prefixInfo.fileName() == "Contents"_L1)
        bundlePath = prefixInfo.path();
#endif
    return bool(qt_apple_bundleType(bundlePath));
}
#endif

/*!
    \internal

    All the paths \a location can resolve to, in order of priority, deduplicated.
*/
QStringList QLibraryInfoPrivate::paths(QLibraryInfo::LibraryPath location)
{
    // The paths a qt.conf lists explicitly always come first, as they are the only
    // ones anyone declared. What follows depends on whether the qt.conf was written
    // by the user, as described in haveUserQtConf().
    QStringList paths = qtConfPaths(location);

    if (!haveUserQtConf()) {
        // Without a user-written qt.conf the Qt-prefixed paths come first, so that an
        // application running against a non-bundled Qt keeps reporting that Qt's paths
        // first, for compatibility, as that's what consumers using path() rely on.
        paths += qtPaths(location);
    }

    // We always report the app-rooted paths, to allow picking up libraries,
    // plugins, and other resources from the app's location during both development
    // and deployment. And as we use paths() in Qt itself, for e.g. plugin or QML
    // module lookup, we will find them either way. This also covers an app bundle
    // nested inside an outer bundle that bundles Qt, for example an app extension
    // inside its main application, where each bundle may hold part of the
    // deployment story.
    paths += appPaths(location);

    paths.removeDuplicates();
    return paths;
}

/*!
    \internal

    The paths a qt.conf lists explicitly for \a location, rooted at the app
    prefix. Empty if there is no qt.conf, or it doesn't list the location.

    Both a user-written and a generated qt.conf are reported here. A generated
    one merges with the Qt build defaults, and is an overlay: the locations it
    lists still take priority, as that's the point of the mechanism, but it
    neither suppresses the other paths, nor implies anything about the locations
    it leaves out.
*/
QStringList QLibraryInfoPrivate::qtConfPaths(QLibraryInfo::LibraryPath location)
{
#if QT_CONFIG(settings)
    if (QLibrarySettings *qtConfSettings = qt_library_settings()) {
        if (location == QLibraryInfo::PrefixPath) {
            // An explicit Prefix is resolved against the application directory,
            // and not the app prefix like the other values, as we'd otherwise
            // end up double-applying any relative paths in the prefix.
            if (!qtConfSettings->paths(location).isEmpty())
                return { QLibraryPrefixes::appPrefix() };
            else
                return {};
        }

        return makeAbsolute(qtConfSettings->paths(location), &QLibraryPrefixes::appPrefix);
    }
#else
    Q_UNUSED(location);
#endif
    return {};
}

/*!
    \internal

    The paths implied by the application's own layout for \a location, rooted at
    the app prefix.

    We add the modern Apple suffixes when the app prefix is a bundle, followed by
    the Unixy stable fallback-value when a user-written qt.conf says the application
    follows that layout.

    Returns empty when neither applies: a plain executable with no qt.conf, or one
    with only a merging qt.conf, which doesn't imply that the application has paths
    of its own. A bundle always reports its Apple suffixes, whatever the qt.conf says.
*/
QStringList QLibraryInfoPrivate::appPaths(QLibraryInfo::LibraryPath location)
{
    bool appPrefixIsBundle = false;
#if defined(Q_OS_APPLE)
    appPrefixIsBundle = prefixIsBundle(QLibraryPrefixes::appPrefix());
#endif

    const bool appDeclaresItsLayout = haveUserQtConf();

    // The prefix itself is reported whenever anything implies that the
    // application has paths of its own, so that the application prefix stays
    // discoverable via QLibraryInfo, but not for a generated qt.conf alone, as
    // that only overlays the locations it lists.
    if (location == QLibraryInfo::PrefixPath) {
        if (appPrefixIsBundle || appDeclaresItsLayout)
            return { QLibraryPrefixes::appPrefix() };
        return {};
    }

    QStringList paths;

#if defined(Q_OS_APPLE)
    if (appPrefixIsBundle)
        paths += appleBundleSuffixes(location);
#endif

    // A bundle on its own only implies the modern Apple layout. The Unixy
    // layout is legacy in a bundle, and outside of one it's not implied by
    // anything but a user-written qt.conf, so we leave it to the Qt prefix
    // otherwise.
    if (appDeclaresItsLayout) {
        const auto locationInfo = QLibraryInfoPrivate::locationInfo(location);
        if (!locationInfo.defaultValue.isNull())
            paths << locationInfo.defaultValue;
    }

    return makeAbsolute(std::move(paths), &QLibraryPrefixes::appPrefix);
}

/*!
    \internal

    The Qt-prefixed paths for \a location, rooted at the Qt prefix. The Unixy
    configure-time suffix is preceded by the modern Apple suffixes when the Qt
    prefix is itself a bundle, which is the case when Qt was deployed into an
    application bundle.
*/
QStringList QLibraryInfoPrivate::qtPaths(QLibraryInfo::LibraryPath location)
{
    if (location == QLibraryInfo::PrefixPath)
        return { QLibraryPrefixes::qtPrefix() };

    QStringList paths;

#if defined(Q_OS_APPLE)
    if (prefixIsBundle(QLibraryPrefixes::qtPrefix()))
        paths += appleBundleSuffixes(location);
#endif

    if (int(location) <= qt_configure_strs.count()) {
        paths << fromRawStringView(qt_configure_strs.viewAt(location - 1));
#if !defined(Q_OS_WIN) // On Windows we use the registry
    } else if (location == QLibraryInfo::SettingsPath) {
        constexpr QStringView path = u"" QT_CONFIGURE_SETTINGS_PATH;
        paths << fromRawStringView(path);
#endif
    }

    return makeAbsolute(std::move(paths), &QLibraryPrefixes::qtPrefix);
}

/*
    Returns the path specified by \a p.
 */
QString QLibraryInfoPrivate::path(QLibraryInfo::LibraryPath location)
{
    return paths(location).value(0, QString());
}

/*!
  Returns additional arguments to the platform plugin matching
  \a platformName which can be specified as a string list using
  the key \c Arguments in a group called \c Platforms of the
  \c qt.conf  file.

  sa {Using qt.conf}

  \internal

  \since 5.3
*/

QStringList QLibraryInfo::platformPluginArguments(const QString &platformName)
{
#if QT_CONFIG(settings)
    if (const QSettings *settings = QLibraryInfoPrivate::configuration()) {
        const QString key = "Platforms/"_L1
                + platformName
                + "Arguments"_L1;
        return settings->value(key).toStringList();
    }
#else
    Q_UNUSED(platformName);
#endif // settings
    return QStringList();
}

/*!
    \enum QLibraryInfo::LibraryPath

    \keyword library location

    This enum type is used to query for a specific path:

    \value PrefixPath The default prefix for all paths.
    \value DocumentationPath The path to documentation upon install.
    \value HeadersPath The path to all headers.
    \value LibrariesPath The path to installed libraries.
    \value LibraryExecutablesPath The path to installed executables required by libraries at runtime.
    \value BinariesPath The path to installed Qt binaries (tools and applications).
    \value PluginsPath The path to installed Qt plugins.
    \value QmlImportsPath The path to installed QML extensions to import.
    \value Qml2ImportsPath This value is deprecated. Use QmlImportsPath instead.
    \value ArchDataPath The path to general architecture-dependent Qt data.
    \value DataPath The path to general architecture-independent Qt data.
    \value TranslationsPath The path to translation information for Qt strings.
    \value ExamplesPath The path to examples upon install.
    \value TestsPath The path to installed Qt testcases.
    \value SettingsPath The path to Qt settings. Not applicable on Windows.

    \sa path()
*/

/*!
    \typealias QLibraryInfo::LibraryLocation
    \deprecated [6.0] Use LibraryPath with QLibraryInfo::path() instead.
*/

/*!
    \headerfile <QtVersion>
    \inmodule QtCore
    \ingroup funclists
    \brief Information about which Qt version the application is running on,
           and the version it was compiled against.
*/

/*!
    \macro QT_VERSION_STR
    \relates <QtVersion>

    This macro expands to a string that specifies Qt's version number (for
    example, "6.1.2"). This is the version with which the application is
    compiled. This may be a different version than the version the application
    will find itself using at \e runtime.

    \sa qVersion(), QT_VERSION
*/

/*!
    \relates <QtVersion>

    Returns the version number of Qt at runtime as a string (for example,
    "6.1.2"). This is the version of the Qt library in use at \e runtime,
    which need not be the version the application was \e compiled with.

    \sa QT_VERSION_STR, QLibraryInfo::version()
*/

const char *qVersion() noexcept
{
    return QT_VERSION_STR;
}

#if QT_DEPRECATED_SINCE(6, 9)

bool qSharedBuild() noexcept
{
    return QLibraryInfo::isSharedBuild();
}

#endif // QT_DEPRECATED_SINCE(6, 9)

QT_END_NAMESPACE

#if defined(Q_CC_GNU) && defined(ELF_INTERPRETER)
#  include <elf.h>
#  include <stdio.h>
#  include <stdlib.h>

#include "private/qcoreapplication_p.h"

QT_WARNING_DISABLE_GCC("-Wformat-overflow")
QT_WARNING_DISABLE_GCC("-Wattributes")
QT_WARNING_DISABLE_CLANG("-Wattributes")
QT_WARNING_DISABLE_INTEL(2621)

#  if defined(Q_OS_LINUX)
#    include "minimum-linux_p.h"
#  endif
#  ifdef QT_ELF_NOTE_OS_TYPE
struct ElfNoteAbiTag
{
    static_assert(sizeof(Elf32_Nhdr) == sizeof(Elf64_Nhdr),
        "The size of an ELF note is wrong (should be 12 bytes)");
    struct Payload {
        Elf32_Word ostype = QT_ELF_NOTE_OS_TYPE;
        Elf32_Word major = QT_ELF_NOTE_OS_MAJOR;
        Elf32_Word minor = QT_ELF_NOTE_OS_MINOR;
#    ifdef QT_ELF_NOTE_OS_PATCH
        Elf32_Word patch = QT_ELF_NOTE_OS_PATCH;
#    endif
    };

    Elf32_Nhdr header = {
        .n_namesz = sizeof(name),
        .n_descsz = sizeof(Payload),
        .n_type = NT_GNU_ABI_TAG
    };
    char name[sizeof ELF_NOTE_GNU] = ELF_NOTE_GNU;  // yes, include the null terminator
    Payload payload = {};
};
__attribute__((section(".note.ABI-tag"), aligned(4), used))
extern constexpr ElfNoteAbiTag QT_MANGLE_NAMESPACE(qt_abi_tag) = {};
#  endif

extern const char qt_core_interpreter[] __attribute__((section(".interp")))
    = ELF_INTERPRETER;

extern "C" void qt_core_boilerplate() __attribute__((force_align_arg_pointer));
void qt_core_boilerplate()
{
    QT_USE_NAMESPACE

    printf("This is the QtCore library version %s\n"
           "%s\n"
           "Contact: https://www.qt.io/licensing/\n"
           "\n",
           QT_PREPEND_NAMESPACE(qt_build_string)(),
           QT_COPYRIGHT);

    QByteArray libPath =
            qt_configure_strs.viewAt(QLibraryInfo::LibrariesPath - 1).toUtf8();
    QByteArray pluginPath =
            qt_configure_strs.viewAt(QLibraryInfo::PluginsPath - 1).toUtf8();
#if QT_CONFIG(relocatable)
    QByteArray prefix = QLibraryPrefixes::qtPrefix().toUtf8();
#else
    QByteArrayView prefix = QT_CONFIGURE_PREFIX_PATH;
#endif
    printf("Installation prefix: %s\n"
           "Library path:        %s\n"
           "Plugin path:         %s\n",
           prefix.data(),
           libPath.data(),
           pluginPath.data());

    QT_PREPEND_NAMESPACE(qDumpCPUFeatures)();

    exit(0);
}

#endif
