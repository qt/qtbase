// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsysinfo.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qstring.h>

#include <private/qoperatingsystemversion_p.h>

#ifdef Q_OS_UNIX
#  include <sys/utsname.h>
#  include <private/qcore_unix_p.h>
#endif

#ifdef Q_OS_ANDROID
#include <QtCore/private/qjnihelpers_p.h>
#include <qjniobject.h>
#endif

#if defined(Q_OS_SOLARIS)
#  include <sys/systeminfo.h>
#endif

#if defined(Q_OS_DARWIN)
#  include "qnamespace.h"
#  include <private/qcore_mac_p.h>
#  if __has_include(<IOKit/IOKitLib.h>)
#    include <IOKit/IOKitLib.h>
#  endif
#endif

#ifdef Q_OS_BSD4
#  include <sys/sysctl.h>
#endif

#ifdef Q_OS_WIN
#  include "qoperatingsystemversion_win_p.h"
#  include "private/qwinregistry_p.h"
#  include "qt_windows.h"
#endif // Q_OS_WIN

#include "archdetect.cpp"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QSysInfo
    \inmodule QtCore
    \brief The QSysInfo class provides information about the system.

    \list
    \li \l WordSize specifies the size of a pointer for the platform
       on which the application is compiled.
    \li \l ByteOrder specifies whether the platform is big-endian or
       little-endian.
    \endlist

    Some constants are defined only on certain platforms. You can use
    the preprocessor symbols Q_OS_WIN and Q_OS_MACOS to test that
    the application is compiled under Windows or \macos.

    \sa QLibraryInfo
*/

/*!
    \enum QSysInfo::Sizes

    This enum provides platform-specific information about the sizes of data
    structures used by the underlying architecture.

    \value WordSize The size in bits of a pointer for the platform on which
           the application is compiled (32 or 64).
*/

/*!
    \enum QSysInfo::Endian

    \value BigEndian  Big-endian byte order (also called Network byte order)
    \value LittleEndian  Little-endian byte order
    \value ByteOrder  Equals BigEndian or LittleEndian, depending on
                      the platform's byte order.
*/

#if defined(Q_OS_DARWIN)

static const char *osVer_helper(QOperatingSystemVersion version = QOperatingSystemVersion::current())
{
#ifdef Q_OS_MACOS
    switch (version.majorVersion()) {
    case 10: {
        switch (version.minorVersion()) {
        case 9: return "Mavericks";
        case 10: return "Yosemite";
        case 11: return "El Capitan";
        case 12: return "Sierra";
        case 13: return "High Sierra";
        case 14: return "Mojave";
        case 15: return "Catalina";
        case 16: return "Big Sur";
        default:
            Q_UNREACHABLE();
        }
    }
    case 11: return "Big Sur";
    case 12: return "Monterey";
    case 13: return "Ventura";
    case 14: return "Sonoma";
    case 15: return "Sequoia";
    case 26: return "Tahoe";
    default:
        // Unknown, future version
        break;
    }
#else
    Q_UNUSED(version);
#endif
    return nullptr;
}

#elif defined(Q_OS_WIN)

#  ifndef QT_BOOTSTRAPPED
class QWindowsSockInit
{
public:
    QWindowsSockInit();
    ~QWindowsSockInit();
    int version;
};

QWindowsSockInit::QWindowsSockInit()
:   version(0)
{
    WSAData wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        qWarning("QTcpSocketAPI: WinSock v2.2 initialization failed.");
    } else {
        version = 0x22;
    }
}

QWindowsSockInit::~QWindowsSockInit()
{
    WSACleanup();
}
Q_GLOBAL_STATIC(QWindowsSockInit, winsockInit)
#  endif // QT_BOOTSTRAPPED

static QString readVersionRegistryString(const wchar_t *subKey)
{
    return QWinRegistryKey(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)")
            .stringValue(subKey);
}

static inline QString windowsDisplayVersion()
{
    // https://tickets.puppetlabs.com/browse/FACT-3058
    // The "ReleaseId" key stopped updating since Windows 10 20H2.
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10_20H2)
        return readVersionRegistryString(L"DisplayVersion");
    else
        return readVersionRegistryString(L"ReleaseId");
}

static QString winSp_helper()
{
    const auto osv = qWindowsVersionInfo();
    const qint16 major = osv.wServicePackMajor;
    if (major) {
        QString sp = QStringLiteral("SP ") + QString::number(major);
        const qint16 minor = osv.wServicePackMinor;
        if (minor)
            sp += u'.' + QString::number(minor);

        return sp;
    }
    return QString();
}

