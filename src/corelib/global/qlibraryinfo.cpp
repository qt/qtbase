/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "qscopedpointer.h"

#ifdef QT_BUILD_QMAKE
QT_BEGIN_NAMESPACE
extern QString qmake_libraryInfoFile();
QT_END_NAMESPACE
#else
# include "qcoreapplication.h"
#endif

#ifndef QT_BUILD_QMAKE_BOOTSTRAP
#  include "private/qglobal_p.h"
#  include "qconfig.cpp"
#endif

#ifdef Q_OS_DARWIN
#  include "private/qcore_mac_p.h"
#endif // Q_OS_DARWIN

#include "archdetect.cpp"

#if !defined(QT_BUILD_QMAKE) && QT_CONFIG(relocatable) && QT_CONFIG(dlopen) && !QT_CONFIG(framework)
#  include <dlfcn.h>
#endif

#if !defined(QT_BUILD_QMAKE) && QT_CONFIG(relocatable) && defined(Q_OS_WIN)
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

extern void qDumpCPUFeatures(); // in qsimd.cpp

#if QT_CONFIG(settings)

struct QLibrarySettings
{
    QLibrarySettings();
    void load();

    QScopedPointer<QSettings> settings;
#ifdef QT_BUILD_QMAKE
    bool haveDevicePaths;
    bool haveEffectiveSourcePaths;
    bool haveEffectivePaths;
    bool havePaths;
#else
    bool reloadOnQAppAvailable;
#endif
};
Q_GLOBAL_STATIC(QLibrarySettings, qt_library_settings)

class QLibraryInfoPrivate
{
public:
    static QSettings *findConfiguration();
#ifdef QT_BUILD_QMAKE
    static void reload()
    {
        if (qt_library_settings.exists())
            qt_library_settings->load();
    }
    static bool haveGroup(QLibraryInfo::PathGroup group)
    {
        QLibrarySettings *ls = qt_library_settings();
        return ls ? (group == QLibraryInfo::EffectiveSourcePaths
                     ? ls->haveEffectiveSourcePaths
                     : group == QLibraryInfo::EffectivePaths
                       ? ls->haveEffectivePaths
                       : group == QLibraryInfo::DevicePaths
                         ? ls->haveDevicePaths
                         : ls->havePaths) : false;
    }
#endif
    static QSettings *configuration()
    {
        QLibrarySettings *ls = qt_library_settings();
        if (ls) {
#ifndef QT_BUILD_QMAKE
            if (ls->reloadOnQAppAvailable && QCoreApplication::instance() != nullptr)
                ls->load();
#endif
            return ls->settings.data();
        } else {
            return nullptr;
        }
    }
};

static const char platformsSection[] = "Platforms";

QLibrarySettings::QLibrarySettings()
{
    load();
}

void QLibrarySettings::load()
{
    // If we get any settings here, those won't change when the application shows up.
    settings.reset(QLibraryInfoPrivate::findConfiguration());
#ifndef QT_BUILD_QMAKE
    reloadOnQAppAvailable = (settings.data() == nullptr && QCoreApplication::instance() == nullptr);
    bool haveDevicePaths;
    bool haveEffectivePaths;
    bool havePaths;
#endif
    if (settings) {
        // This code needs to be in the regular library, as otherwise a qt.conf that
        // works for qmake would break things for dynamically built Qt tools.
        QStringList children = settings->childGroups();
        haveDevicePaths = children.contains(QLatin1String("DevicePaths"));
#ifdef QT_BUILD_QMAKE
        haveEffectiveSourcePaths = children.contains(QLatin1String("EffectiveSourcePaths"));
#else
        // EffectiveSourcePaths is for the Qt build only, so needs no backwards compat trickery.
        bool haveEffectiveSourcePaths = false;
#endif
        haveEffectivePaths = haveEffectiveSourcePaths || children.contains(QLatin1String("EffectivePaths"));
        // Backwards compat: an existing but empty file is claimed to contain the Paths section.
        havePaths = (!haveDevicePaths && !haveEffectivePaths
                     && !children.contains(QLatin1String(platformsSection)))
                    || children.contains(QLatin1String("Paths"));
#ifndef QT_BUILD_QMAKE
        if (!havePaths)
            settings.reset(nullptr);
#else
    } else {
        haveDevicePaths = false;
        haveEffectiveSourcePaths = false;
        haveEffectivePaths = false;
        havePaths = false;
#endif
    }
}

