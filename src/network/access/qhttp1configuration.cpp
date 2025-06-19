// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttp1configuration.h"

#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

// QHttp1ConfigurationPrivate is unused until we need it:
static_assert(sizeof(QHttp1Configuration) == sizeof(void*),
              "You have added too many members to QHttp1Configuration::ShortData. "
              "Decrease their size or switch to using a d-pointer.");

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

/*!
    Default constructs a QHttp1Configuration object.
*/
QHttp1Configuration::QHttp1Configuration()
    : u(ShortData{6, {}}) // QHttpNetworkConnectionPrivate::defaultHttpChannelCount
{
}

/*!
    Copy-constructs this QHttp1Configuration.
*/
QHttp1Configuration::QHttp1Configuration(const QHttp1Configuration &)
    = default;

/*!
    \fn QHttp1Configuration::QHttp1Configuration(QHttp1Configuration &&other)

    Move-constructs this QHttp1Configuration from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    Copy-assigns \a other to this QHttp1Configuration.
*/
QHttp1Configuration &QHttp1Configuration::operator=(const QHttp1Configuration &)
    = default;

/*!
    \fn QHttp1Configuration &QHttp1Configuration::operator=(QHttp1Configuration &&)

    Move-assigns \a other to this QHttp1Configuration.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    Destructor.
*/
QHttp1Configuration::~QHttp1Configuration()
    = default;

/*!
    Sets the number of connections (minimum: 1; maximum: 255)
    used per http(s) \e{host}:\e{port} combination to \a number.

    If \a number is ≤ 0, does nothing. If \a number is > 255, 255 is used.

    \sa numberOfConnectionsPerHost
*/
void QHttp1Configuration::setNumberOfConnectionsPerHost(qsizetype number)
{
    auto n = qt_saturate<std::uint8_t>(number);
    if (n == 0)
        return;
    u.data.numConnectionsPerHost = n;
}

/*!
    Returns the number of connections used per http(s) \c{host}:\e{port}
    combination. The default is six (6).

    \sa setNumberOfConnectionsPerHost
*/
qsizetype QHttp1Configuration::numberOfConnectionsPerHost() const
{
    return u.data.numConnectionsPerHost;
}

/*!
    \fn void QHttp1Configuration::swap(QHttp1Configuration &other)
    \memberswap{HTTP/1 configuration}
*/

/*!
    \fn bool QHttp1Configuration::operator==(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    \since 6.5

    Returns \c true if \a lhs and \a rhs represent the same set of HTTP/1
    parameters.
*/

/*!
    \fn bool QHttp1Configuration::operator!=(const QHttp1Configuration &lhs, const QHttp1Configuration &rhs) noexcept
    \since 6.5

    Returns \c true if \a lhs and \a rhs do not represent the same set of
    HTTP/1 parameters.
*/

/*!
    \fn size_t QHttp1Configuration::qHash(const QHttp1Configuration &key, size_t seed)
    \since 6.5
    \qhash{QHttp1Configuration}
*/

/*!
    \internal
*/
bool QHttp1Configuration::equals(const QHttp1Configuration &other) const noexcept
{
    return u.data.numConnectionsPerHost == other.u.data.numConnectionsPerHost;
}

/*!
    \internal
*/
size_t QHttp1Configuration::hash(size_t seed) const noexcept
{
    return qHash(u.data.numConnectionsPerHost, seed);
}

QT_END_NAMESPACE