static const char *osVer_helper(QOperatingSystemVersion version = QOperatingSystemVersion::current())
{
    Q_UNUSED(version);
    const OSVERSIONINFOEX osver = qWindowsVersionInfo();
    const bool workstation = osver.wProductType == VER_NT_WORKSTATION;

#define Q_WINVER(major, minor) (major << 8 | minor)
    switch (Q_WINVER(osver.dwMajorVersion, osver.dwMinorVersion)) {
    case Q_WINVER(10, 0):
        if (workstation) {
            if (osver.dwBuildNumber >= 22000)
                return "11";
            return "10";
        }
        // else: Server
        if (osver.dwBuildNumber >= 26100)
            return "Server 2025";
        if (osver.dwBuildNumber >= 20348)
            return "Server 2022";
        if (osver.dwBuildNumber >= 17763)
            return "Server 2019";
        return "Server 2016";
    }
#undef Q_WINVER
    // unknown, future version
    return nullptr;
}

#endif
#if defined(Q_OS_UNIX)
#  if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_FREEBSD)
#    define USE_ETC_OS_RELEASE
struct QUnixOSVersion
{
                                    // from /etc/os-release         older /etc/lsb-release         // redhat /etc/redhat-release         // debian /etc/debian_version
    QString productType;            // $ID                          $DISTRIB_ID                    // single line file containing:       // Debian
    QString productVersion;         // $VERSION_ID                  $DISTRIB_RELEASE               // <Vendor_ID release Version_ID>     // single line file <Release_ID/sid>
    QString prettyName;             // $PRETTY_NAME                 $DISTRIB_DESCRIPTION
};

static QString unquote(QByteArrayView str)
{
    // man os-release says:
    // Variable assignment values must be enclosed in double
    // or single quotes if they include spaces, semicolons or
    // other special characters outside of A–Z, a–z, 0–9. Shell
    // special characters ("$", quotes, backslash, backtick)
    // must be escaped with backslashes, following shell style.
    // All strings should be in UTF-8 format, and non-printable
    // characters should not be used. It is not supported to
    // concatenate multiple individually quoted strings.
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"')
        str = str.sliced(1).chopped(1);
    return QString::fromUtf8(str);
}

static QByteArray getEtcFileContent(const char *filename)
{
    // we're avoiding QFile here
    int fd = qt_safe_open(filename, O_RDONLY);
    if (fd == -1)
        return QByteArray();

    QT_STATBUF sbuf;
    if (QT_FSTAT(fd, &sbuf) == -1) {
        qt_safe_close(fd);
        return QByteArray();
    }

    QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
    buffer.resize(qt_safe_read(fd, buffer.data(), sbuf.st_size));
    qt_safe_close(fd);
    return buffer;
}

static bool readEtcFile(QUnixOSVersion &v, const char *filename,
                        const QByteArray &idKey, const QByteArray &versionKey, const QByteArray &prettyNameKey)
{

    QByteArray buffer = getEtcFileContent(filename);
    if (buffer.isEmpty())
        return false;

    const char *ptr = buffer.constData();
    const char *end = buffer.constEnd();
    const char *eol;
    QByteArray line;
    for (; ptr != end; ptr = eol + 1) {
        // find the end of the line after ptr
        eol = static_cast<const char *>(memchr(ptr, '\n', end - ptr));
        if (!eol)
            eol = end - 1;
        line.setRawData(ptr, eol - ptr);

        if (line.startsWith(idKey)) {
            ptr += idKey.size();
            v.productType = unquote({ptr, eol});
            continue;
        }

        if (line.startsWith(prettyNameKey)) {
            ptr += prettyNameKey.size();
            v.prettyName = unquote({ptr, eol});
            continue;
        }

        if (line.startsWith(versionKey)) {
            ptr += versionKey.size();
            v.productVersion = unquote({ptr, eol});
            continue;
        }
    }

    return true;
}

static bool readOsRelease(QUnixOSVersion &v)
{
    QByteArray id = QByteArrayLiteral("ID=");
    QByteArray versionId = QByteArrayLiteral("VERSION_ID=");
    QByteArray prettyName = QByteArrayLiteral("PRETTY_NAME=");

    // man os-release(5) says:
    // The file /etc/os-release takes precedence over /usr/lib/os-release.
    // Applications should check for the former, and exclusively use its data
    // if it exists, and only fall back to /usr/lib/os-release if it is
    // missing.
    return readEtcFile(v, "/etc/os-release", id, versionId, prettyName) ||
            readEtcFile(v, "/usr/lib/os-release", id, versionId, prettyName);
}

