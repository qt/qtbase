// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdir.h"
#include "qstringlist.h"
#include "qfile.h"
#if QT_CONFIG(settings)
#include "qsettings.h"
#endif
#include "qlibraryinfo.h"
#include "qlibraryinfo_p.h"
#include "qscopedpointer.h"

#include "qcoreapplication.h"

#include "private/qglobal_p.h"
#include "archdetect.cpp"
#include "qconfig.cpp"

#ifdef Q_OS_DARWIN
#  include "private/qcore_mac_p.h"
#endif // Q_OS_DARWIN

#if QT_CONFIG(relocatable) && QT_CONFIG(dlopen) && !QT_CONFIG(framework)
#    include <dlfcn.h>
#endif

#if QT_CONFIG(relocatable) && defined(Q_OS_WIN)
#    include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

extern void qDumpCPUFeatures(); // in qsimd.cpp

#if QT_CONFIG(settings)

static QSettings *findConfiguration();

struct QLibrarySettings
{
    QLibrarySettings();
    void load();
    bool havePaths();
    QSettings *configuration();

    QScopedPointer<QSettings> settings;
    bool paths;
    bool reloadOnQAppAvailable;
};
Q_GLOBAL_STATIC(QLibrarySettings, qt_library_settings)

QLibrarySettings::QLibrarySettings() : paths(false), reloadOnQAppAvailable(false)
{
    load();
}

QSettings *QLibrarySettings::configuration()
{
    if (reloadOnQAppAvailable && QCoreApplication::instance() != nullptr)
        load();
    return settings.data();
}

bool QLibrarySettings::havePaths()
{
    if (reloadOnQAppAvailable && QCoreApplication::instance() != nullptr)
        load();
    return paths;
}

void QLibrarySettings::load()
{
    // If we get any settings here, those won't change when the application shows up.
    settings.reset(findConfiguration());
    reloadOnQAppAvailable = (settings.data() == nullptr && QCoreApplication::instance() == nullptr);

    if (settings) {
        // This code needs to be in the regular library, as otherwise a qt.conf that
        // works for qmake would break things for dynamically built Qt tools.
        QStringList children = settings->childGroups();
        paths = !children.contains("Platforms"_L1)
                || children.contains("Paths"_L1);
    }
}

static QSettings *findConfiguration()
{
    if (QLibraryInfoPrivate::qtconfManualPath)
        return new QSettings(*QLibraryInfoPrivate::qtconfManualPath, QSettings::IniFormat);

    QString qtconfig = QStringLiteral(":/qt/etc/qt.conf");
    if (QFile::exists(qtconfig))
        return new QSettings(qtconfig, QSettings::IniFormat);
#ifdef Q_OS_DARWIN
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
                return new QSettings(qtconfig, QSettings::IniFormat);
        }
    }
#endif
    if (QCoreApplication::instance()) {
        QDir pwd(QCoreApplication::applicationDirPath());
        qtconfig = pwd.filePath(u"qt" QT_STRINGIFY(QT_VERSION_MAJOR) ".conf"_s);
        if (QFile::exists(qtconfig))
            return new QSettings(qtconfig, QSettings::IniFormat);
        qtconfig = pwd.filePath("qt.conf"_L1);
        if (QFile::exists(qtconfig))
            return new QSettings(qtconfig, QSettings::IniFormat);
    }
    return nullptr;     //no luck
}

const QString *QLibraryInfoPrivate::qtconfManualPath = nullptr;

QSettings *QLibraryInfoPrivate::configuration()
{
    QLibrarySettings *ls = qt_library_settings();
    return ls ? ls->configuration() : nullptr;
}

void QLibraryInfoPrivate::reload()
{
    if (qt_library_settings.exists())
        qt_library_settings->load();
}

static bool havePaths() {
    QLibrarySettings *ls = qt_library_settings();
    return ls && ls->havePaths();
}

#endif // settings

