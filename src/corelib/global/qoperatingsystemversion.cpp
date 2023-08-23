// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qoperatingsystemversion.h"

#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN)
#include "qoperatingsystemversion_p.h"
#endif

#if defined(Q_OS_DARWIN)
#include <QtCore/private/qcore_mac_p.h>
#endif

#include <qversionnumber.h>
#include <qdebug.h>

#ifdef Q_OS_ANDROID
#include <QtCore/private/qjnihelpers_p.h>
#include <QJniObject>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QOperatingSystemVersion
    \inmodule QtCore
    \since 5.9
    \brief The QOperatingSystemVersion class provides information about the
    operating system version.

    Unlike other version functions in QSysInfo, QOperatingSystemVersion provides
    access to the full version number that \a developers typically use to vary
    behavior or determine whether to enable APIs or features based on the
    operating system version (as opposed to the kernel version number or
    marketing version).

    Presently, Android, Apple Platforms (iOS, macOS, tvOS, and watchOS),
    and Windows are supported.

    The \a majorVersion(), \a minorVersion(), and \a microVersion() functions
    return the parts of the operating system version number based on:

    \table
        \header
            \li Platforms
            \li Value
        \row
            \li Android
            \li result of parsing
                \l{https://developer.android.com/reference/android/os/Build.VERSION.html#RELEASE}{android.os.Build.VERSION.RELEASE}
                using QVersionNumber, with a fallback to
                \l{https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT}{android.os.Build.VERSION.SDK_INT}
                to determine the major and minor version component if the former
                fails
        \row
            \li Apple Platforms
            \li majorVersion, minorVersion, and patchVersion from
                \l{https://developer.apple.com/reference/foundation/nsprocessinfo/1410906-operatingsystemversion?language=objc}{NSProcessInfo.operatingSystemVersion}
        \row
            \li Windows
            \li dwMajorVersion, dwMinorVersion, and dwBuildNumber from
                \l{https://docs.microsoft.com/en-us/windows/win32/devnotes/rtlgetversion}
                {RtlGetVersion} -
                note that this function ALWAYS return the version number of the
                underlying operating system, as opposed to the shim underneath
                GetVersionEx that hides the real version number if the
                application is not manifested for that version of the OS
    \endtable

    Because QOperatingSystemVersion stores both a version number and an OS type, the OS type
    can be taken into account when performing comparisons. For example, on a macOS system running
    macOS Sierra (v10.12), the following expression will return \c false even though the
    major version number component of the object on the left hand side of the expression (10) is
    greater than that of the object on the right (9):

    \snippet code/src_corelib_global_qoperatingsystemversion.cpp 0

    This allows expressions for multiple operating systems to be joined with a logical OR operator
    and still work as expected. For example:

    \snippet code/src_corelib_global_qoperatingsystemversion.cpp 1

    A more naive comparison algorithm might incorrectly return true on all versions of macOS,
    including Mac OS 9. This behavior is achieved by overloading the comparison operators to return
    \c false whenever the OS types of the QOperatingSystemVersion instances being compared do not
    match. Be aware that due to this it can be the case \c x >= y and \c x < y are BOTH \c false
    for the same instances of \c x and \c y.
*/

/*!
    \enum QOperatingSystemVersion::OSType

    This enum provides symbolic names for the various operating
    system families supported by QOperatingSystemVersion.

    \value Android      The Google Android operating system.
    \value IOS          The Apple iOS operating system.
    \value MacOS        The Apple macOS operating system.
    \value TvOS         The Apple tvOS operating system.
    \value WatchOS      The Apple watchOS operating system.
    \value Windows      The Microsoft Windows operating system.

    \value Unknown      An unknown or unsupported operating system.
*/

/*!
    \fn QOperatingSystemVersion::QOperatingSystemVersion(OSType osType, int vmajor, int vminor = -1, int vmicro = -1)

    Constructs a QOperatingSystemVersion consisting of the OS type \a osType, and
    major, minor, and micro version numbers \a vmajor, \a vminor and \a vmicro, respectively.
*/

/*!
    Returns a QOperatingSystemVersion indicating the current OS and its version number.

    \sa currentType()
*/
QOperatingSystemVersion QOperatingSystemVersion::current()
{
    return QOperatingSystemVersionBase::current();
}

QOperatingSystemVersionBase QOperatingSystemVersionBase::current()
{
    static const QOperatingSystemVersionBase v = current_impl();
    return v;
}

#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN)
QOperatingSystemVersionBase QOperatingSystemVersionBase::current_impl()
{
    QOperatingSystemVersionBase version;
    version.m_os = currentType();
#ifdef Q_OS_ANDROID
#ifndef QT_BOOTSTRAPPED
    const QVersionNumber v = QVersionNumber::fromString(QJniObject::getStaticObjectField(
        "android/os/Build$VERSION", "RELEASE", "Ljava/lang/String;").toString());
    if (!v.isNull()) {
        version.m_major = v.majorVersion();
        version.m_minor = v.minorVersion();
        version.m_micro = v.microVersion();
        return version;
    }
#endif

    version.m_major = -1;
    version.m_minor = -1;

    static const struct {
        uint major : 4;
        uint minor : 4;
    } versions[] = {
        { 1, 0 }, // API level 1
        { 1, 1 }, // API level 2
        { 1, 5 }, // API level 3
        { 1, 6 }, // API level 4
        { 2, 0 }, // API level 5
        { 2, 0 }, // API level 6
        { 2, 1 }, // API level 7
        { 2, 2 }, // API level 8
        { 2, 3 }, // API level 9
        { 2, 3 }, // API level 10
        { 3, 0 }, // API level 11
        { 3, 1 }, // API level 12
        { 3, 2 }, // API level 13
        { 4, 0 }, // API level 14
        { 4, 0 }, // API level 15
        { 4, 1 }, // API level 16
        { 4, 2 }, // API level 17
        { 4, 3 }, // API level 18
        { 4, 4 }, // API level 19
        { 4, 4 }, // API level 20
        { 5, 0 }, // API level 21
        { 5, 1 }, // API level 22
        { 6, 0 }, // API level 23
        { 7, 0 }, // API level 24
        { 7, 1 }, // API level 25
        { 8, 0 }, // API level 26
        { 8, 1 }, // API level 27
        { 9, 0 }, // API level 28
        { 10, 0 }, // API level 29
        { 11, 0 }, // API level 30
        { 12, 0 }, // API level 31
        { 12, 0 }, // API level 32
        { 13, 0 }, // API level 33
    };

    // This will give us at least the first 2 version components
    const size_t versionIdx = QtAndroidPrivate::androidSdkVersion() - 1;
    if (versionIdx < sizeof(versions) / sizeof(versions[0])) {
        version.m_major = versions[versionIdx].major;
        version.m_minor = versions[versionIdx].minor;
    }

    // API level 6 was exactly version 2.0.1
    version.m_micro = versionIdx == 5 ? 1 : -1;
#else
    version.m_major = -1;
    version.m_minor = -1;
    version.m_micro = -1;
#endif
    return version;
}
#endif

static inline int compareVersionComponents(int lhs, int rhs)
{
    return lhs >= 0 && rhs >= 0 ? lhs - rhs : 0;
}

int QOperatingSystemVersionBase::compare(QOperatingSystemVersionBase v1,
                                         QOperatingSystemVersionBase v2)
{
    if (v1.m_major == v2.m_major) {
        if (v1.m_minor == v2.m_minor) {
            return compareVersionComponents(v1.m_micro, v2.m_micro);
        }
        return compareVersionComponents(v1.m_minor, v2.m_minor);
    }
    return compareVersionComponents(v1.m_major, v2.m_major);
}

/*!
    \fn QVersionNumber QOperatingSystemVersion::version() const

    \since 6.1

    Returns the operating system's version number.

    See the main class documentation for what the version number is on a given
    operating system.

    \sa majorVersion(), minorVersion(), microVersion()
*/

/*!
    \fn int QOperatingSystemVersion::majorVersion() const

    Returns the major version number, that is, the first segment of the
    operating system's version number.

    See the main class documentation for what the major version number is on a given
    operating system.

    -1 indicates an unknown or absent version number component.

    \sa version(), minorVersion(), microVersion()
*/

/*!
    \fn int QOperatingSystemVersion::minorVersion() const

    Returns the minor version number, that is, the second segment of the
    operating system's version number.

    See the main class documentation for what the minor version number is on a given
    operating system.

    -1 indicates an unknown or absent version number component.

    \sa version(), majorVersion(), microVersion()
*/

/*!
    \fn int QOperatingSystemVersion::microVersion() const

    Returns the micro version number, that is, the third segment of the
    operating system's version number.

    See the main class documentation for what the micro version number is on a given
    operating system.

    -1 indicates an unknown or absent version number component.

    \sa version(), majorVersion(), minorVersion()
*/

/*!
    \fn int QOperatingSystemVersion::segmentCount() const

    Returns the number of integers stored in the version number.
*/

/*!
    \fn QOperatingSystemVersion::OSType QOperatingSystemVersion::type() const

    Returns the OS type identified by the QOperatingSystemVersion.

    \sa name()
*/

/*!
    \fn QOperatingSystemVersion::OSType QOperatingSystemVersion::currentType()

    Returns the current OS type without constructing a QOperatingSystemVersion instance.

    \since 5.10

    \sa current()
*/

/*!
    \fn QString QOperatingSystemVersion::name() const

    Returns a string representation of the OS type identified by the QOperatingSystemVersion.

    \sa type()
*/
QString QOperatingSystemVersion::name() const
{
    return QOperatingSystemVersionBase::name();
}

QString QOperatingSystemVersionBase::name(QOperatingSystemVersionBase osversion)
{
    switch (osversion.type()) {
    case QOperatingSystemVersionBase::Windows:
        return QStringLiteral("Windows");
    case QOperatingSystemVersionBase::MacOS: {
        if (osversion.majorVersion() < 10)
            return QStringLiteral("Mac OS");
        if (osversion.majorVersion() == 10 && osversion.minorVersion() < 8)
            return QStringLiteral("Mac OS X");
        if (osversion.majorVersion() == 10 && osversion.minorVersion() < 12)
            return QStringLiteral("OS X");
        return QStringLiteral("macOS");
    }
    case QOperatingSystemVersionBase::IOS: {
        if (osversion.majorVersion() < 4)
            return QStringLiteral("iPhone OS");
        return QStringLiteral("iOS");
    }
    case QOperatingSystemVersionBase::TvOS:
        return QStringLiteral("tvOS");
    case QOperatingSystemVersionBase::WatchOS:
        return QStringLiteral("watchOS");
    case QOperatingSystemVersionBase::Android:
        return QStringLiteral("Android");
    case QOperatingSystemVersionBase::Unknown:
    default:
        return QString();
    }
}

/*!
    \fn bool QOperatingSystemVersion::isAnyOfType(std::initializer_list<OSType> types) const

    Returns whether the OS type identified by the QOperatingSystemVersion
    matches any of the OS types in \a types.
*/
bool QOperatingSystemVersion::isAnyOfType(std::initializer_list<OSType> types) const
{
    // ### Qt7: Remove this function
    return std::find(types.begin(), types.end(), type()) != types.end();
}

bool QOperatingSystemVersionBase::isAnyOfType(std::initializer_list<OSType> types, OSType type)
{
    return std::find(types.begin(), types.end(), type) != types.end();
}

#ifndef QT_BOOTSTRAPPED

/*!
    \variable QOperatingSystemVersion::Windows7
    \brief a version corresponding to Windows 7 (version 6.1).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows7 =
    QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 1);

/*!
    \variable QOperatingSystemVersion::Windows8
    \brief a version corresponding to Windows 8 (version 6.2).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows8 =
    QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 2);

/*!
    \variable QOperatingSystemVersion::Windows8_1
    \brief a version corresponding to Windows 8.1 (version 6.3).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows8_1 =
    QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 3);

/*!
    \variable QOperatingSystemVersion::Windows10
    \brief a version corresponding to general Windows 10 (version 10.0).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows10 =
    QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10);

/*!
    \variable QOperatingSystemVersion::Windows10_1809
    \brief a version corresponding to Windows 10 October 2018 Update
           Version 1809 (version 10.0.17763).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_1809;

/*!
    \variable QOperatingSystemVersion::Windows10_1903
    \brief a version corresponding to Windows 10 May 2019 Update
           Version 1903 (version 10.0.18362).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_1903;

/*!
    \variable QOperatingSystemVersion::Windows10_1909
    \brief a version corresponding to Windows 10 November 2019 Update
           Version 1909 (version 10.0.18363).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_1909;

/*!
    \variable QOperatingSystemVersion::Windows10_2004
    \brief a version corresponding to Windows 10 May 2020 Update
           Version 2004 (version 10.0.19041).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_2004;

/*!
    \variable QOperatingSystemVersion::Windows10_20H2
    \brief a version corresponding to Windows 10 October 2020 Update
           Version 20H2 (version 10.0.19042).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_20H2;

/*!
    \variable QOperatingSystemVersion::Windows10_21H1
    \brief a version corresponding to Windows 10 May 2021 Update
           Version 21H1 (version 10.0.19043).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_21H1;

/*!
    \variable QOperatingSystemVersion::Windows10_21H2
    \brief a version corresponding to Windows 10 November 2021 Update
           Version 21H2 (version 10.0.19044).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_21H2;

/*!
    \variable QOperatingSystemVersion::Windows10_22H2
    \brief a version corresponding to Windows 10 October 2022 Update
           Version 22H2 (version 10.0.19045).
    \since 6.5
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows10_22H2;

/*!
    \variable QOperatingSystemVersion::Windows11
    \brief a version corresponding to the initial release of Windows 11
           (version 10.0.22000).
    \since 6.3
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows11;

/*!
    \variable QOperatingSystemVersion::Windows11_21H2
    \brief a version corresponding to Windows 11 Version 21H2 (version 10.0.22000).
    \since 6.4
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows11_21H2;

/*!
    \variable QOperatingSystemVersion::Windows11_22H2
    \brief a version corresponding to Windows 11 Version 22H2 (version 10.0.22621).
    \since 6.4
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Windows11_22H2;

/*!
    \variable QOperatingSystemVersion::OSXMavericks
    \brief a version corresponding to OS X Mavericks (version 10.9).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXMavericks =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 9);

/*!
    \variable QOperatingSystemVersion::OSXYosemite
    \brief a version corresponding to OS X Yosemite (version 10.10).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXYosemite =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 10);

/*!
    \variable QOperatingSystemVersion::OSXElCapitan
    \brief a version corresponding to OS X El Capitan (version 10.11).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXElCapitan =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 11);

/*!
    \variable QOperatingSystemVersion::MacOSSierra
    \brief a version corresponding to macOS Sierra (version 10.12).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSSierra =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 12);

/*!
    \variable QOperatingSystemVersion::MacOSHighSierra
    \brief a version corresponding to macOS High Sierra (version 10.13).
    \since 5.9.1
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSHighSierra =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 13);

/*!
    \variable QOperatingSystemVersion::MacOSMojave
    \brief a version corresponding to macOS Mojave (version 10.14).
    \since 5.11.2
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSMojave =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 14);

/*!
    \variable QOperatingSystemVersion::MacOSCatalina
    \brief a version corresponding to macOS Catalina (version 10.15).
    \since 5.12.5
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSCatalina =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 15);

/*!
    \variable QOperatingSystemVersion::MacOSBigSur
    \brief a version corresponding to macOS Big Sur (version 11).
    \since 6.0
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSBigSur =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 11, 0);

/*!
    \variable QOperatingSystemVersion::MacOSMonterey
    \brief a version corresponding to macOS Monterey (version 12).
    \since 6.3
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSMonterey =
    QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 12, 0);

/*!
    \variable QOperatingSystemVersion::MacOSVentura
    \brief a version corresponding to macOS Ventura (version 13).
    \since 6.4
*/
const QOperatingSystemVersionBase QOperatingSystemVersion::MacOSVentura;

/*!
    \variable QOperatingSystemVersion::MacOSSonoma
    \brief a version corresponding to macOS Sonoma (version 14).
    \since 6.5
*/

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean
    \brief a version corresponding to Android Jelly Bean (version 4.1, API level 16).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 1);

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean_MR1
    \brief a version corresponding to Android Jelly Bean, maintenance release 1
    (version 4.2, API level 17).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean_MR1 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 2);

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean_MR2
    \brief a version corresponding to Android Jelly Bean, maintenance release 2
    (version 4.3, API level 18).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean_MR2 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 3);

