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

#ifndef QHSTS_P_H
#define QHSTS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>
#include <QtCore/qvector.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qurl.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QHstsCache
{
public:

    QHstsCache();

    void updateFromHeaders(const QList<QPair<QByteArray, QByteArray>> &headers,
                           const QUrl &url);
    void updateKnownHost(const QUrl &url, const QDateTime &expires,
                         bool includeSubDomains);
    bool isKnownHost(const QUrl &url) const;

    void clear();

private:

    using size_type = std::vector<int>::size_type;

    struct DomainLabel
    {
        DomainLabel() = default;
        DomainLabel(const QString &name) : label(name) { }

        bool operator < (const DomainLabel &rhs) const
        { return label < rhs.label; }

        QString label;
        size_type domainIndex;
    };

    struct Domain
    {
        void setHostPolicy(const QDateTime &expiration, bool subs)
        {
            expirationTime = expiration;
            isKnownHost = expirationTime.isValid()
                          && expirationTime > QDateTime::currentDateTimeUtc();
            includeSubDomains = subs;
        }

        bool validateHostPolicy()
        {
            if (!isKnownHost)
                return false;

            if (expirationTime > QDateTime::currentDateTimeUtc())
                return true;

            isKnownHost = false;
            includeSubDomains = false;
            return false;
        }

        bool isKnownHost = false;
        bool includeSubDomains = false;
        QDateTime expirationTime;
        std::vector<DomainLabel> labels;
    };

    /*
    Each Domain represents a DNS name or prefix thereof; each entry in its
    std::vector<DomainLabel> labels pairs the next fragment of a DNS name
    with the index into 'children' at which to find another Domain object.
    The root Domain, children[0], has top-level-domain DomainLabel entries,
    such as "com", "org" and "net"; the entry in 'children' at the index it
    pairs with "com" is the Domain entry for .com; if that has "example" in
    its labels, it'll be paired with the index of the entry in 'children'
    that represents example.com; from which, in turn, we can find the
    Domain object for www.example.com, and so on.
    */
    mutable std::vector<Domain> children;
};

class Q_AUTOTEST_EXPORT QHstsHeaderParser
{
public:

    bool parse(const QList<QPair<QByteArray, QByteArray>> &headers);

    QDateTime expirationDate() const { return expiry; }
    bool includeSubDomains() const { return subDomainsFound; }

private:

    bool parseSTSHeader();
    bool parseDirective();
    bool processDirective(const QByteArray &name, const QByteArray &value);
    bool nextToken();

    QByteArray header;
    QByteArray token;

    QDateTime expiry;
    int tokenPos = 0;
    bool maxAgeFound = false;
    qint64 maxAge = 0;
    bool subDomainsFound = false;
};

QT_END_NAMESPACE

#endif
