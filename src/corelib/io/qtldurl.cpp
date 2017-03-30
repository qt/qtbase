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

#if QT_CONFIG(topleveldomain)

#include "qplatformdefs.h"
#include "qurl.h"
#include "private/qurltlds_p.h"
#include "private/qtldurl_p.h"
#include "QtCore/qstring.h"
#include "QtCore/qvector.h"

QT_BEGIN_NAMESPACE

enum TLDMatchType {
    ExactMatch,
    SuffixMatch,
    ExceptionMatch,
};

static bool containsTLDEntry(QStringView entry, TLDMatchType match)
{
    const QStringView matchSymbols[] = {
        QStringViewLiteral(""),
        QStringViewLiteral("*"),
        QStringViewLiteral("!"),
    };
    const auto symbol = matchSymbols[match];
    int index = qt_hash(entry, qt_hash(symbol)) % tldCount;

    // select the right chunk from the big table
    short chunk = 0;
    uint chunkIndex = tldIndices[index], offset = 0;
    while (chunk < tldChunkCount && tldIndices[index] >= tldChunks[chunk]) {
        chunkIndex -= tldChunks[chunk];
        offset += tldChunks[chunk];
        chunk++;
    }

    // check all the entries from the given index
    while (chunkIndex < tldIndices[index+1] - offset) {
        const auto utf8 = tldData[chunk] + chunkIndex;
        if ((symbol.isEmpty() || QLatin1Char(*utf8) == symbol) && entry == QString::fromUtf8(utf8 + symbol.size()))
            return true;
        chunkIndex += qstrlen(utf8) + 1; // +1 for the ending \0
    }
    return false;
}

/*!
    \internal

    Return the top-level-domain per Qt's copy of the Mozilla public suffix list of
    \a domain.
*/

Q_CORE_EXPORT QString qTopLevelDomain(const QString &domain)
{
    const QString domainLower = domain.toLower();
    QVector<QStringRef> sections = domainLower.splitRef(QLatin1Char('.'), QString::SkipEmptyParts);
    if (sections.isEmpty())
        return QString();

    QString level, tld;
    for (int j = sections.count() - 1; j >= 0; --j) {
        level.prepend(QLatin1Char('.') + sections.at(j));
        if (qIsEffectiveTLD(level.rightRef(level.size() - 1)))
            tld = level;
    }
    return tld;
}

/*!
    \internal

    Return true if \a domain is a top-level-domain per Qt's copy of the Mozilla public suffix list.
*/

Q_CORE_EXPORT bool qIsEffectiveTLD(const QStringRef &domain)
{
    // for domain 'foo.bar.com':
    // 1. return if TLD table contains 'foo.bar.com'
    // 2. else if table contains '*.bar.com',
    // 3. test that table does not contain '!foo.bar.com'

    if (containsTLDEntry(domain, ExactMatch)) // 1
        return true;

    const int dot = domain.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        if (containsTLDEntry(domain.mid(dot), SuffixMatch))   // 2
            return !containsTLDEntry(domain, ExceptionMatch); // 3
    }
    return false;
}

QT_END_NAMESPACE

#endif
