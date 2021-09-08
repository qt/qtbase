// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHTTP1CONFIGURATION_H
#define QHTTP1CONFIGURATION_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qshareddata.h>

#ifndef Q_CLANG_QDOC
QT_REQUIRE_CONFIG(http);
#endif

QT_BEGIN_NAMESPACE

class QHttp1ConfigurationPrivate;
class Q_NETWORK_EXPORT QHttp1Configuration
{
public:
    QHttp1Configuration();
    QHttp1Configuration(const QHttp1Configuration &other);
    QHttp1Configuration(QHttp1Configuration &&other) noexcept;

    QHttp1Configuration &operator=(const QHttp1Configuration &other);
    QHttp1Configuration &operator=(QHttp1Configuration &&other) noexcept;

    ~QHttp1Configuration();

    void setNumberOfConnectionsPerHost(unsigned amount);
    unsigned numberOfConnectionsPerHost() const;

    void swap(QHttp1Configuration &other) noexcept;

private:
    QSharedDataPointer<QHttp1ConfigurationPrivate> d;

    bool isEqual(const QHttp1Configuration &other) const noexcept;

    friend bool operator==(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    { return lhs.isEqual(rhs); }
    friend bool operator!=(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    { return !lhs.isEqual(rhs); }

};

Q_DECLARE_SHARED(QHttp1Configuration)

QT_END_NAMESPACE

#endif // QHTTP1CONFIGURATION_H