QSettings *QLibraryInfoPrivate::findConfiguration()
{
#ifdef QT_BUILD_QMAKE
    QString qtconfig = qmake_libraryInfoFile();
    if (QFile::exists(qtconfig))
        return new QSettings(qtconfig, QSettings::IniFormat);
#else
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
        qtconfig = pwd.filePath(QLatin1String("qt.conf"));
        if (QFile::exists(qtconfig))
            return new QSettings(qtconfig, QSettings::IniFormat);
    }
#endif
    return nullptr;     //no luck
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

#ifndef QT_BUILD_QMAKE

/*!
    \internal

   You cannot create a QLibraryInfo, instead only the static functions are available to query
   information.
*/

QLibraryInfo::QLibraryInfo()
{ }

/*!
  \deprecated
  This function used to return the person to whom this build of Qt is licensed, now returns an empty string.
*/

#if QT_DEPRECATED_SINCE(5, 8)
QString
QLibraryInfo::licensee()
{
    return QString();
}
#endif

/*!
  \deprecated
  This function used to return the products that the license for this build of Qt has access to, now returns an empty string.
*/

#if QT_DEPRECATED_SINCE(5, 8)
QString
QLibraryInfo::licensedProducts()
{
    return QString();
}
#endif

/*!
    \since 4.6
    \deprecated
    This function used to return the installation date for this build of Qt, but now returns a constant date.
*/
#if QT_CONFIG(datestring)
#if QT_DEPRECATED_SINCE(5, 5)
QDate
QLibraryInfo::buildDate()
{
    return QDate::fromString(QString::fromLatin1("2012-12-20"), Qt::ISODate);
}
#endif
#endif // datestring

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
#  elif _MSC_VER < 2000
#    define COMPILER_STRING "MSVC 2019"
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

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.8
    Returns the version of the Qt library.

    \sa qVersion()
*/
QVersionNumber QLibraryInfo::version() noexcept
{
    return QVersionNumber(QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);
}
#endif // QT_BOOTSTRAPPED

#endif // QT_BUILD_QMAKE

/*
 * To add a new entry in QLibrary::LibraryLocation, add it to the enum above the bootstrapped values and:
 * - add its relative path in the qtConfEntries[] array below
 *   (the key is what appears in a qt.conf file)
 * - add a property name in qmake/property.cpp propList[] array
 *   (it's used with qmake -query)
 * - add to qt_config.prf, qt_module.prf, qt_module_fwdpri.prf
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
    { "Imports", "imports" }, // should be ${ArchData}/imports
    { "Qml2Imports", "qml" }, // should be ${ArchData}/qml
    { "ArchData", "." },
    { "Data", "." },
    { "Translations", "translations" }, // should be ${Data}/translations
    { "Examples", "examples" },
    { "Tests", "tests" },
#ifdef QT_BUILD_QMAKE
    { "Sysroot", "" },
    { "SysrootifyPrefix", "" },
    { "HostBinaries", "bin" },
    { "HostLibraries", "lib" },
    { "HostData", "." },
    { "TargetSpec", "" },
    { "HostSpec", "" },
    { "HostPrefix", "" },
#endif
};

#ifdef QT_BUILD_QMAKE
void QLibraryInfo::reload()
{
    QLibraryInfoPrivate::reload();
}

void QLibraryInfo::sysrootify(QString *path)
{
    if (!QVariant::fromValue(rawLocation(SysrootifyPrefixPath, FinalPaths)).toBool())
        return;

    const QString sysroot = rawLocation(SysrootPath, FinalPaths);
    if (sysroot.isEmpty())
        return;

    if (path->length() > 2 && path->at(1) == QLatin1Char(':')
        && (path->at(2) == QLatin1Char('/') || path->at(2) == QLatin1Char('\\'))) {
        path->replace(0, 2, sysroot); // Strip out the drive on Windows targets
    } else {
        path->prepend(sysroot);
    }
}
#endif // QT_BUILD_QMAKE

#ifndef QT_BUILD_QMAKE
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
#endif

#if !defined(QT_BUILD_QMAKE) && QT_CONFIG(relocatable)
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
#if defined(Q_OS_WINRT)
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
static HMODULE getWindowsModuleHandle()
{
    return reinterpret_cast<HMODULE>(&__ImageBase);
}
#else  // Q_OS_WINRT
static HMODULE getWindowsModuleHandle()
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)&QLibraryInfo::isDebugBuild, &hModule);
    return hModule;
}
#endif // !Q_OS_WINRT
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

#if defined(QT_BUILD_QMAKE) && !defined(QT_BUILD_QMAKE_BOOTSTRAP)
QString qmake_abslocation();

static QString getPrefixFromHostBinDir(const char *hostBinDirToPrefixPath)
{
    const QFileInfo qmfi = QFileInfo(qmake_abslocation()).canonicalFilePath();
    return QDir::cleanPath(qmfi.absolutePath() + QLatin1Char('/')
                           + QLatin1String(hostBinDirToPrefixPath));
}

static QString getExtPrefixFromHostBinDir()
{
    return getPrefixFromHostBinDir(QT_CONFIGURE_HOSTBINDIR_TO_EXTPREFIX_PATH);
}

static QString getHostPrefixFromHostBinDir()
{
    return getPrefixFromHostBinDir(QT_CONFIGURE_HOSTBINDIR_TO_HOSTPREFIX_PATH);
}
#endif

#ifndef QT_BUILD_QMAKE_BOOTSTRAP
static QString getPrefix(
#ifdef QT_BUILD_QMAKE
        QLibraryInfo::PathGroup group
#endif
        )
{
#if defined(QT_BUILD_QMAKE)
#  if QT_CONFIGURE_CROSSBUILD
    if (group == QLibraryInfo::DevicePaths)
        return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
#  endif
    return getExtPrefixFromHostBinDir();
#elif QT_CONFIG(relocatable)
    return getRelocatablePrefix();
#else
    return QString::fromLocal8Bit(QT_CONFIGURE_PREFIX_PATH);
#endif
}
#endif // QT_BUILD_QMAKE_BOOTSTRAP

/*!
  Returns the location specified by \a loc.
*/
QString
QLibraryInfo::location(LibraryLocation loc)
{
#ifdef QT_BUILD_QMAKE // ends inside rawLocation !
    QString ret = rawLocation(loc, FinalPaths);

    // Automatically prepend the sysroot to target paths
    if (loc < SysrootPath || loc > LastHostPath)
        sysrootify(&ret);

    return ret;
}