static bool readEtcLsbRelease(QUnixOSVersion &v)
{
    bool ok = readEtcFile(v, "/etc/lsb-release", QByteArrayLiteral("DISTRIB_ID="),
                          QByteArrayLiteral("DISTRIB_RELEASE="), QByteArrayLiteral("DISTRIB_DESCRIPTION="));
    if (ok && (v.prettyName.isEmpty() || v.prettyName == v.productType)) {
        // some distributions have redundant information for the pretty name,
        // so try /etc/<lowercasename>-release

        // we're still avoiding QFile here
        QByteArray distrorelease = "/etc/" + v.productType.toLatin1().toLower() + "-release";
        int fd = qt_safe_open(distrorelease, O_RDONLY);
        if (fd != -1) {
            QT_STATBUF sbuf;
            if (QT_FSTAT(fd, &sbuf) != -1 && sbuf.st_size > v.prettyName.size()) {
                // file apparently contains interesting information
                QByteArray buffer(sbuf.st_size, Qt::Uninitialized);
                buffer.resize(qt_safe_read(fd, buffer.data(), sbuf.st_size));
                v.prettyName = QString::fromLatin1(buffer.trimmed());
            }
            qt_safe_close(fd);
        }
    }

    // some distributions have a /etc/lsb-release file that does not provide the values
    // we are looking for, i.e. DISTRIB_ID, DISTRIB_RELEASE and DISTRIB_DESCRIPTION.
    // Assuming that neither DISTRIB_ID nor DISTRIB_RELEASE were found, or contained valid values,
    // returning false for readEtcLsbRelease will allow further /etc/<lowercasename>-release parsing.
    return ok && !(v.productType.isEmpty() && v.productVersion.isEmpty());
}

#if defined(Q_OS_LINUX)
static QByteArray getEtcFileFirstLine(const char *fileName)
{
    QByteArray buffer = getEtcFileContent(fileName);
    if (buffer.isEmpty())
        return QByteArray();

    const char *ptr = buffer.constData();
    return QByteArray(ptr, buffer.indexOf("\n")).trimmed();
}

static bool readEtcRedHatRelease(QUnixOSVersion &v)
{
    // /etc/redhat-release analysed should be a one line file
    // the format of its content is <Vendor_ID release Version>
    // i.e. "Red Hat Enterprise Linux Workstation release 6.5 (Santiago)"
    QByteArray line = getEtcFileFirstLine("/etc/redhat-release");
    if (line.isEmpty())
        return false;

    v.prettyName = QString::fromLatin1(line);

    const char keyword[] = "release ";
    const qsizetype releaseIndex = line.indexOf(keyword);
    v.productType = QString::fromLatin1(line.mid(0, releaseIndex)).remove(u' ');
    const qsizetype spaceIndex = line.indexOf(' ', releaseIndex + strlen(keyword));
    v.productVersion = QString::fromLatin1(line.mid(releaseIndex + strlen(keyword),
                                                    spaceIndex > -1 ? spaceIndex - releaseIndex - int(strlen(keyword)) : -1));
    return true;
}

static bool readEtcDebianVersion(QUnixOSVersion &v)
{
    // /etc/debian_version analysed should be a one line file
    // the format of its content is <Release_ID/sid>
    // i.e. "jessie/sid"
    QByteArray line = getEtcFileFirstLine("/etc/debian_version");
    if (line.isEmpty())
        return false;

    v.productType = QStringLiteral("Debian");
    v.productVersion = QString::fromLatin1(line);
    return true;
}
#endif

static bool findUnixOsVersion(QUnixOSVersion &v)
{
    if (readOsRelease(v))
        return true;
    if (readEtcLsbRelease(v))
        return true;
#if defined(Q_OS_LINUX)
    if (readEtcRedHatRelease(v))
        return true;
    if (readEtcDebianVersion(v))
        return true;
#endif
    return false;
}
#  endif // USE_ETC_OS_RELEASE
#endif // Q_OS_UNIX

#ifdef Q_OS_ANDROID
static const char *osVer_helper(QOperatingSystemVersion)
{
    // https://source.android.com/source/build-numbers.html
    // https://developer.android.com/guide/topics/manifest/uses-sdk-element.html#ApiLevels
    const int sdk_int = QtAndroidPrivate::androidSdkVersion();
    switch (sdk_int) {
    case 3:
        return "Cupcake";
    case 4:
        return "Donut";
    case 5:
    case 6:
    case 7:
        return "Eclair";
    case 8:
        return "Froyo";
    case 9:
    case 10:
        return "Gingerbread";
    case 11:
    case 12:
    case 13:
        return "Honeycomb";
    case 14:
    case 15:
        return "Ice Cream Sandwich";
    case 16:
    case 17:
    case 18:
        return "Jelly Bean";
    case 19:
    case 20:
        return "KitKat";
    case 21:
    case 22:
        return "Lollipop";
    case 23:
        return "Marshmallow";
    case 24:
    case 25:
        return "Nougat";
    case 26:
    case 27:
        return "Oreo";
    case 28:
        return "Pie";
    case 29:
        return "10";
    case 30:
        return "11";
    case 31:
        return "12";
    case 32:
        return "12L";
    case 33:
        return "13";
    default:
        break;
    }

    return "";
}
#endif

