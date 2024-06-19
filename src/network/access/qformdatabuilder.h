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

#ifndef Q_OS_WASM
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE

class QHttpPartPrivate;
class QHttpMultiPart;
class QDebug;

class QFormDataBuilderPrivate;
class QFormDataPartBuilderPrivate;

class QFormDataPartBuilder
{
    QFormDataPartBuilder(QFormDataBuilderPrivate *qfdb, qsizetype idx) : d(qfdb), m_index(idx) {}
public:
    void swap(QFormDataPartBuilder &other) noexcept
    {
        qt_ptr_swap(d, other.d);
        std::swap(m_index, other.m_index);
    }

    QFormDataPartBuilder() = default;
    // Rule of zero applies

    Q_WEAK_OVERLOAD QFormDataPartBuilder setBody(const QByteArray &data,
                                                 QAnyStringView fileName = {},
                                                 QAnyStringView mimeType = {})
    { return setBodyHelper(data, fileName, mimeType); }

    Q_NETWORK_EXPORT QFormDataPartBuilder setBody(QByteArrayView data,
                                                  QAnyStringView fileName = {},
                                                  QAnyStringView mimeType = {});
    Q_NETWORK_EXPORT QFormDataPartBuilder setBodyDevice(QIODevice *body,
                                                        QAnyStringView fileName = {},
                                                        QAnyStringView mimeType = {});
    Q_NETWORK_EXPORT QFormDataPartBuilder setHeaders(const QHttpHeaders &headers);
private:
    Q_NETWORK_EXPORT QFormDataPartBuilder setBodyHelper(const QByteArray &data,
                                                        QAnyStringView fileName,
                                                        QAnyStringView mimeType);

    QFormDataPartBuilderPrivate* d_func();
    const QFormDataPartBuilderPrivate* d_func() const;

    QFormDataBuilderPrivate *d;
    size_t m_index;

    friend class QFormDataBuilder;
};

Q_DECLARE_SHARED(QFormDataPartBuilder)

class QFormDataBuilder
{
public:
    Q_NETWORK_EXPORT QFormDataBuilder();

    QFormDataBuilder(QFormDataBuilder &&other) noexcept : d_ptr(std::exchange(other.d_ptr, nullptr)) {}

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QFormDataBuilder)
    void swap(QFormDataBuilder &other) noexcept
    {
        qt_ptr_swap(d_ptr, other.d_ptr);
    }

    Q_NETWORK_EXPORT ~QFormDataBuilder();
    Q_NETWORK_EXPORT QFormDataPartBuilder part(QAnyStringView name);
    Q_NETWORK_EXPORT std::unique_ptr<QHttpMultiPart> buildMultiPart();
private:
    QFormDataBuilderPrivate *d_ptr;

    Q_DECLARE_PRIVATE(QFormDataBuilder)
    Q_DISABLE_COPY(QFormDataBuilder)
};

Q_DECLARE_SHARED(QFormDataBuilder)

QT_END_NAMESPACE

#endif // QFORMDATABUILDER_H