/*!
    \variable QOperatingSystemVersion::AndroidKitKat
    \brief a version corresponding to Android KitKat (versions 4.4 & 4.4W, API levels 19 & 20).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidKitKat =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 4);

/*!
    \variable QOperatingSystemVersion::AndroidLollipop
    \brief a version corresponding to Android Lollipop (version 5.0, API level 21).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidLollipop =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 5, 0);

/*!
    \variable QOperatingSystemVersion::AndroidLollipop_MR1
    \brief a version corresponding to Android Lollipop, maintenance release 1
    (version 5.1, API level 22).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidLollipop_MR1 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 5, 1);

/*!
    \variable QOperatingSystemVersion::AndroidMarshmallow
    \brief a version corresponding to Android Marshmallow (version 6.0, API level 23).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidMarshmallow =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 6, 0);

/*!
    \variable QOperatingSystemVersion::AndroidNougat
    \brief a version corresponding to Android Nougat (version 7.0, API level 24).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidNougat =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 7, 0);

/*!
    \variable QOperatingSystemVersion::AndroidNougat_MR1
    \brief a version corresponding to Android Nougat, maintenance release 1
    (version 7.0, API level 25).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidNougat_MR1 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 7, 1);

/*!
    \variable QOperatingSystemVersion::AndroidOreo
    \brief a version corresponding to Android Oreo (version 8.0, API level 26).
    \since 5.9.2
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidOreo =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 8, 0);

/*!
    \variable QOperatingSystemVersion::AndroidOreo_MR1
    \brief a version corresponding to Android Oreo_MR1 (version 8.1, API level 27).
    \since 6.1
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidOreo_MR1 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 8, 1);

/*!
    \variable QOperatingSystemVersion::AndroidPie
    \brief a version corresponding to Android Pie (version 9.0, API level 28).
    \since 6.1
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidPie =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 9, 0);

/*!
    \variable QOperatingSystemVersion::Android10
    \brief a version corresponding to Android 10 (version 10.0, API level 29).
    \since 6.1
 */
const QOperatingSystemVersion QOperatingSystemVersion::Android10 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 10, 0);

/*!
    \variable QOperatingSystemVersion::Android11
    \brief a version corresponding to Android 11 (version 11.0, API level 30).
    \since 6.1
 */
const QOperatingSystemVersion QOperatingSystemVersion::Android11 =
    QOperatingSystemVersion(QOperatingSystemVersion::Android, 11, 0);

/*!
    \variable QOperatingSystemVersion::Android12
    \brief a version corresponding to Android 12 (version 12.0, API level 31).
    \since 6.5
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Android12;

/*!
    \variable QOperatingSystemVersion::Android12L
    \brief a version corresponding to Android 12L (version 12.0, API level 32).
    \since 6.5
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Android12L;

/*!
    \variable QOperatingSystemVersion::Android13
    \brief a version corresponding to Android 13 (version 13.0, API level 33).
    \since 6.5
 */
const QOperatingSystemVersionBase QOperatingSystemVersion::Android13;

#endif // !QT_BOOTSTRAPPED

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QOperatingSystemVersion &ov)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QOperatingSystemVersion(" << ov.name()
        << ", " << ov.majorVersion() << '.' << ov.minorVersion()
        << '.' << ov.microVersion() << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