QString
QLibraryInfo::rawLocation(LibraryLocation loc, PathGroup group)
{
#endif // QT_BUILD_QMAKE, started inside location !
    QString ret;
    bool fromConf = false;
#if QT_CONFIG(settings)
#ifdef QT_BUILD_QMAKE
    // Logic for choosing the right data source: if EffectivePaths are requested
    // and qt.conf with that section is present, use it, otherwise fall back to
    // FinalPaths. For FinalPaths, use qt.conf if present and contains not only
    // [EffectivePaths], otherwise fall back to builtins.
    // EffectiveSourcePaths falls back to EffectivePaths.
    // DevicePaths falls back to FinalPaths.
    PathGroup orig_group = group;
    if (QLibraryInfoPrivate::haveGroup(group)
        || (group == EffectiveSourcePaths
            && (group = EffectivePaths, QLibraryInfoPrivate::haveGroup(group)))
        || ((group == EffectivePaths || group == DevicePaths)
            && (group = FinalPaths, QLibraryInfoPrivate::haveGroup(group)))
        || (group = orig_group, false))
#else
    if (QLibraryInfoPrivate::configuration())
#endif
    {
        fromConf = true;

        QString key;
        QString defaultValue;
        if (unsigned(loc) < sizeof(qtConfEntries)/sizeof(qtConfEntries[0])) {
            key = QLatin1String(qtConfEntries[loc].key);
            defaultValue = QLatin1String(qtConfEntries[loc].value);
        }
#ifndef Q_OS_WIN // On Windows we use the registry
        else if (loc == SettingsPath) {
            key = QLatin1String("Settings");
            defaultValue = QLatin1String(".");
        }
#endif

        if(!key.isNull()) {
            QSettings *config = QLibraryInfoPrivate::configuration();
            config->beginGroup(QLatin1String(
#ifdef QT_BUILD_QMAKE
                   group == DevicePaths ? "DevicePaths" :
                   group == EffectiveSourcePaths ? "EffectiveSourcePaths" :
                   group == EffectivePaths ? "EffectivePaths" :
#endif
                                             "Paths"));

            ret = config->value(key, defaultValue).toString();

#ifdef QT_BUILD_QMAKE
            if (ret.isEmpty()) {
                if (loc == HostPrefixPath)
                    ret = config->value(QLatin1String(qtConfEntries[PrefixPath].key),
                                        QLatin1String(qtConfEntries[PrefixPath].value)).toString();
                else if (loc == TargetSpecPath || loc == HostSpecPath || loc == SysrootifyPrefixPath)
                    fromConf = false;
                // The last case here is SysrootPath, which can be legitimately empty.
                // All other keys have non-empty fallbacks to start with.
            }
#endif

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
                QStringRef envVarName = ret.midRef(startIndex + 2, endIndex - startIndex - 2);
                QString value = QString::fromLocal8Bit(qgetenv(envVarName.toLocal8Bit().constData()));
                ret.replace(startIndex, endIndex - startIndex + 1, value);
                startIndex += value.length();
            }

            config->endGroup();

            ret = QDir::fromNativeSeparators(ret);
        }
    }
