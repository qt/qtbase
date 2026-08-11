// Copyright (C) 2026 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSSLKEYINGMATERIAL_H
#define QSSLKEYINGMATERIAL_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qcompare.h>
#include <QtCore/qtypes.h>

QT_REQUIRE_CONFIG(ssl);


QT_BEGIN_NAMESPACE

namespace QTlsPrivate {
class TlsCryptographOpenSSL;
}

class QDebug;

class QSslKeyingMaterial
{
public:
    QSslKeyingMaterial() = default;
    explicit QSslKeyingMaterial(const QByteArray &label, qsizetype size)
        : m_label(label)
        , m_requestedSize(size)
    {
    }
    explicit QSslKeyingMaterial(const QByteArray &label,
                                qsizetype size,
                                const QByteArray &context)
        : m_label(label)
        , m_context(context)
        , m_requestedSize(size)
    {
    }

    bool isValid() const noexcept
    {
        return !m_label.isEmpty() && requestedSize() > 0;
    }

    QByteArray label() const noexcept
    {
        return m_label;
    }

    QByteArray context() const noexcept
    {
        return m_context;
    }

    QByteArray value() const noexcept
    {
        return m_value;
    }

    qsizetype requestedSize() const noexcept
    {
        return m_requestedSize;
    }

    void swap(QSslKeyingMaterial &other) noexcept
    {
        m_label.swap(other.m_label);
        m_context.swap(other.m_context);
        m_value.swap(other.m_value);
        std::swap(m_requestedSize, other.m_requestedSize);
    }

private:
    QByteArray m_label;
    QByteArray m_context;
    QByteArray m_value;
    qsizetype m_requestedSize = 0;

    friend bool comparesEqual(const QSslKeyingMaterial &lhs,
                              const QSslKeyingMaterial &rhs) noexcept
    {
        return lhs.m_requestedSize == rhs.m_requestedSize
               && lhs.m_label == rhs.m_label
               && lhs.m_context == rhs.m_context
               && lhs.m_value == rhs.m_value;
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