/*!
    \class QLibraryInfo
    \inmodule QtCore
    \brief The QLibraryInfo class provides information about the Qt library.

    Many pieces of information are established when Qt is configured and built.
    This class provides an abstraction for accessing that information.
    By using the static functions of this class, an application can obtain
    information about the instance of the Qt library which the application
    is using at run-time.

    You can also use a \c qt.conf file to override the hard-coded paths
    that are compiled into the Qt library. For more information, see
    the \l {Using qt.conf} documentation.

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
#  elif _MSC_VER < 2000
#    define COMPILER_STRING "MSVC 2022"
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

static QString prefixFromAppDirHelper()
{
    QString appDir;

    if (QCoreApplication::instance()) {
#ifdef Q_OS_DARWIN
        CFBundleRef bundleRef = CFBundleGetMainBundle();
        if (bundleRef) {
            QCFType<CFURLRef> urlRef = CFBundleCopyBundleURL(bundleRef);
            if (urlRef) {
                QCFString path = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
#ifdef Q_OS_MACOS
                QString bundleContentsDir = QString(path) + "/Contents/"_L1;
                if (QDir(bundleContentsDir).exists())
                    return QDir::cleanPath(bundleContentsDir);
#else
                return QDir::cleanPath(QString(path)); // iOS
#endif // Q_OS_MACOS
            }
        }
#endif // Q_OS_DARWIN
        // We make the prefix path absolute to the executable's directory.
        appDir = QCoreApplication::applicationDirPath();
    } else {
        appDir = QDir::currentPath();
    }

    return appDir;
}

#if QT_CONFIG(relocatable)
#if !defined(QT_STATIC) && !(defined(Q_OS_DARWIN) && QT_CONFIG(framework)) \
        && (QT_CONFIG(dlopen) || defined(Q_OS_WIN))
static QString prefixFromQtCoreLibraryHelper(const QString &qtCoreLibraryPath)
{
    const QString qtCoreLibrary = QDir::fromNativeSeparators(qtCoreLibraryPath);
    const QString libDir = QFileInfo(qtCoreLibrary).absolutePath();
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

static QString getRelocatablePrefix(QLibraryInfoPrivate::UsageMode usageMode)
{
    QString prefixPath;

    // For static builds, the prefix will be the app directory.
    // For regular builds, the prefix will be relative to the location of the QtCore shared library.
#if defined(QT_STATIC)
    prefixPath = prefixFromAppDirHelper();
    if (usageMode == QLibraryInfoPrivate::UsedFromQtBinDir) {
        // For Qt tools in a static build, we must chop off the bin directory.
        constexpr QByteArrayView binDir = qt_configure_strs.viewAt(QLibraryInfo::BinariesPath - 1);
        constexpr size_t binDirLength = binDir.size() + 1;
        prefixPath.chop(binDirLength);
    }
#elif defined(Q_OS_DARWIN) && QT_CONFIG(framework)
    Q_UNUSED(usageMode);
#ifndef QT_LIBINFIX
    #define QT_LIBINFIX ""
#endif
    auto qtCoreBundle = CFBundleGetBundleWithIdentifier(CFSTR("org.qt-project.QtCore" QT_LIBINFIX));
    if (!qtCoreBundle) {
        // When running Qt apps over Samba shares, CoreFoundation will fail to find
        // the Resources directory inside the bundle, This directory is a symlink,
        // and CF relies on readdir() and dtent.dt_type to detect symlinks, which
        // does not work reliably for Samba shares. We work around it by manually
        // looking for the QtCore bundle.
        auto allBundles = CFBundleGetAllBundles();
        auto bundleCount = CFArrayGetCount(allBundles);
        for (int i = 0; i < bundleCount; ++i) {
            auto bundle = CFBundleRef(CFArrayGetValueAtIndex(allBundles, i));
            auto url = QCFType<CFURLRef>(CFBundleCopyBundleURL(bundle));
            auto path = QCFType<CFStringRef>(CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle));
            if (CFStringHasSuffix(path, CFSTR("/QtCore" QT_LIBINFIX ".framework"))) {
                qtCoreBundle = bundle;
                break;
            }
        }
    }
    Q_ASSERT(qtCoreBundle);

    QCFType<CFURLRef> qtCorePath = CFBundleCopyBundleURL(qtCoreBundle);
    Q_ASSERT(qtCorePath);

    QCFType<CFURLRef> qtCorePathAbsolute = CFURLCopyAbsoluteURL(qtCorePath);
    Q_ASSERT(qtCorePathAbsolute);

    QCFType<CFURLRef> libDirCFPath = CFURLCreateCopyDeletingLastPathComponent(NULL, qtCorePathAbsolute);

    const QCFString libDirCFString = CFURLCopyFileSystemPath(libDirCFPath, kCFURLPOSIXPathStyle);

    const QString prefixDir = QString(libDirCFString) + "/" QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH;

    prefixPath = QDir::cleanPath(prefixDir);
#elif QT_CONFIG(dlopen)
    Q_UNUSED(usageMode);
    Dl_info info;
    int result = dladdr(reinterpret_cast<void *>(&QLibraryInfo::isDebugBuild), &info);
    if (result > 0 && info.dli_fname)
        prefixPath = prefixFromQtCoreLibraryHelper(QString::fromLocal8Bit(info.dli_fname));
#elif defined(Q_OS_WIN)
    Q_UNUSED(usageMode);
    HMODULE hModule = getWindowsModuleHandle();
    const int kBufferSize = 4096;
    wchar_t buffer[kBufferSize];
    DWORD pathSize = GetModuleFileName(hModule, buffer, kBufferSize);
    const QString qtCoreFilePath = QString::fromWCharArray(buffer, int(pathSize));
    const QString qtCoreDirPath = QFileInfo(qtCoreFilePath).absolutePath();
    pathSize = GetModuleFileName(NULL, buffer, kBufferSize);
    const QString exeDirPath = QFileInfo(QString::fromWCharArray(buffer, int(pathSize))).absolutePath();
    if (QFileInfo(exeDirPath) == QFileInfo(qtCoreDirPath)) {
        // QtCore DLL is next to the executable. This is either a windeployqt'ed executable or an
        // executable within the QT_HOST_BIN directory. We're detecting the latter case by checking
        // whether there's an import library corresponding to our QtCore DLL in PREFIX/lib.
        const QString libdir = QString::fromLocal8Bit(
            qt_configure_strs.viewAt(QLibraryInfo::LibrariesPath - 1));
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
    const QString libdir = QString::fromLocal8Bit(
        qt_configure_strs.viewAt(QLibraryInfo::LibrariesPath - 1));
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

    Q_ASSERT_X(!prefixPath.isEmpty(), "getRelocatablePrefix",
                                      "Failed to find the Qt prefix path.");
    return prefixPath;
}
#endif

static QString getPrefix(QLibraryInfoPrivate::UsageMode usageMode)
{
#if QT_CONFIG(relocatable)
    return getRelocatablePrefix(usageMode);
#else
    Q_UNUSED(usageMode);
    return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
#endif
}

QLibraryInfoPrivate::LocationInfo QLibraryInfoPrivate::locationInfo(QLibraryInfo::LibraryPath loc)
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

    if (int(loc) < qtConfEntries.count()) {
        result.key = QLatin1StringView(qtConfEntries.viewAt(loc * 2));
        result.defaultValue = QLatin1StringView(qtConfEntries.viewAt(loc * 2 + 1));
        if (result.key == u"QmlImports")
            result.fallbackKey = u"Qml2Imports"_s;
#ifndef Q_OS_WIN // On Windows we use the registry
    } else if (loc == QLibraryInfo::SettingsPath) {
        result.key = "Settings"_L1;
        result.defaultValue = QLatin1StringView(dot);
#endif
    }

    return result;
}

/*! \fn QString QLibraryInfo::location(LibraryLocation loc)
    \deprecated [6.0] Use path() instead.

    Returns the path specified by \a loc.
*/

