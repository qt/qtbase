// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhstspolicy.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/*!
    \class QHstsPolicy
    \brief The QHstsPolicy class specifies that a host supports HTTP Strict Transport
           Security policy (HSTS).
    \since 5.9
    \ingroup network
    \inmodule QtNetwork

    HSTS policy defines a period of time during which QNetworkAccessManager
    should only access a host in a secure fashion. HSTS policy is defined by
    RFC6797.

    You can set expiry time and host name for this policy, and control whether it
    applies to subdomains, either in the constructor or by calling \l setExpiry(),
    \l setHost() and \l setIncludesSubDomains().

    \sa QNetworkAccessManager::setStrictTransportSecurityEnabled()
*/

/*
    \enum QHstsPolicy::PolicyFlag

    Specifies attributes that a policy can have.

    \value IncludeSubDomains HSTS policy also applies to subdomains.
*/

class QHstsPolicyPrivate : public QSharedData
{
public:
    QUrl url;
    QDateTime expiry;
    bool includeSubDomains = false;

    bool operator == (const QHstsPolicyPrivate &other) const
    {
        return url.host() == other.url.host() && expiry == other.expiry
               && includeSubDomains == other.includeSubDomains;
    }
};

/*!
    \fn bool QHstsPolicy::operator==(const QHstsPolicy &lhs, const QHstsPolicy &rhs)

    Returns \c true if the two policies \a lhs and \a rhs have the same host and
    expiration date while agreeing on whether to include or exclude subdomains.
*/

/*!
    \fn bool QHstsPolicy::operator!=(const QHstsPolicy &lhs, const QHstsPolicy &rhs)

    Returns \c true if the two policies \a lhs and \a rhs do not have the same host
    or expiration date, or do not agree on whether to include or exclude subdomains.
*/

/*!
    \internal
*/
bool QHstsPolicy::isEqual(const QHstsPolicy &other) const
{
    return *d == *other.d;
}

/*!
    Constructs an invalid (expired) policy with empty host name and subdomains
    not included.
*/
QHstsPolicy::QHstsPolicy() : d(new QHstsPolicyPrivate)
{
}

/*!
  \enum QHstsPolicy::PolicyFlag

  \value IncludeSubDomains Indicates whether a policy must include subdomains
*/

/*!
    Constructs QHstsPolicy with \a expiry (in UTC); \a flags is a value indicating
    whether this policy must also include subdomains, \a host data is interpreted
    according to \a mode.

    \sa QUrl::setHost(), QUrl::ParsingMode, QHstsPolicy::PolicyFlag
*/
QHstsPolicy::QHstsPolicy(const QDateTime &expiry, PolicyFlags flags,
                         const QString &host, QUrl::ParsingMode mode)
    : d(new QHstsPolicyPrivate)
{
    d->url.setHost(host, mode);
    d->expiry = expiry;
    d->includeSubDomains = flags.testFlag(IncludeSubDomains);
}

/*!
    Creates a copy of \a other object.
*/
QHstsPolicy::QHstsPolicy(const QHstsPolicy &other)
                : d(new QHstsPolicyPrivate(*other.d))
{
}

/*!
    Destructor.
*/
QHstsPolicy::~QHstsPolicy()
{
}

/*!
    Copy-assignment operator, makes a copy of \a other.
*/
QHstsPolicy &QHstsPolicy::operator=(const QHstsPolicy &other)
{
    d = other.d;
    return *this;
}

/*!
    Sets a host, \a host data is interpreted according to \a mode parameter.

    \sa host(), QUrl::setHost(), QUrl::ParsingMode
*/
void QHstsPolicy::setHost(const QString &host, QUrl::ParsingMode mode)
{
    d->url.setHost(host, mode);
}

/*!
    Returns a host for a given policy, formatted according to \a options.

    \sa setHost(), QUrl::host(), QUrl::ComponentFormattingOptions
*/
QString QHstsPolicy::host(QUrl::ComponentFormattingOptions options) const
{
    return d->url.host(options);
}

/*!
    Sets the expiration date for the policy (in UTC) to \a expiry.

    \sa expiry()
*/
void QHstsPolicy::setExpiry(const QDateTime &expiry)
{
    d->expiry = expiry;
}

/*!
    Returns the expiration date for the policy (in UTC).

    \sa setExpiry()
*/
QDateTime QHstsPolicy::expiry() const
{
    return d->expiry;
}

/*!
    Sets whether subdomains are included for this policy to \a include.

    \sa includesSubDomains()
*/
void QHstsPolicy::setIncludesSubDomains(bool include)
{
    d->includeSubDomains = include;
}

/*!
    Returns \c true if this policy also includes subdomains.

    \sa setIncludesSubDomains()
 */
bool QHstsPolicy::includesSubDomains() const
{
    return d->includeSubDomains;
}

/*!
    Return \c true if this policy has a valid expiration date and this date
    is greater than QDateTime::currentGetDateTimeUtc().

    \sa setExpiry(), expiry()
*/
bool QHstsPolicy::isExpired() const
{
    return !d->expiry.isValid() || d->expiry <= QDateTime::currentDateTimeUtc();
}

/*!
  \fn void QHstsPolicy::swap(QHstsPolicy &other)
    \memberswap{policy}
*/

QT_END_NAMESPACE
