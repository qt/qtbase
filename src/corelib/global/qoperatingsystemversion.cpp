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

#include "qoperatingsystemversion.h"
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN)
#include "qoperatingsystemversion_p.h"
#endif

#include <qversionnumber.h>

#if defined(Q_OS_ANDROID)
#include <private/qjni_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QOperatingSystemVersion
    \inmodule QtCore
    \since 5.9
    \brief The QOperatingSystemVersion class provides information about the operating system version.

    Unlike other version functions in QSysInfo, QOperatingSystemVersion provides access to the full
    version number that \a developers typically use to vary behavior or determine whether to enable
    APIs or features based on the operating system version (as opposed to the kernel version number
    or marketing version).

    This class is also a complete replacement for QSysInfo::macVersion and QSysInfo::windowsVersion,
    additionally providing access to the third (micro) version number component.

    Presently, Android, Apple Platforms (iOS, macOS, tvOS, and watchOS), and Windows are supported.

    The \a majorVersion(), \a minorVersion(), and \a microVersion() functions return the parts of
    the operating system version number based on:

    \table
    \header \li Platforms       \li Value
    \row    \li Android         \li result of parsing
                                    \l{https://developer.android.com/reference/android/os/Build.VERSION.html#RELEASE}{"android.os.Build.VERSION.RELEASE"}
                                    using QVersionNumber, with a fallback to
                                    \l{https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT}{"android.os.Build.VERSION.SDK_INT"}
                                    to determine the major and minor version component if the former fails
    \row    \li Apple Platforms \li majorVersion, minorVersion, and patchVersion from
                                    \l{https://developer.apple.com/reference/foundation/nsprocessinfo/1410906-operatingsystemversion?language=objc}{"NSProcessInfo.operatingSystemVersion"}
    \row    \li Windows         \li dwMajorVersion, dwMinorVersion, and dwBuildNumber from
                                    \l{https://msdn.microsoft.com/en-us/library/mt723418.aspx}{"RtlGetVersion"} -
                                    note that this function ALWAYS return the version number of the underlying operating system,
                                    as opposed to the shim underneath GetVersionEx that hides the real version number
                                    if the application is not manifested for that version of the OS
    \endtable
*/

/*!
    \fn QOperatingSystemVersion::QOperatingSystemVersion(int maj, int min, int mic)

    Constructs a QOperatingSystemVersion consisting of the OS type \a os, and
    major, minor, and micro version numbers \a maj, \a min and \a mic, respectively.
*/

/*!
    \fn QOperatingSystemVersion QOperatingSystemVersion::current()

    Returns a QOperatingSystemVersion indicating the current OS and its version number.
*/
#if !defined(Q_OS_DARWIN) && !defined(Q_OS_WIN)
QOperatingSystemVersion QOperatingSystemVersion::current()
{
    QOperatingSystemVersion version;
    version.m_os = currentType();
#if defined(Q_OS_ANDROID)
#ifndef QT_BOOTSTRAPPED
    const QVersionNumber v = QVersionNumber::fromString(QJNIObjectPrivate::getStaticObjectField(
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

    static const int versions[][2] = {
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
    };

    // This will give us at least the first 2 version components
    const size_t versionIdx = size_t(QJNIObjectPrivate::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT")) - 1;
    if (versionIdx < sizeof(versions) / sizeof(versions[0])) {
        version.m_major = versions[versionIdx][0];
        version.m_minor = versions[versionIdx][1];
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

/*!
    \fn int QOperatingSystemVersion::compare(const QOperatingSystemVersion &v1,
                                             const QOperatingSystemVersion &v2)

    Compares \a v1 with \a v2 and returns an integer less than, equal to, or
    greater than zero, depending on whether \a v1 is less than, equal to, or
    greater than \a v2, respectively.

    Comparisons are performed by comparing the version number components of
    \a v1 and \a v2.

    \note This function cannot take the OS type into account; you should use
    the overloaded comparison operators to compare QOperatingSystemVersions
    in a safe manner.
*/
int QOperatingSystemVersion::compare(const QOperatingSystemVersion &v1, const QOperatingSystemVersion &v2)
{
    if (v1.m_major == v2.m_major) {
        if (v1.m_minor == v2.m_minor) {
            return compareVersionComponents(v1.m_micro, v2.m_micro);
        }
        return compareVersionComponents(v1.m_minor, v2.m_minor);
    }
    return compareVersionComponents(v1.m_major, v2.m_major);
}

#ifndef QT_BOOTSTRAPPED
/*!
    \fn QOperatingSystemVersion QOperatingSystemVersion::fromVersionNumber(const QVersionNumber &version,
                                                                           QOperatingSystemVersion::OSType os)

    Returns a QOperatingSystemVersion consisting of the OS type \a os and version number \a version.
*/
QOperatingSystemVersion QOperatingSystemVersion::fromVersionNumber(const QVersionNumber &version,
                                                                   QOperatingSystemVersion::OSType os)
{
    return QOperatingSystemVersion(os, version.majorVersion(), version.minorVersion(), version.microVersion());
}

/*!
    \fn QOperatingSystemVersion QOperatingSystemVersion::toVersionNumber() const

    Returns the QOperatingSystemVersion's version number as a QVersionNumber.
*/
QVersionNumber QOperatingSystemVersion::toVersionNumber() const
{
    return QVersionNumber(m_major, m_minor, m_micro);
}
#endif

/*!
    \fn int QOperatingSystemVersion::majorVersion() const

    Returns the major version number, that is, the first segment of the operating system's version number.

    See the main class documentation for what the major version number is on a given operating system.

    -1 indicates an unknown or absent version number component.

    \sa minorVersion(), microVersion()
*/

/*!
    \fn int QOperatingSystemVersion::minorVersion() const

    Returns the minor version number, that is, the second segment of the operating system's version number.

    See the main class documentation for what the minor version number is on a given operating system.

    -1 indicates an unknown or absent version number component.

    \sa majorVersion(), macro()
*/

/*!
    \fn int QOperatingSystemVersion::microVersion() const

    Returns the micro version number, that is, the third segment of the operating system's version number.

    See the main class documentation for what the micro version number is on a given operating system.

    -1 indicates an unknown or absent version number component.

    \sa majorVersion(), minorVersion()
*/

/*!
    \fn QOperatingSystemVersion::OSType QOperatingSystemVersion::type() const

    Returns the OS type identified by the QOperatingSystemVersion.

    \sa typeName()
*/

/*!
    \fn QString QOperatingSystemVersion::name() const

    Returns a string representation of the OS type identified by the QOperatingSystemVersion.

    \sa type()
*/
QString QOperatingSystemVersion::name() const
{
    switch (type()) {
    case QOperatingSystemVersion::Windows:
        return QStringLiteral("Windows");
    case QOperatingSystemVersion::MacOS: {
        if (majorVersion() < 10)
            return QStringLiteral("Mac OS");
        if (majorVersion() == 10 && minorVersion() < 8)
            return QStringLiteral("Mac OS X");
        if (majorVersion() == 10 && minorVersion() < 12)
            return QStringLiteral("OS X");
        return QStringLiteral("macOS");
    }
    case QOperatingSystemVersion::IOS: {
        if (majorVersion() < 4)
            return QStringLiteral("iPhone OS");
        return QStringLiteral("iOS");
    }
    case QOperatingSystemVersion::TvOS:
        return QStringLiteral("tvOS");
    case QOperatingSystemVersion::WatchOS:
        return QStringLiteral("watchOS");
    case QOperatingSystemVersion::Android:
        return QStringLiteral("Android");
    case QOperatingSystemVersion::Unknown:
    default:
        return QString();
    }
}

/*!
    \variable QOperatingSystemVersion::Windows7
    \brief a version corresponding to Windows 7 (version 6.1).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows7 = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 1);

/*!
    \variable QOperatingSystemVersion::Windows8
    \brief a version corresponding to Windows 8 (version 6.2).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows8 = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 2);

/*!
    \variable QOperatingSystemVersion::Windows8_1
    \brief a version corresponding to Windows 8.1 (version 6.3).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows8_1 = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 6, 3);

/*!
    \variable QOperatingSystemVersion::Windows10
    \brief a version corresponding to Windows 10 (version 10.0).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::Windows10 = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10);

/*!
    \variable QOperatingSystemVersion::OSXMavericks
    \brief a version corresponding to OS X Mavericks (version 10.9).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXMavericks = QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 9);

/*!
    \variable QOperatingSystemVersion::OSXYosemite
    \brief a version corresponding to OS X Yosemite (version 10.10).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXYosemite = QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 10);

/*!
    \variable QOperatingSystemVersion::OSXElCapitan
    \brief a version corresponding to OS X El Capitan (version 10.11).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::OSXElCapitan = QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 11);

/*!
    \variable QOperatingSystemVersion::MacOSSierra
    \brief a version corresponding to macOS Sierra (version 10.12).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::MacOSSierra = QOperatingSystemVersion(QOperatingSystemVersion::MacOS, 10, 12);

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean
    \brief a version corresponding to Android Jelly Bean (version 4.1, API level 16).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean = QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 1);

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean_MR1
    \brief a version corresponding to Android Jelly Bean, maintenance release 1 (version 4.2, API level 17).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean_MR1 = QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 2);

/*!
    \variable QOperatingSystemVersion::AndroidJellyBean_MR2
    \brief a version corresponding to Android Jelly Bean, maintenance release 2 (version 4.3, API level 18).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidJellyBean_MR2 = QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 3);

/*!
    \variable QOperatingSystemVersion::AndroidKitKat
    \brief a version corresponding to Android KitKat (versions 4.4 & 4.4W, API levels 19 & 20).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidKitKat = QOperatingSystemVersion(QOperatingSystemVersion::Android, 4, 4);

/*!
    \variable QOperatingSystemVersion::AndroidLollipop
    \brief a version corresponding to Android Lollipop (version 5.0, API level 21).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidLollipop = QOperatingSystemVersion(QOperatingSystemVersion::Android, 5, 0);

/*!
    \variable QOperatingSystemVersion::AndroidLollipop_MR1
    \brief a version corresponding to Android Lollipop, maintenance release 1 (version 5.1, API level 22).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidLollipop_MR1 = QOperatingSystemVersion(QOperatingSystemVersion::Android, 5, 1);

/*!
    \variable QOperatingSystemVersion::AndroidMarshmallow
    \brief a version corresponding to Android Marshmallow (version 6.0, API level 23).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidMarshmallow = QOperatingSystemVersion(QOperatingSystemVersion::Android, 6, 0);

/*!
    \variable QOperatingSystemVersion::AndroidNougat
    \brief a version corresponding to Android Nougat (version 7.0, API level 24).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidNougat = QOperatingSystemVersion(QOperatingSystemVersion::Android, 7, 0);

/*!
    \variable QOperatingSystemVersion::AndroidNougat_MR1
    \brief a version corresponding to Android Nougat, maintenance release 1 (version 7.0, API level 25).
    \since 5.9
 */
const QOperatingSystemVersion QOperatingSystemVersion::AndroidNougat_MR1 = QOperatingSystemVersion(QOperatingSystemVersion::Android, 7, 1);

QT_END_NAMESPACE