/*!
    \since 6.0
    Returns the path specified by \a p.
*/
QString QLibraryInfo::path(LibraryPath p)
{
    return QLibraryInfoPrivate::path(p);
}


/*
    Returns the path specified by \a p.

    The usage mode can be set to UsedFromQtBinDir to enable special handling for executables that
    live in <install-prefix>/bin.
 */
QString QLibraryInfoPrivate::path(QLibraryInfo::LibraryPath p, UsageMode usageMode)
{
    const QLibraryInfo::LibraryPath loc = p;
    QString ret;
    bool fromConf = false;
#if QT_CONFIG(settings)
    if (havePaths()) {
        fromConf = true;

        auto li = QLibraryInfoPrivate::locationInfo(loc);
        if (!li.key.isNull()) {
            QSettings *config = QLibraryInfoPrivate::configuration();
            Q_ASSERT(config != nullptr);
            config->beginGroup("Paths"_L1);

            if (li.fallbackKey.isNull()) {
                ret = config->value(li.key, li.defaultValue).toString();
            } else {
                QVariant v = config->value(li.key);
                if (!v.isValid())
                    v = config->value(li.fallbackKey, li.defaultValue);
                ret = v.toString();
            }

            qsizetype startIndex = 0;
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
                auto envVarName = QStringView{ret}.mid(startIndex + 2, endIndex - startIndex - 2);
                QString value = QString::fromLocal8Bit(qgetenv(envVarName.toLocal8Bit().constData()));
                ret.replace(startIndex, endIndex - startIndex + 1, value);
                startIndex += value.size();
            }

            config->endGroup();

            ret = QDir::fromNativeSeparators(ret);
        }
    }