/*!
    \since 5.4

    Returns the architecture of the CPU that Qt was compiled for, in text
    format. Note that this may not match the actual CPU that the application is
    running on if there's an emulation layer or if the CPU supports multiple
    architectures (like x86-64 processors supporting i386 applications). To
    detect that, use currentCpuArchitecture().

    Values returned by this function are stable and will not change over time,
    so applications can rely on the returned value as an identifier, except
    that new CPU types may be added over time.

    Typical returned values are (note: list not exhaustive):
    \list
        \li "arm"
        \li "arm64"
        \li "i386"
        \li "ia64"
        \li "mips"
        \li "mips64"
        \li "power"
        \li "power64"
        \li "sparc"
        \li "sparcv9"
        \li "x86_64"
    \endlist

    \sa QSysInfo::buildAbi(), QSysInfo::currentCpuArchitecture()
*/
QString QSysInfo::buildCpuArchitecture()
{
    return QStringLiteral(ARCH_PROCESSOR);
}

/*!
    \since 5.4

    Returns the architecture of the CPU that the application is running on, in
    text format. Note that this function depends on what the OS will report and
    may not detect the actual CPU architecture if the OS hides that information
    or is unable to provide it. For example, a 32-bit OS running on a 64-bit
    CPU is usually unable to determine the CPU is actually capable of running
    64-bit programs.

    Values returned by this function are mostly stable: an attempt will be made
    to ensure that they stay constant over time and match the values returned
    by buildCpuArchitecture(). However, due to the nature of the
    operating system functions being used, there may be discrepancies.

    Typical returned values are (note: list not exhaustive):
    \list
        \li "arm"
        \li "arm64"
        \li "i386"
        \li "ia64"
        \li "mips"
        \li "mips64"
        \li "power"
        \li "power64"
        \li "sparc"
        \li "sparcv9"
        \li "x86_64"
    \endlist

    \sa QSysInfo::buildAbi(), QSysInfo::buildCpuArchitecture()
*/
QString QSysInfo::currentCpuArchitecture()
{
#if defined(Q_OS_WIN)
    // We don't need to catch all the CPU architectures in this function;
    // only those where the host CPU might be different than the build target
    // (usually, 64-bit platforms).
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
#  ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
        return QStringLiteral("x86_64");
#  endif
#  ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#  endif
    case PROCESSOR_ARCHITECTURE_IA64:
        return QStringLiteral("ia64");
    }
#elif defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)
    // iOS-based OSes do not return the architecture on uname(2)'s result.
    return buildCpuArchitecture();
#elif defined(Q_OS_UNIX)
    long ret = -1;
    struct utsname u;

#  if defined(Q_OS_SOLARIS)
    // We need a special call for Solaris because uname(2) on x86 returns "i86pc" for
    // both 32- and 64-bit CPUs. Reference:
    // http://docs.oracle.com/cd/E18752_01/html/816-5167/sysinfo-2.html#REFMAN2sysinfo-2
    // http://fxr.watson.org/fxr/source/common/syscall/systeminfo.c?v=OPENSOLARIS
    // http://fxr.watson.org/fxr/source/common/conf/param.c?v=OPENSOLARIS;im=10#L530
    if (ret == -1)
        ret = sysinfo(SI_ARCHITECTURE_64, u.machine, sizeof u.machine);
