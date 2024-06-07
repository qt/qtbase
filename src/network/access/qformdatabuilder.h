// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFORMDATABUILDER_H
#define QFORMDATABUILDER_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtNetwork/qhttpheaders.h>
#include <QtNetwork/qhttpmultipart.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>

#include <memory>
#include <variant>
#include <vector>

#ifndef Q_OS_WASM
QT_REQUIRE_CONFIG(http);
#endif

class tst_QFormDataBuilder;

QT_BEGIN_NAMESPACE

class QHttpPartPrivate;
class QHttpMultiPart;
class QDebug;

class QFormDataPartBuilder
{
    struct PrivateConstructor { explicit PrivateConstructor() = default; };
public:
    Q_NETWORK_EXPORT explicit QFormDataPartBuilder(QAnyStringView name, PrivateConstructor);

    QFormDataPartBuilder(QFormDataPartBuilder &&other) noexcept
        : m_headerValue(std::move(other.m_headerValue)),
          m_originalBodyName(std::move(other.m_originalBodyName)),
          m_httpHeaders(std::move(other.m_httpHeaders)),
          m_body(std::move(other.m_body)),
          m_reserved(std::exchange(other.m_reserved, nullptr))
    {

    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFormDataPartBuilder)
    void swap(QFormDataPartBuilder &other) noexcept
    {
        m_headerValue.swap(other.m_headerValue);
        m_originalBodyName.swap(other.m_originalBodyName);
        m_httpHeaders.swap(other.m_httpHeaders);
        m_body.swap(other.m_body);
        qt_ptr_swap(m_reserved, other.m_reserved);
    }

    Q_NETWORK_EXPORT ~QFormDataPartBuilder();

    Q_WEAK_OVERLOAD QFormDataPartBuilder &setBody(const QByteArray &data,
                                                  QAnyStringView fileName = {},
                                                  QAnyStringView mimeType = {})
    { return setBodyHelper(data, fileName, mimeType); }

    Q_NETWORK_EXPORT QFormDataPartBuilder &setBody(QByteArrayView data,
                                                   QAnyStringView fileName = {},
                                                   QAnyStringView mimeType = {});
    Q_NETWORK_EXPORT QFormDataPartBuilder &setBodyDevice(QIODevice *body,
                                                         QAnyStringView fileName = {},
                                                         QAnyStringView mimeType = {});
    Q_NETWORK_EXPORT QFormDataPartBuilder &setHeaders(const QHttpHeaders &headers);
private:
    Q_DISABLE_COPY(QFormDataPartBuilder)

    Q_NETWORK_EXPORT QFormDataPartBuilder &setBodyHelper(const QByteArray &data,
                                                         QAnyStringView fileName,
                                                         QAnyStringView mimeType);
    Q_NETWORK_EXPORT QHttpPart build();

    QByteArray m_headerValue;
    QByteArray m_mimeType;
    QString m_originalBodyName;
    QHttpHeaders m_httpHeaders;
    std::variant<QIODevice*, QByteArray> m_body;
    void *m_reserved = nullptr;

    friend class QFormDataBuilder;
    friend class ::tst_QFormDataBuilder;
    friend void swap(QFormDataPartBuilder &lhs, QFormDataPartBuilder &rhs) noexcept
    { lhs.swap(rhs); }
};

class QFormDataBuilder
{
public:
    Q_NETWORK_EXPORT explicit QFormDataBuilder();

    QFormDataBuilder(QFormDataBuilder &&other) noexcept
        : m_parts(std::move(other.m_parts)),
          m_reserved(std::exchange(other.m_reserved, nullptr))
    {

    }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFormDataBuilder)
    void swap(QFormDataBuilder &other) noexcept
    {
        m_parts.swap(other.m_parts);
        qt_ptr_swap(m_reserved, other.m_reserved);
    }

    Q_NETWORK_EXPORT ~QFormDataBuilder();
    Q_NETWORK_EXPORT QFormDataPartBuilder &part(QAnyStringView name);
    Q_NETWORK_EXPORT std::unique_ptr<QHttpMultiPart> buildMultiPart();
private:
    std::vector<QFormDataPartBuilder> m_parts;
    void *m_reserved = nullptr;

    friend void swap(QFormDataBuilder &lhs, QFormDataBuilder &rhs) noexcept
    { lhs.swap(rhs); }

    Q_DISABLE_COPY(QFormDataBuilder)
};

QT_END_NAMESPACE

#endif // QFORMDATABUILDER_H
