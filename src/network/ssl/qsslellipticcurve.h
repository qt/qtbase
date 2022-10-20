/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QSSLELLIPTICCURVE_H
#define QSSLELLIPTICCURVE_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

class QSslEllipticCurve;
// qHash is a friend, but we can't use default arguments for friends (§8.3.6.4)
constexpr size_t qHash(QSslEllipticCurve curve, size_t seed = 0) noexcept;

class QSslEllipticCurve {
public:
    constexpr QSslEllipticCurve() noexcept
        : id(0)
    {
    }

    Q_NETWORK_EXPORT static QSslEllipticCurve fromShortName(const QString &name);
    Q_NETWORK_EXPORT static QSslEllipticCurve fromLongName(const QString &name);

    [[nodiscard]] Q_NETWORK_EXPORT QString shortName() const;
    [[nodiscard]] Q_NETWORK_EXPORT QString longName() const;

    constexpr bool isValid() const noexcept
    {
        return id != 0;
    }

    Q_NETWORK_EXPORT bool isTlsNamedCurve() const noexcept;

private:
    int id;

    friend constexpr bool operator==(QSslEllipticCurve lhs, QSslEllipticCurve rhs) noexcept
    { return lhs.id == rhs.id; }
    friend constexpr bool operator!=(QSslEllipticCurve lhs, QSslEllipticCurve rhs) noexcept
    { return !(lhs == rhs); }
    friend constexpr size_t qHash(QSslEllipticCurve curve, size_t seed) noexcept;

    friend class QSslContext;
    friend class QSslSocketPrivate;
};

Q_DECLARE_TYPEINFO(QSslEllipticCurve, Q_PRIMITIVE_TYPE);

constexpr inline size_t qHash(QSslEllipticCurve curve, size_t seed) noexcept
{ return qHash(curve.id, seed); }

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, QSslEllipticCurve curve);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslEllipticCurve)

#endif // QSSLELLIPTICCURVE_H