#  endif

    if (ret == -1)
        ret = uname(&u);

    // we could use detectUnixVersion() above, but we only need a field no other function does
    if (ret != -1) {
        // the use of QT_BUILD_INTERNAL here is simply to ensure all branches build
        // as we don't often build on some of the less common platforms
#  if defined(Q_PROCESSOR_ARM) || defined(QT_BUILD_INTERNAL)
        if (strcmp(u.machine, "aarch64") == 0)
            return QStringLiteral("arm64");
        if (strncmp(u.machine, "armv", 4) == 0)
            return QStringLiteral("arm");
#  endif
#  if defined(Q_PROCESSOR_POWER) || defined(QT_BUILD_INTERNAL)
        // harmonize "powerpc" and "ppc" to "power"
        if (strncmp(u.machine, "ppc", 3) == 0)
            return "power"_L1 + QLatin1StringView(u.machine + 3);
        if (strncmp(u.machine, "powerpc", 7) == 0)
            return "power"_L1 + QLatin1StringView(u.machine + 7);
        if (strcmp(u.machine, "Power Macintosh") == 0)
            return "power"_L1;
#  endif
#  if defined(Q_PROCESSOR_SPARC) || defined(QT_BUILD_INTERNAL)
        // Solaris sysinfo(2) (above) uses "sparcv9", but uname -m says "sun4u";
        // Linux says "sparc64"
        if (strcmp(u.machine, "sun4u") == 0 || strcmp(u.machine, "sparc64") == 0)
            return QStringLiteral("sparcv9");
        if (strcmp(u.machine, "sparc32") == 0)
            return QStringLiteral("sparc");
#  endif
#  if defined(Q_PROCESSOR_X86) || defined(QT_BUILD_INTERNAL)
        // harmonize all "i?86" to "i386"
        if (strlen(u.machine) == 4 && u.machine[0] == 'i'
                && u.machine[2] == '8' && u.machine[3] == '6')
            return QStringLiteral("i386");
        if (strcmp(u.machine, "amd64") == 0) // Solaris
            return QStringLiteral("x86_64");
#  endif
        return QString::fromLatin1(u.machine);
    }
#endif
    return buildCpuArchitecture();
}

/*!
    \since 5.4

    Returns the full architecture string that Qt was compiled for. This string
    is useful for identifying different, incompatible builds. For example, it
    can be used as an identifier to request an upgrade package from a server.

    The values returned from this function are kept stable as follows: the
    mandatory components of the result will not change in future versions of
    Qt, but optional suffixes may be added.

    The returned value is composed of three or more parts, separated by dashes
    ("-"). They are:

    \table
    \header \li Component           \li Value
    \row    \li CPU Architecture    \li The same as QSysInfo::buildCpuArchitecture(), such as "arm", "i386", "mips" or "x86_64"
    \row    \li Endianness          \li "little_endian" or "big_endian"
    \row    \li Word size           \li Whether it's a 32- or 64-bit application. Possible values are:
                                        "llp64" (Windows 64-bit), "lp64" (Unix 64-bit), "ilp32" (32-bit)
    \row    \li (Optional) ABI      \li Zero or more components identifying different ABIs possible in this architecture.
                                        Currently, Qt has optional ABI components for ARM and MIPS processors: one
                                        component is the main ABI (such as "eabi", "o32", "n32", "o64"); another is
                                        whether the calling convention is using hardware floating point registers ("hardfloat"
                                        is present).

                                        Additionally, if Qt was configured with \c{-qreal float}, the ABI option tag "qreal_float"
                                        will be present. If Qt was configured with another type as qreal, that type is present after
                                        "qreal_", with all characters other than letters and digits escaped by an underscore, followed
                                        by two hex digits. For example, \c{-qreal long double} becomes "qreal_long_20double".
    \endtable

    \sa QSysInfo::buildCpuArchitecture()
*/
QString QSysInfo::buildAbi()
{
    // ARCH_FULL is a concatenation of strings (incl. ARCH_PROCESSOR), which breaks
    // QStringLiteral on MSVC. Since the concatenation behavior we want is specified
    // the same C++11 paper as the Unicode strings, we'll use that macro and hope
    // that Microsoft implements the new behavior when they add support for Unicode strings.
    return QStringLiteral(ARCH_FULL);
}

static QString unknownText()
{
    return QStringLiteral("unknown");
}

/*!
    \since 5.4

    Returns the type of the operating system kernel Qt was compiled for. It's
    also the kernel the application is running on, unless the host operating
    system is running a form of compatibility or virtualization layer.

    Values returned by this function are stable and will not change over time,
    so applications can rely on the returned value as an identifier, except
    that new OS kernel types may be added over time.

    On Windows, this function returns the type of Windows kernel, like "winnt".
    On Unix systems, it returns the same as the output of \c{uname
    -s} (lowercased).

    \note This function may return surprising values: it returns "linux"
    for all operating systems running Linux (including Android), "qnx" for all
    operating systems running QNX, "freebsd" for
    Debian/kFreeBSD, and "darwin" for \macos and iOS. For information on the type
    of product the application is running on, see productType().

    \sa QFileSelector, kernelVersion(), productType(), productVersion(), prettyProductName()
*/
QString QSysInfo::kernelType()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("winnt");
#elif defined(Q_OS_UNIX)
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.sysname).toLower();
#endif
    return unknownText();
}

