/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <qglobal.h>

#include <QtNetwork/private/qtnetworkglobal_p.h>

#if QT_CONFIG(topleveldomain)

#include "qurl.h"
#include "private/qtldurl_p.h"
#include "QtCore/qstring.h"

#include "qurltlds_p.h"

// Defined in src/3rdparty/libpsl/src/lookup_string_in_fixed_set.c
extern "C" int LookupStringInFixedSet(const unsigned char *graph, std::size_t length,
                                      const char *key, std::size_t key_length);

QT_BEGIN_NAMESPACE

static constexpr int PSL_NOT_FOUND = -1;
static constexpr int PSL_FLAG_EXCEPTION = 1 << 0;
static constexpr int PSL_FLAG_WILDCARD = 1 << 1;

static int lookupDomain(QByteArrayView domain)
{
    return LookupStringInFixedSet(kDafsa, sizeof(kDafsa), domain.data(), domain.size());
}

/*!
    \internal

    Return true if \a domain is a top-level-domain per Qt's copy of the Mozilla public suffix list.

    The \a domain must be in lower-case format (as per QString::toLower()).
*/

Q_NETWORK_EXPORT bool qIsEffectiveTLD(QStringView domain)
{
    // for domain 'foo.bar.com':
    // 1. return false if TLD table contains '!foo.bar.com'
    // 2. return true if TLD table contains 'foo.bar.com'
    // 3. return true if the table contains '*.bar.com'

    QByteArray decodedDomain = domain.toUtf8();
    QByteArrayView domainView(decodedDomain);

    auto ret = lookupDomain(domainView);
    if (ret != PSL_NOT_FOUND) {
        if (ret & PSL_FLAG_EXCEPTION) // 1
            return false;
        if ((ret & PSL_FLAG_WILDCARD) == 0) // 2
            return true;
    }

    const auto dot = domainView.indexOf('.');
    if (dot < 0) // Actual TLD: may be effective if the subject of a wildcard rule:
        return ret != PSL_NOT_FOUND;
    ret = lookupDomain(domainView.sliced(dot + 1)); // 3
    if (ret == PSL_NOT_FOUND)
        return false;
    return (ret & PSL_FLAG_WILDCARD) != 0;
}

QT_END_NAMESPACE

#endif
