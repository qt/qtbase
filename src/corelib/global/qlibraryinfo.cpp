/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include "qconfig.cpp"

#ifdef Q_OS_DARWIN
#  include "private/qcore_mac_p.h"
#endif // Q_OS_DARWIN

#include "archdetect.cpp"

#if QT_CONFIG(relocatable) && QT_CONFIG(dlopen) && !QT_CONFIG(framework)
#    include <dlfcn.h>
#endif

#if QT_CONFIG(relocatable) && defined(Q_OS_WIN)
#    include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

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
        paths = !children.contains(QLatin1String("Platforms"))
                || children.contains(QLatin1String("Paths"));
    }
}

static QSettings *findConfiguration()
{
    if (!QLibraryInfoPrivate::qtconfManualPath.isEmpty())
        return new QSettings(QLibraryInfoPrivate::qtconfManualPath, QSettings::IniFormat);

    QString qtconfig = QStringLiteral(":/qt/etc/qt.conf");
    if (QFile::exists(qtconfig))
        return new QSettings(qtconfig, QSettings::IniFormat);
#ifdef Q_OS_DARWIN
    CFBundleRef bundleRef = CFBundleGetMainBundle();
    if (bundleRef) {
        QCFType<CFURLRef> urlRef = CFBundleCopyResourceURL(bundleRef,
                                                           QCFString(QLatin1String("qt.conf")),
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
        qtconfig = pwd.filePath(QLatin1String("qt" QT_STRINGIFY(QT_VERSION_MAJOR) ".conf"));
        if (QFile::exists(qtconfig))
            return new QSettings(qtconfig, QSettings::IniFormat);
        qtconfig = pwd.filePath(QLatin1String("qt.conf"));
        if (QFile::exists(qtconfig))
            return new QSettings(qtconfig, QSettings::IniFormat);
    }
    return nullptr;     //no luck
}

QString QLibraryInfoPrivate::qtconfManualPath;

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

#if defined(Q_CC_INTEL) // must be before GNU, Clang and MSVC because ICC/ICL claim to be them
#  ifdef __INTEL_CLANG_COMPILER
#    define ICC_COMPAT "Clang"
#  elif defined(__INTEL_MS_COMPAT_LEVEL)
#    define ICC_COMPAT "Microsoft"
#  elif defined(__GNUC__)
#    define ICC_COMPAT "GCC"
#  else
#    define ICC_COMPAT "no"
#  endif
#  if __INTEL_COMPILER == 1300
#    define ICC_VERSION "13.0"
#  elif __INTEL_COMPILER == 1310
#    define ICC_VERSION "13.1"
#  elif __INTEL_COMPILER == 1400
#    define ICC_VERSION "14.0"
#  elif __INTEL_COMPILER == 1500
#    define ICC_VERSION "15.0"
#  else
#    define ICC_VERSION QT_STRINGIFY(__INTEL_COMPILER)
#  endif
#  ifdef __INTEL_COMPILER_UPDATE
#    define COMPILER_STRING "Intel(R) C++ " ICC_VERSION "." QT_STRINGIFY(__INTEL_COMPILER_UPDATE) \
                            " build " QT_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" \
                            ICC_COMPAT " compatibility]"
#  else
#    define COMPILER_STRING "Intel(R) C++ " ICC_VERSION \
                            " build " QT_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" \
                            ICC_COMPAT " compatibility]"
#  endif
#elif defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
#  ifdef __apple_build_version__ // Apple clang has other version numbers
#    define COMPILER_STRING "Clang " __clang_version__ " (Apple)"
#  else
#    define COMPILER_STRING "Clang " __clang_version__
#  endif
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
#define QT_BUILD_STR "Qt " QT_VERSION_STR " (" ARCH_FULL SHARED_STRING DEBUG_STRING " build; by " COMPILER_STRING ")"

/*!
  Returns a string describing how this version of Qt was built.

  \internal

  \since 5.3
*/

const char *QLibraryInfo::build() noexcept
{
    return QT_BUILD_STR;
}