/*!
    \since 5.4

    Returns the release version of the operating system kernel. On Windows, it
    returns the version of the NT kernel. On Unix systems, including
    Android and \macos, it returns the same as the \c{uname -r}
    command would return. On VxWorks, it returns the numeric part of the string
    reported by kernelVersion().

    If the version could not be determined, this function may return an empty
    string.

    \sa kernelType(), productType(), productVersion(), prettyProductName()
*/
QString QSysInfo::kernelVersion()
{
#ifdef Q_OS_WIN
    const auto osver = QOperatingSystemVersion::current();
    return QString::asprintf("%d.%d.%d",
                             osver.majorVersion(), osver.minorVersion(), osver.microVersion());
#else
    struct utsname u;
    if (uname(&u) == 0) {
#   ifdef Q_OS_VXWORKS
        // The string follows the pattern "Core Kernel version: w.x.y.z"
        auto versionStr = QByteArrayView(u.kernelversion);
        if (auto lastSpace = versionStr.lastIndexOf(' '); lastSpace != -1) {
            return QString::fromLatin1(versionStr.sliced(lastSpace + 1));
        }
#   else
        return QString::fromLatin1(u.release);
#   endif
    }
    return QString();
#endif
}


/*!
    \since 5.4

    Returns the product name of the operating system this application is
    running in. If the application is running on some sort of emulation or
    virtualization layer (such as WINE on a Unix system), this function will
    inspect the emulation / virtualization layer.

    Values returned by this function are stable and will not change over time,
    so applications can rely on the returned value as an identifier, except
    that new OS types may be added over time.

    \b{Linux and Android note}: this function returns "android" for Linux
    systems running Android userspace, notably when using the Bionic library.
    For all other Linux systems, regardless of C library being used, it tries
    to determine the distribution name and returns that. If determining the
    distribution name failed, it returns "unknown".

    \b{\macos note}: this function returns "macos" for all \macos systems,
    regardless of Apple naming convention. Previously, in Qt 5, it returned
    "osx", again regardless of Apple naming conventions.

    \b{Darwin, iOS, tvOS, and watchOS note}: this function returns "ios" for
    iOS systems, "tvos" for tvOS systems, "watchos" for watchOS systems, and
    "darwin" in case the system could not be determined.

    \b{FreeBSD note}: this function returns "debian" for Debian/kFreeBSD and
    "unknown" otherwise.

    \b{Windows note}: this function return "windows"

    \b{VxWorks note}: this function return "vxworks"

    For other Unix-type systems, this function usually returns "unknown".

    \sa QFileSelector, kernelType(), kernelVersion(), productVersion(), prettyProductName()
*/
QString QSysInfo::productType()
{
    // similar, but not identical to QFileSelectorPrivate::platformSelectors
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");

#elif defined(Q_OS_QNX)
    return QStringLiteral("qnx");

#elif defined(Q_OS_ANDROID)
    return QStringLiteral("android");

#elif defined(Q_OS_IOS)
    return QStringLiteral("ios");
#elif defined(Q_OS_TVOS)
    return QStringLiteral("tvos");
#elif defined(Q_OS_WATCHOS)
    return QStringLiteral("watchos");
#elif defined(Q_OS_VISIONOS)
    return QStringLiteral("visionos");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_DARWIN)
    return QStringLiteral("darwin");
#elif defined(Q_OS_WASM)
    return QStringLiteral("wasm");
#elif defined(Q_OS_VXWORKS)
    return QStringLiteral("vxworks");

#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.productType.isEmpty())
        return unixOsVersion.productType;
#endif
    return unknownText();
}

/*!
    \since 5.4

    Returns the product version of the operating system in string form. If the
    version could not be determined, this function returns "unknown".

    It will return the Android, iOS, \macos, VxWorks, Windows full-product
    versions on those systems.

    Typical returned values are (note: list not exhaustive):
    \list
        \li "12" (Android 12)
        \li "36" (Fedora 36)
        \li "15.5" (iOS 15.5)
        \li "12.4" (macOS Monterey)
        \li "22.04" (Ubuntu 22.04)
        \li "8.6" (watchOS 8.6)
        \li "11" (Windows 11)
        \li "Server 2022" (Windows Server 2022)
        \li "24.03" (VxWorks 7 - 24.03)
    \endlist

    On Linux systems, it will try to determine the distribution version and will
    return that. This is also done on Debian/kFreeBSD, so this function will
    return Debian version in that case.

    In all other Unix-type systems, this function always returns "unknown".

    \note The version string returned from this function is not guaranteed to
    be orderable. On Linux, the version of
    the distribution may jump unexpectedly, please refer to the distribution's
    documentation for versioning practices.

    \sa kernelType(), kernelVersion(), productType(), prettyProductName()
*/
QString QSysInfo::productVersion()
{
#if defined(Q_OS_ANDROID)
    const auto version = QOperatingSystemVersion::current();
    return QString::asprintf("%d.%d", version.majorVersion(), version.minorVersion());
#elif defined(Q_OS_DARWIN)
    const auto version = QOperatingSystemVersion::current();
    return QString::asprintf("%d.%d.%d", version.majorVersion(),
                                         version.minorVersion(),
                                         version.microVersion());
#elif defined(Q_OS_WIN)
    const char *version = osVer_helper();
    if (version) {
        const QLatin1Char spaceChar(' ');
        return QString::fromLatin1(version).remove(spaceChar).toLower() + winSp_helper().remove(spaceChar).toLower();
    }
    // fall through

#elif defined(Q_OS_VXWORKS)
    utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.releaseversion);
    // fall through

