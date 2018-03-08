/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    applies to subdomains, either in the constructor or by calling setExpiry(),
    setHost() and setIncludesSubdomains().

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
    Returns \c true if the two policies have the same host and expiration date
    while agreeing on whether to include or exclude subdomains.
*/
bool operator==(const QHstsPolicy &lhs, const QHstsPolicy &rhs)
{
    return *lhs.d == *rhs.d;
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

  Swaps this policy with the \a other policy.
*/

QT_END_NAMESPACE
