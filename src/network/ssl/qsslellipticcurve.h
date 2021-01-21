/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSSLELLIPTICCURVE_H
#define QSSLELLIPTICCURVE_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#if QT_DEPRECATED_SINCE(5, 6)
#include <QtCore/QHash>
#endif
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

class QSslEllipticCurve;
// qHash is a friend, but we can't use default arguments for friends (§8.3.6.4)
Q_DECL_CONSTEXPR uint qHash(QSslEllipticCurve curve, uint seed = 0) noexcept;

class QSslEllipticCurve {
public:
    Q_DECL_CONSTEXPR QSslEllipticCurve() noexcept
        : id(0)
    {
    }

    Q_NETWORK_EXPORT static QSslEllipticCurve fromShortName(const QString &name);
    Q_NETWORK_EXPORT static QSslEllipticCurve fromLongName(const QString &name);

    Q_REQUIRED_RESULT Q_NETWORK_EXPORT QString shortName() const;
    Q_REQUIRED_RESULT Q_NETWORK_EXPORT QString longName() const;

    Q_DECL_CONSTEXPR bool isValid() const noexcept
    {
        return id != 0;
    }

    Q_NETWORK_EXPORT bool isTlsNamedCurve() const noexcept;

private:
    int id;

    friend Q_DECL_CONSTEXPR bool operator==(QSslEllipticCurve lhs, QSslEllipticCurve rhs) noexcept;
    friend Q_DECL_CONSTEXPR uint qHash(QSslEllipticCurve curve, uint seed) noexcept;

    friend class QSslContext;
    friend class QSslSocketPrivate;
    friend class QSslSocketBackendPrivate;
};

Q_DECLARE_TYPEINFO(QSslEllipticCurve, Q_PRIMITIVE_TYPE);

Q_DECL_CONSTEXPR inline uint qHash(QSslEllipticCurve curve, uint seed) noexcept
{ return qHash(curve.id, seed); }

Q_DECL_CONSTEXPR inline bool operator==(QSslEllipticCurve lhs, QSslEllipticCurve rhs) noexcept
{ return lhs.id == rhs.id; }

Q_DECL_CONSTEXPR inline bool operator!=(QSslEllipticCurve lhs, QSslEllipticCurve rhs) noexcept
{ return !operator==(lhs, rhs); }

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, QSslEllipticCurve curve);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslEllipticCurve)

#endif // QSSLELLIPTICCURVE_H