#endif // settings

#ifndef QT_BUILD_QMAKE_BOOTSTRAP
    if (!fromConf) {
        // "volatile" here is a hack to prevent compilers from doing a
        // compile-time strlen() on "path". The issue is that Qt installers
        // will binary-patch the Qt installation paths -- in such scenarios, Qt
        // will be built with a dummy path, thus the compile-time result of
        // strlen is meaningless.
        const char * volatile path = nullptr;
        if (loc == PrefixPath) {
            ret = getPrefix(
#ifdef QT_BUILD_QMAKE
                        group
#endif
                   );
        } else if (unsigned(loc) <= sizeof(qt_configure_str_offsets)/sizeof(qt_configure_str_offsets[0])) {
            path = qt_configure_strs + qt_configure_str_offsets[loc - 1];
#ifndef Q_OS_WIN // On Windows we use the registry
        } else if (loc == SettingsPath) {
            path = QT_CONFIGURE_SETTINGS_PATH;
#endif
# ifdef QT_BUILD_QMAKE
        } else if (loc == HostPrefixPath) {
            static const QByteArray hostPrefixPath = getHostPrefixFromHostBinDir().toLatin1();
            path = hostPrefixPath.constData();
# endif
        }

        if (path)
            ret = QString::fromLocal8Bit(path);
    }
#endif

#ifdef QT_BUILD_QMAKE
    // These values aren't actually paths and thus need to be returned verbatim.
    if (loc == TargetSpecPath || loc == HostSpecPath || loc == SysrootifyPrefixPath)
        return ret;
#endif

    if (!ret.isEmpty() && QDir::isRelativePath(ret)) {
        QString baseDir;
#ifdef QT_BUILD_QMAKE
        if (loc == HostPrefixPath || loc == PrefixPath || loc == SysrootPath) {
            // We make the prefix/sysroot path absolute to the executable's directory.
            // loc == PrefixPath while a sysroot is set would make no sense here.
            // loc == SysrootPath only makes sense if qmake lives inside the sysroot itself.
            baseDir = QFileInfo(qmake_libraryInfoFile()).absolutePath();
        } else if (loc > SysrootPath && loc <= LastHostPath) {
            // We make any other host path absolute to the host prefix directory.
            baseDir = rawLocation(HostPrefixPath, group);
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = rawLocation(PrefixPath, group);
            if (group == EffectivePaths)
                sysrootify(&baseDir);
        }
#else
        if (loc == PrefixPath) {
            baseDir = prefixFromAppDirHelper();
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = location(PrefixPath);
        }
#endif // QT_BUILD_QMAKE
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
#if !defined(QT_BUILD_QMAKE) && QT_CONFIG(settings)
    QScopedPointer<const QSettings> settings(QLibraryInfoPrivate::findConfiguration());
    if (!settings.isNull()) {
        const QString key = QLatin1String(platformsSection)
                + QLatin1Char('/')
                + platformName
                + QLatin1String("Arguments");
        return settings->value(key).toStringList();
    }
#else
    Q_UNUSED(platformName);
#endif // !QT_BUILD_QMAKE && settings
    return QStringList();
}

/*!
    \enum QLibraryInfo::LibraryLocation

    \keyword library location

    This enum type is used to specify a specific location
    specifier:

    \value PrefixPath The default prefix for all paths.
    \value DocumentationPath The location for documentation upon install.
    \value HeadersPath The location for all headers.
    \value LibrariesPath The location of installed libraries.
    \value LibraryExecutablesPath The location of installed executables required by libraries at runtime.
    \value BinariesPath The location of installed Qt binaries (tools and applications).
    \value PluginsPath The location of installed Qt plugins.
    \value ImportsPath The location of installed QML extensions to import (QML 1.x).
    \value Qml2ImportsPath The location of installed QML extensions to import (QML 2.x).
    \value ArchDataPath The location of general architecture-dependent Qt data.
    \value DataPath The location of general architecture-independent Qt data.
    \value TranslationsPath The location of translation information for Qt strings.
    \value ExamplesPath The location for examples upon install.
    \value TestsPath The location of installed Qt testcases.
    \value SettingsPath The location for Qt settings. Not applicable on Windows.

    \sa location()
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
