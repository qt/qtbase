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

class QDebug;

class QSslKeyingMaterial
{
public:
    explicit QSslKeyingMaterial(const QByteArray &label, qsizetype size)
        : keyingLabel(label)
        , keyingValueSize(size)
    {
    }
    explicit QSslKeyingMaterial(const QByteArray &label,
                                qsizetype size,
                                const QByteArray &context)
        : keyingLabel(label)
        , keyingContext(context)
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

    void swap(QSslKeyingMaterial &other) noexcept
    {
        keyingLabel.swap(other.keyingLabel);
        keyingContext.swap(other.keyingContext);
        keyingValue.swap(other.keyingValue);
        std::swap(keyingValueSize, other.keyingValueSize);
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

    friend size_t qHash(const QSslKeyingMaterial &material) noexcept
    { return qHash(material, 0); }
    friend Q_NETWORK_EXPORT size_t qHash(const QSslKeyingMaterial &material, size_t seed) noexcept;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QSslKeyingMaterial &keying);
#endif // QT_NO_DEBUG_STREAM

    friend class QTlsPrivate::TlsCryptographOpenSSL;
};

Q_DECLARE_SHARED(QSslKeyingMaterial)

QT_END_NAMESPACE

#endif // QSSLKEYINGMATERIAL_H
