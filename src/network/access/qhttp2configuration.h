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

#ifndef QHTTP2CONFIGURATION_H
#define QHTTP2CONFIGURATION_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qshareddata.h>

#ifndef Q_CLANG_QDOC
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE

class QHttp2ConfigurationPrivate;
class Q_NETWORK_EXPORT QHttp2Configuration
{
    friend Q_NETWORK_EXPORT bool operator==(const QHttp2Configuration &lhs, const QHttp2Configuration &rhs);

public:
    QHttp2Configuration();
    QHttp2Configuration(const QHttp2Configuration &other);
    QHttp2Configuration(QHttp2Configuration &&other) noexcept;
    QHttp2Configuration &operator = (const QHttp2Configuration &other);
    QHttp2Configuration &operator = (QHttp2Configuration &&other) noexcept;

    ~QHttp2Configuration();

    void setServerPushEnabled(bool enable);
    bool serverPushEnabled() const;

    void setHuffmanCompressionEnabled(bool enable);
    bool huffmanCompressionEnabled() const;

    bool setSessionReceiveWindowSize(unsigned size);
    unsigned sessionReceiveWindowSize() const;

    bool setStreamReceiveWindowSize(unsigned size);
    unsigned streamReceiveWindowSize() const;

    bool setMaxFrameSize(unsigned size);
    unsigned maxFrameSize() const;

    void swap(QHttp2Configuration &other) noexcept;

private:

    QSharedDataPointer<QHttp2ConfigurationPrivate> d;
};

Q_DECLARE_SHARED(QHttp2Configuration)

Q_NETWORK_EXPORT bool operator==(const QHttp2Configuration &lhs, const QHttp2Configuration &rhs);

inline bool operator!=(const QHttp2Configuration &lhs, const QHttp2Configuration &rhs)
{
    return !(lhs == rhs);
}

QT_END_NAMESPACE

#endif // QHTTP2CONFIGURATION_H