#endif // settings

    if (!fromConf) {
        if (loc == QLibraryInfo::PrefixPath) {
            ret = getPrefix(usageMode);
        } else if (int(loc) <= qt_configure_strs.count()) {
            ret = QString::fromLocal8Bit(qt_configure_strs.viewAt(loc - 1));
#ifndef Q_OS_WIN // On Windows we use the registry
        } else if (loc == QLibraryInfo::SettingsPath) {
            // Use of volatile is a hack to discourage compilers from calling
            // strlen(), in the inlined fromLocal8Bit(const char *)'s body, at
            // compile-time, as Qt installers binary-patch the path, replacing
            // the dummy path seen at compile-time, typically changing length.
            const char *volatile path = QT_CONFIGURE_SETTINGS_PATH;
            ret = QString::fromLocal8Bit(path);
#endif
        }
    }

    if (!ret.isEmpty() && QDir::isRelativePath(ret)) {
        QString baseDir;
        if (loc == QLibraryInfo::PrefixPath) {
            baseDir = prefixFromAppDirHelper();
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = path(QLibraryInfo::PrefixPath, usageMode);
        }
        ret = QDir::cleanPath(baseDir + u'/' + ret);
    }
    return ret;
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
    QScopedPointer<const QSettings> settings(findConfiguration());
    if (!settings.isNull()) {
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
    \deprecated Use LibraryPath with QLibraryInfo::path() instead.
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
    printf("This is the QtCore library version %s\n"
           "Copyright (C) 2023 The Qt Company Ltd.\n"
           "Contact: http://www.qt.io/licensing/\n"
           "\n"
           "Installation prefix: %s\n"
           "Library path:        %s\n"
           "Plugin path:         %s\n",
           QT_PREPEND_NAMESPACE(qt_build_string)(),
           QT_CONFIGURE_PREFIX_PATH,
           qt_configure_strs[QT_PREPEND_NAMESPACE(QLibraryInfo)::LibrariesPath - 1],
           qt_configure_strs[QT_PREPEND_NAMESPACE(QLibraryInfo)::PluginsPath - 1]);

    QT_PREPEND_NAMESPACE(qDumpCPUFeatures)();

    exit(0);
}

#endif
