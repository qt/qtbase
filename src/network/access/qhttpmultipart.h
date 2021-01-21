/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QHTTPMULTIPART_H
#define QHTTPMULTIPART_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QByteArray>
#include <QtCore/QIODevice>
#include <QtNetwork/QNetworkRequest>

QT_REQUIRE_CONFIG(http);

QT_BEGIN_NAMESPACE


class QHttpPartPrivate;
class QHttpMultiPart;

class Q_NETWORK_EXPORT QHttpPart
{
public:
    QHttpPart();
    QHttpPart(const QHttpPart &other);
    ~QHttpPart();
    QHttpPart &operator=(QHttpPart &&other) noexcept { swap(other); return *this; }
    QHttpPart &operator=(const QHttpPart &other);

    void swap(QHttpPart &other) noexcept { qSwap(d, other.d); }

    bool operator==(const QHttpPart &other) const;
    inline bool operator!=(const QHttpPart &other) const
    { return !operator==(other); }

    void setHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);
    void setRawHeader(const QByteArray &headerName, const QByteArray &headerValue);

    void setBody(const QByteArray &body);
    void setBodyDevice(QIODevice *device);

private:
    QSharedDataPointer<QHttpPartPrivate> d;

    friend class QHttpMultiPartIODevice;
};

Q_DECLARE_SHARED(QHttpPart)

class QHttpMultiPartPrivate;

class Q_NETWORK_EXPORT QHttpMultiPart : public QObject
{
    Q_OBJECT

public:

    enum ContentType {
        MixedType,
        RelatedType,
        FormDataType,
        AlternativeType
    };

    explicit QHttpMultiPart(QObject *parent = nullptr);
    explicit QHttpMultiPart(ContentType contentType, QObject *parent = nullptr);
    ~QHttpMultiPart();

    void append(const QHttpPart &httpPart);

    void setContentType(ContentType contentType);

    QByteArray boundary() const;
    void setBoundary(const QByteArray &boundary);

private:
    Q_DECLARE_PRIVATE(QHttpMultiPart)
    Q_DISABLE_COPY(QHttpMultiPart)

    friend class QNetworkAccessManager;
    friend class QNetworkAccessManagerPrivate;
};

QT_END_NAMESPACE

#endif // QHTTPMULTIPART_H
