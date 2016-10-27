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

#include <QtCore/qglobal.h>

#ifndef QOPERATINGSYSTEMVERSION_H
#define QOPERATINGSYSTEMVERSION_H

QT_BEGIN_NAMESPACE

class QString;
class QVersionNumber;

class Q_CORE_EXPORT QOperatingSystemVersion
{
public:
    enum OSType {
        Unknown = 0,
        Windows,
        MacOS,
        IOS,
        TvOS,
        WatchOS,
        Android
    };

    static const QOperatingSystemVersion Windows7;
    static const QOperatingSystemVersion Windows8;
    static const QOperatingSystemVersion Windows8_1;
    static const QOperatingSystemVersion Windows10;

    static const QOperatingSystemVersion OSXMavericks;
    static const QOperatingSystemVersion OSXYosemite;
    static const QOperatingSystemVersion OSXElCapitan;
    static const QOperatingSystemVersion MacOSSierra;

    static const QOperatingSystemVersion AndroidJellyBean;
    static const QOperatingSystemVersion AndroidJellyBean_MR1;
    static const QOperatingSystemVersion AndroidJellyBean_MR2;
    static const QOperatingSystemVersion AndroidKitKat;
    static const QOperatingSystemVersion AndroidLollipop;
    static const QOperatingSystemVersion AndroidLollipop_MR1;
    static const QOperatingSystemVersion AndroidMarshmallow;
    static const QOperatingSystemVersion AndroidNougat;
    static const QOperatingSystemVersion AndroidNougat_MR1;

    QOperatingSystemVersion(const QOperatingSystemVersion &other) = default;
    Q_DECL_CONSTEXPR QOperatingSystemVersion(OSType osType, int vmajor, int vminor = -1, int vmicro = -1)
        : m_os(osType), m_major(vmajor), m_minor(vminor), m_micro(vmicro) { }

    static QOperatingSystemVersion current();

    static int compare(const QOperatingSystemVersion &v1, const QOperatingSystemVersion &v2);

    QOperatingSystemVersion fromVersionNumber(const QVersionNumber &version, OSType os);
    QVersionNumber toVersionNumber() const;

    Q_DECL_CONSTEXPR int majorVersion() const { return m_major; }
    Q_DECL_CONSTEXPR int minorVersion() const { return m_minor; }
    Q_DECL_CONSTEXPR int microVersion() const { return m_micro; }

    Q_DECL_CONSTEXPR OSType type() const { return m_os; }
    QString name() const;

private:
    QOperatingSystemVersion() = default;
    OSType m_os;
    int m_major;
    int m_minor;
    int m_micro;
};

inline bool operator>(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) > 0; }

inline bool operator>=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) >= 0; }

inline bool operator<(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) < 0; }

inline bool operator<=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) <= 0; }

inline bool operator==(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) == 0; }

inline bool operator!=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
{ return !(lhs == rhs); }

QT_END_NAMESPACE

#endif // QOPERATINGSYSTEMVERSION_H