#elif defined(USE_ETC_OS_RELEASE) // Q_OS_UNIX
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.productVersion.isEmpty())
        return unixOsVersion.productVersion;
#endif

    // fallback
    return unknownText();
}

/*!
    \since 5.4

    Returns a prettier form of productType() and productVersion(), containing
    other tokens like the operating system type, codenames and other
    information. The result of this function is suitable for displaying to the
    user, but not for long-term storage, as the string may change with updates
    to Qt.

    If productType() is "unknown", this function will instead use the
    kernelType() and kernelVersion() functions.

    \sa kernelType(), kernelVersion(), productType(), productVersion()
*/
QString QSysInfo::prettyProductName()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_DARWIN) || defined(Q_OS_WIN)
    const auto version = QOperatingSystemVersion::current();
    QString versionString;
#  if defined(Q_OS_DARWIN)
    if (const int microVersion = version.microVersion(); microVersion > 0)
        versionString = QString::asprintf("%d.%d.%d", version.majorVersion(),
                                                      version.minorVersion(),
                                                      microVersion);
    else
#  endif // Darwin
        versionString = QString::asprintf("%d.%d", version.majorVersion(),
                                                   version.minorVersion());
    QString result = version.name() + u' ';
    const char *name = osVer_helper(version);
    if (!name)
        return result + versionString;
    result += QLatin1StringView(name);
#  if !defined(Q_OS_WIN)
    return result + " ("_L1 + versionString + u')';
#  else
    // (resembling winver.exe): Windows 10 "Windows 10 Version 1809"
    const auto displayVersion = windowsDisplayVersion();
    if (!displayVersion.isEmpty())
        result += " Version "_L1 + displayVersion;
    return result;
#  endif // Windows
#elif defined(Q_OS_HAIKU)
    return "Haiku "_L1 + productVersion();
#elif defined(Q_OS_UNIX)
#  ifdef USE_ETC_OS_RELEASE
    QUnixOSVersion unixOsVersion;
    findUnixOsVersion(unixOsVersion);
    if (!unixOsVersion.prettyName.isEmpty())
        return unixOsVersion.prettyName;
#  endif
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLatin1(u.sysname) + u' ' + QString::fromLatin1(u.release);
#endif
    return unknownText();
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.6

    Returns this machine's host name, if one is configured. Note that hostnames
    are not guaranteed to be globally unique, especially if they were
    configured automatically.

    This function does not guarantee the returned host name is a Fully
    Qualified Domain Name (FQDN). For that, use QHostInfo to resolve the
    returned name to an FQDN.

    This function returns the same as QHostInfo::localHostName().

    \sa QHostInfo::localDomainName, machineUniqueId()
*/
QString QSysInfo::machineHostName()
{
    // the hostname can change, so we can't cache it
#if defined(Q_OS_LINUX)
    // gethostname(3) on Linux just calls uname(2), so do it ourselves
    // and avoid a memcpy
    struct utsname u;
    if (uname(&u) == 0)
        return QString::fromLocal8Bit(u.nodename);
    return QString();
#else
#  ifdef Q_OS_WIN
    // Important: QtNetwork depends on machineHostName() initializing ws2_32.dll
    winsockInit();
    QString hostName;
    hostName.resize(512);
    unsigned long len = hostName.size();
    BOOL res = GetComputerNameEx(ComputerNameDnsHostname,
                                 reinterpret_cast<wchar_t *>(hostName.data()), &len);
    if (!res && len > 512) {
        hostName.resize(len - 1);
        GetComputerNameEx(ComputerNameDnsHostname, reinterpret_cast<wchar_t *>(hostName.data()),
                          &len);
    }
    hostName.truncate(len);
    return hostName;
#  else // !Q_OS_WIN

    char hostName[512];
    if (gethostname(hostName, sizeof(hostName)) == -1)
        return QString();
    hostName[sizeof(hostName) - 1] = '\0';
    return QString::fromLocal8Bit(hostName);
#  endif
#endif
}
#endif // QT_BOOTSTRAPPED

