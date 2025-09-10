// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    if (isC())
        return;

    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = QLocalePrivate::get(locale)->bcp47Name('_');
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status)) {
        qWarning("Could not create collator: %d", status);
        collator = nullptr;
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
    UColAttributeValue val = (caseSensitivity == Qt::CaseSensitive)
        ? UCOL_DEFAULT_STRENGTH : UCOL_SECONDARY;

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_STRENGTH, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, numericMode ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: numeric collation failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING,
                      ignorePunctuation ? UCOL_SHIFTED : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Alternate handling failed: %d", status);

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    if (collator)
        ucol_close(collator);
    collator = nullptr;
}

int QCollator::compare(QStringView s1, QStringView s2) const
{
    if (!s1.size())
        return s2.size() ? -1 : 0;
    if (!s2.size())
        return +1;

    d->ensureInitialized();

    if (d->collator) {
        // truncating sizes (QTBUG-105038)
        return ucol_strcoll(d->collator,
                            reinterpret_cast<const UChar *>(s1.data()), s1.size(),
                            reinterpret_cast<const UChar *>(s2.data()), s2.size());
    }

    return QtPrivate::compareStrings(s1, s2, d->caseSensitivity);
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    d->ensureInitialized();

    if (d->isC())
        return QCollatorSortKey(new QCollatorSortKeyPrivate(string.toUtf8()));

    if (d->collator) {
        QByteArray result(16 + string.size() + (string.size() >> 2), Qt::Uninitialized);
        // truncating sizes (QTBUG-105038)
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
