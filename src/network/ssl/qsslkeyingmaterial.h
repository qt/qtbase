// Copyright (C) 2026 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSLKEYINGMATERIAL_H
#define QSSLKEYINGMATERIAL_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QString>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {
class TlsCryptographOpenSSL;
}

class QSslKeyingMaterial
{
public:
    explicit QSslKeyingMaterial(const QByteArray &label,
                                qsizetype size,
                                const QByteArray &context = QByteArray()) noexcept
        : keyingLabel(label)
        , keyingContext(context)
        , keyingValue()
        , keyingValueSize(size)
    {
    }

    bool isValid() const noexcept
    {
        return !label().isEmpty() && requestedSize() > 0;
    }

    QByteArray label() const noexcept
    {
        return keyingLabel;
    }

    QByteArray context() const noexcept
    {
        return keyingContext;
    }

    QByteArray value() const noexcept
    {
        return keyingValue;
    }

    qsizetype requestedSize() const noexcept
    {
        return keyingValueSize;
    }

private:
    QByteArray keyingLabel;
    QByteArray keyingContext;
    QByteArray keyingValue;
    qsizetype keyingValueSize;

    friend bool comparesEqual(const QSslKeyingMaterial &lhs,
                              const QSslKeyingMaterial &rhs) noexcept
    {
        return lhs.keyingLabel == rhs.keyingLabel
               && lhs.keyingContext == rhs.keyingContext
               && lhs.keyingValue == rhs.keyingValue
               && lhs.keyingValueSize == rhs.keyingValueSize;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QSslKeyingMaterial)

    friend class QTlsPrivate::TlsCryptographOpenSSL;
};

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, QSslKeyingMaterial curve);
#endif

inline size_t qHash(const QSslKeyingMaterial &material, size_t seed = 0)
{
    return qHashMulti(seed,
                      material.label(),
                      material.context(),
                      material.value(),
                      material.requestedSize());
}

QT_END_NAMESPACE

#endif // QSSLKEYINGMATERIAL_H