enum {
    UuidStringLen = sizeof("00000000-0000-0000-0000-000000000000") - 1
};

/*!
    \since 5.11

    Returns a unique ID for this machine, if one can be determined. If no
    unique ID could be determined, this function returns an empty byte array.
    Unlike machineHostName(), the value returned by this function is likely
    globally unique.

    A unique ID is useful in network operations to identify this machine for an
    extended period of time, when the IP address could change or if this
    machine could have more than one IP address. For example, the ID could be
    used when communicating with a server or when storing device-specific data
    in shared network storage.

    Note that on some systems, this value will persist across reboots and on
    some it will not. Applications should not blindly depend on this fact
    without verifying the OS capabilities. In particular, on Linux systems,
    this ID is usually permanent and it matches the D-Bus machine ID, except
    for nodes without their own storage (replicated nodes).

    \sa machineHostName(), bootUniqueId()
*/
QByteArray QSysInfo::machineUniqueId()
{
#if defined(Q_OS_DARWIN) && __has_include(<IOKit/IOKitLib.h>)
    char uuid[UuidStringLen + 1];
    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    QCFString stringRef = (CFStringRef)IORegistryEntryCreateCFProperty(service, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    CFStringGetCString(stringRef, uuid, sizeof(uuid), kCFStringEncodingMacRoman);
    return QByteArray(uuid);
#elif defined(Q_OS_BSD4) && defined(KERN_HOSTUUID)
    char uuid[UuidStringLen + 1];
    size_t uuidlen = sizeof(uuid);
    int name[] = { CTL_KERN, KERN_HOSTUUID };
    if (sysctl(name, sizeof name / sizeof name[0], &uuid, &uuidlen, nullptr, 0) == 0
            && uuidlen == sizeof(uuid))
        return QByteArray(uuid, uuidlen - 1);
#elif defined(Q_OS_UNIX)
    // The modern name on Linux is /etc/machine-id, but that path is
    // unlikely to exist on non-Linux (non-systemd) systems. The old
    // path is more than enough.
    static const char fullfilename[] = "/usr/local/var/lib/dbus/machine-id";
    const char *firstfilename = fullfilename + sizeof("/usr/local") - 1;
    int fd = qt_safe_open(firstfilename, O_RDONLY);
    if (fd == -1 && errno == ENOENT)
        fd = qt_safe_open(fullfilename, O_RDONLY);

    if (fd != -1) {
        char buffer[32];    // 128 bits, hex-encoded
        qint64 len = qt_safe_read(fd, buffer, sizeof(buffer));
        qt_safe_close(fd);

        if (len != -1)
            return QByteArray(buffer, len);
    }
#elif defined(Q_OS_WIN)
    // Let's poke at the registry
    const QString machineGuid = QWinRegistryKey(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Cryptography)")
                                .stringValue(u"MachineGuid"_s);
    if (!machineGuid.isEmpty())
        return machineGuid.toLatin1();
#endif
    return QByteArray();
}

/*!
    \since 5.11

    Returns a unique ID for this machine's boot, if one can be determined. If
    no unique ID could be determined, this function returns an empty byte
    array. This value is expected to change after every boot and can be
    considered globally unique.

    This function is currently only implemented for Linux and Apple operating
    systems.

    \sa machineUniqueId()
*/
QByteArray QSysInfo::bootUniqueId()
{
#ifdef Q_OS_LINUX
    // use low-level API here for simplicity
    int fd = qt_safe_open("/proc/sys/kernel/random/boot_id", O_RDONLY);
    if (fd != -1) {
        char uuid[UuidStringLen];
        qint64 len = qt_safe_read(fd, uuid, sizeof(uuid));
        qt_safe_close(fd);
        if (len == UuidStringLen)
            return QByteArray(uuid, UuidStringLen);
    }
#elif defined(Q_OS_DARWIN)
    // "kern.bootsessionuuid" is only available by name
    char uuid[UuidStringLen + 1];
    size_t uuidlen = sizeof(uuid);
    if (sysctlbyname("kern.bootsessionuuid", uuid, &uuidlen, nullptr, 0) == 0
            && uuidlen == sizeof(uuid))
        return QByteArray(uuid, uuidlen - 1);
#endif
    return QByteArray();
};

QT_END_NAMESPACE
