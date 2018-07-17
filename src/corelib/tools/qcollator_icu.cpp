/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "qcollator_p.h"
#include "qlocale_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>

#include "qdebug.h"

QT_BEGIN_NAMESPACE

void QCollatorPrivate::init()
{
    cleanup();

    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = QLocalePrivate::get(locale)->bcp47Name('_');
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status)) {
        qWarning("Could not create collator: %d", status);
        collator = 0;
        dirty = false;
        return;
    }

    // enable normalization by default
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

    // The strength attribute in ICU is rather badly documented. Basically UCOL_PRIMARY
    // ignores differences between base characters and accented characters as well as case.
    // So A and A-umlaut would compare equal.
    // UCOL_SECONDARY ignores case differences. UCOL_TERTIARY is the default in most languages
    // and does case sensitive comparison.
    // UCOL_QUATERNARY is used as default in a few languages such as Japanese to take care of some
    // additional differences in those languages.
    UColAttributeValue val = (caseSensitivity == Qt::CaseSensitive) ? UCOL_DEFAULT_STRENGTH : UCOL_SECONDARY;

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_STRENGTH, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, numericMode ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: numeric collation failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, ignorePunctuation ? UCOL_SHIFTED : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Alternate handling failed: %d", status);

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        ucol_close(collator);
    collator = 0;
}

int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
    if (d->dirty)
        d->init();

    if (d->collator)
        return ucol_strcoll(d->collator, (const UChar *)s1, len1, (const UChar *)s2, len2);

    return QString::compare_helper(s1, len1, s2, len2, d->caseSensitivity);
}

int QCollator::compare(const QString &s1, const QString &s2) const
{
    if (d->dirty)
        d->init();

    if (d->collator)
        return compare(s1.constData(), s1.size(), s2.constData(), s2.size());

    return QString::compare(s1, s2, d->caseSensitivity);
}

int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    if (d->dirty)
        d->init();

    if (d->collator)
        return compare(s1.constData(), s1.size(), s2.constData(), s2.size());

    return QStringRef::compare(s1, s2, d->caseSensitivity);
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (d->dirty)
        d->init();

    if (d->collator) {
        QByteArray result(16 + string.size() + (string.size() >> 2), Qt::Uninitialized);
        int size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                                   string.size(), (uint8_t *)result.data(), result.size());
        if (size > result.size()) {
            result.resize(size);
            size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                                   string.size(), (uint8_t *)result.data(), result.size());
        }
        result.truncate(size);
        return QCollatorSortKey(new QCollatorSortKeyPrivate(std::move(result)));
    }

    return QCollatorSortKey(new QCollatorSortKeyPrivate(QByteArray()));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return qstrcmp(d->m_key, otherKey.d->m_key);
}

QT_END_NAMESPACE