/*!
    \since 5.0
    Returns \c true if this build of Qt was built with debugging enabled, or
    false if it was built in release mode.
*/
bool
QLibraryInfo::isDebugBuild()
{
#ifdef QT_DEBUG
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

/*
 * To add a new entry in QLibraryInfo::LibraryPath, add it to the enum
 * in qtbase/src/corelib/global/qlibraryinfo.h and:
 * - add its relative path in the qtConfEntries[] array below
 *   (the key is what appears in a qt.conf file)
 */

static const struct {
    char key[19], value[13];
} qtConfEntries[] = {
    { "Prefix", "." },
    { "Documentation", "doc" }, // should be ${Data}/doc
    { "Headers", "include" },
    { "Libraries", "lib" },
#ifdef Q_OS_WIN
    { "LibraryExecutables", "bin" },
#else
    { "LibraryExecutables", "libexec" }, // should be ${ArchData}/libexec
#endif
    { "Binaries", "bin" },
    { "Plugins", "plugins" }, // should be ${ArchData}/plugins
    { "Qml2Imports", "qml" }, // should be ${ArchData}/qml
    { "ArchData", "." },
    { "Data", "." },
    { "Translations", "translations" }, // should be ${Data}/translations
    { "Examples", "examples" },
    { "Tests", "tests" },
};

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
                QString bundleContentsDir = QString(path) + QLatin1String("/Contents/");
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
    const QString prefixDir = libDir + QLatin1Char('/')
            + QLatin1String(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH);
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

static QString getRelocatablePrefix()
{
    QString prefixPath;

    // For static builds, the prefix will be the app directory.
    // For regular builds, the prefix will be relative to the location of the QtCore shared library.
#if defined(QT_STATIC)
    prefixPath = prefixFromAppDirHelper();
#elif defined(Q_OS_DARWIN) && QT_CONFIG(framework)
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

    const QString prefixDir = QString(libDirCFString) + QLatin1Char('/')
        + QLatin1String(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH);

    prefixPath = QDir::cleanPath(prefixDir);
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
    pathSize = GetModuleFileName(NULL, buffer, kBufferSize);
    const QString exeDirPath = QFileInfo(QString::fromWCharArray(buffer, int(pathSize))).absolutePath();
    if (QFileInfo(exeDirPath) == QFileInfo(qtCoreDirPath)) {
        // QtCore DLL is next to the executable. This is either a windeployqt'ed executable or an
        // executable within the QT_HOST_BIN directory. We're detecting the latter case by checking
        // whether there's an import library corresponding to our QtCore DLL in PREFIX/lib.
        const QString libdir = QString::fromLocal8Bit(
            qt_configure_strs + qt_configure_str_offsets[QLibraryInfo::LibrariesPath - 1]);
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
                + slash + QLatin1String(QT_CONFIGURE_LIBLOCATION_TO_PREFIX_PATH)
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
        qt_configure_strs + qt_configure_str_offsets[QLibraryInfo::LibrariesPath - 1]);
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

static QString getPrefix()
{
#if QT_CONFIG(relocatable)
    return getRelocatablePrefix();
#else
    return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
#endif
}

void QLibraryInfoPrivate::keyAndDefault(QLibraryInfo::LibraryPath loc, QString *key,
                                              QString *value)
{
    if (unsigned(loc) < sizeof(qtConfEntries)/sizeof(qtConfEntries[0])) {
        *key = QLatin1String(qtConfEntries[loc].key);
        *value = QLatin1String(qtConfEntries[loc].value);
    }
#ifndef Q_OS_WIN // On Windows we use the registry
    else if (loc == QLibraryInfo::SettingsPath) {
        *key = QLatin1String("Settings");
        *value = QLatin1String(".");
    }
#endif
    else {
        key->clear();
        value->clear();
    }
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
    const LibraryPath loc = p;
    QString ret;
    bool fromConf = false;
#if QT_CONFIG(settings)
    if (havePaths()) {
        fromConf = true;

        QString key;
        QString defaultValue;
        QLibraryInfoPrivate::keyAndDefault(loc, &key, &defaultValue);
        if (!key.isNull()) {
            QSettings *config = QLibraryInfoPrivate::configuration();
            Q_ASSERT(config != nullptr);
            config->beginGroup(QLatin1String("Paths"));

            ret = config->value(key, defaultValue).toString();
            int startIndex = 0;
            forever {
                startIndex = ret.indexOf(QLatin1Char('$'), startIndex);
                if (startIndex < 0)
                    break;
                if (ret.length() < startIndex + 3)
                    break;
                if (ret.at(startIndex + 1) != QLatin1Char('(')) {
                    startIndex++;
                    continue;
                }
                int endIndex = ret.indexOf(QLatin1Char(')'), startIndex + 2);
                if (endIndex < 0)
                    break;
                auto envVarName = QStringView{ret}.mid(startIndex + 2, endIndex - startIndex - 2);
                QString value = QString::fromLocal8Bit(qgetenv(envVarName.toLocal8Bit().constData()));
                ret.replace(startIndex, endIndex - startIndex + 1, value);
                startIndex += value.length();
            }

            config->endGroup();

            ret = QDir::fromNativeSeparators(ret);
        }
    }
#endif // settings

    if (!fromConf) {
        // "volatile" here is a hack to prevent compilers from doing a
        // compile-time strlen() on "path". The issue is that Qt installers
        // will binary-patch the Qt installation paths -- in such scenarios, Qt
        // will be built with a dummy path, thus the compile-time result of
        // strlen is meaningless.
        const char * volatile path = nullptr;
        if (loc == PrefixPath) {
            ret = getPrefix();
        } else if (unsigned(loc) <= sizeof(qt_configure_str_offsets)/sizeof(qt_configure_str_offsets[0])) {
            path = qt_configure_strs + qt_configure_str_offsets[loc - 1];
#ifndef Q_OS_WIN // On Windows we use the registry
        } else if (loc == SettingsPath) {
            path = QT_CONFIGURE_SETTINGS_PATH;
#endif
        }

        if (path)
            ret = QString::fromLocal8Bit(path);
    }

    if (!ret.isEmpty() && QDir::isRelativePath(ret)) {
        QString baseDir;
        if (loc == PrefixPath) {
            baseDir = prefixFromAppDirHelper();
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = path(PrefixPath);
        }
        ret = QDir::cleanPath(baseDir + QLatin1Char('/') + ret);
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
        const QString key = QLatin1String("Platforms")
                + QLatin1Char('/')
                + platformName
                + QLatin1String("Arguments");
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
    \value Qml2ImportsPath The path to installed QML extensions to import.
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

QT_END_NAMESPACE

#if defined(Q_CC_GNU) && defined(ELF_INTERPRETER)
#  include <stdio.h>
#  include <stdlib.h>

#include "private/qcoreapplication_p.h"

QT_WARNING_DISABLE_GCC("-Wattributes")
QT_WARNING_DISABLE_CLANG("-Wattributes")
QT_WARNING_DISABLE_INTEL(2621)

extern const char qt_core_interpreter[] __attribute__((section(".interp")))
    = ELF_INTERPRETER;

extern "C" void qt_core_boilerplate() __attribute__((force_align_arg_pointer));
void qt_core_boilerplate()
{
    printf("This is the QtCore library version " QT_BUILD_STR "\n"
           "Copyright (C) 2016 The Qt Company Ltd.\n"
           "Contact: http://www.qt.io/licensing/\n"
           "\n"
           "Installation prefix: %s\n"
           "Library path:        %s\n"
           "Plugin path:         %s\n",
           qt_configure_prefix_path_str + 12,
           qt_configure_strs + qt_configure_str_offsets[QT_PREPEND_NAMESPACE(QLibraryInfo)::LibrariesPath - 1],
           qt_configure_strs + qt_configure_str_offsets[QT_PREPEND_NAMESPACE(QLibraryInfo)::PluginsPath - 1]);

    QT_PREPEND_NAMESPACE(qDumpCPUFeatures)();

    exit(0);
}

#endif
