// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttp1configuration.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QHttp1Configuration
    \brief The QHttp1Configuration class controls HTTP/1 parameters and settings.
    \since 6.5

    \reentrant
    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    QHttp1Configuration controls HTTP/1 parameters and settings that
    QNetworkAccessManager will use to send requests and process responses.

    \note The configuration must be set before the first request
    was sent to a given host (and thus an HTTP/1 session established).

    \sa QNetworkRequest::setHttp1Configuration(), QNetworkRequest::http1Configuration(), QNetworkAccessManager
*/

class QHttp1ConfigurationPrivate : public QSharedData
{
public:
    unsigned numberOfConnectionsPerHost = 6; // QHttpNetworkConnectionPrivate::defaultHttpChannelCount
};

/*!
    Default constructs a QHttp1Configuration object.
*/
QHttp1Configuration::QHttp1Configuration()
    : d(new QHttp1ConfigurationPrivate)
{
}

/*!
    Copy-constructs this QHttp1Configuration.
*/
QHttp1Configuration::QHttp1Configuration(const QHttp1Configuration &) = default;

/*!
    Move-constructs this QHttp1Configuration from \a other
*/
QHttp1Configuration::QHttp1Configuration(QHttp1Configuration &&other) noexcept
{
    swap(other);
}

/*!
    Copy-assigns \a other to this QHttp1Configuration.
*/
QHttp1Configuration &QHttp1Configuration::operator=(const QHttp1Configuration &) = default;

/*!
    Move-assigns \a other to this QHttp1Configuration.
*/
QHttp1Configuration &QHttp1Configuration::operator=(QHttp1Configuration &&) noexcept = default;

/*!
    Destructor.
*/
QHttp1Configuration::~QHttp1Configuration()
    = default;

/*!
    Sets number of connections (default 6) to a http(s)://host:port
    \sa numberOfConnectionsPerHost
*/
void QHttp1Configuration::setNumberOfConnectionsPerHost(unsigned number)
{
    if (number == 0) {
        return;
    }
    d->numberOfConnectionsPerHost = number;
}
/*!
    Returns the number of connections (default 6) to a http(s)://host:port
    \sa setNumberOfConnectionsPerHost
*/
unsigned QHttp1Configuration::numberOfConnectionsPerHost() const
{
    return d->numberOfConnectionsPerHost;
}

/*!
    Swaps this configuration with the \a other configuration.
*/
void QHttp1Configuration::swap(QHttp1Configuration &other) noexcept
{
    d.swap(other.d);
}

/*!
    \fn bool QHttp1Configuration::operator==(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept

    Returns \c true if \a lhs and \a rhs represent the same set of HTTP/1
    parameters.
*/

/*!
    \fn bool QHttp1Configuration::operator!=(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept

    Returns \c true if \a lhs and \a rhs do not represent the same set of
    HTTP/1 parameters.
*/

/*!
    \internal
*/
bool QHttp1Configuration::isEqual(const QHttp1Configuration &other) const noexcept
{
    if (d == other.d)
        return true;

    return d->numberOfConnectionsPerHost == other.d->numberOfConnectionsPerHost;
}

QT_END_NAMESPACE
