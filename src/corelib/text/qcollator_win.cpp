// Copyright (C) 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qcollator_p.h"
#include "qlocale_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <QDebug>

#include <qt_windows.h>
#include <qsysinfo.h>

QT_BEGIN_NAMESPACE

//NOTE: SORT_DIGITSASNUMBERS is available since win7
#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 8
#endif

void QCollatorPrivate::init()
{
    collator = 0;
    if (isC())
        return;

    if (caseSensitivity == Qt::CaseInsensitive)
        collator |= NORM_IGNORECASE;

    // WINE does not support SORT_DIGITSASNUMBERS :-(
    // (and its std::sort() crashes on bad comparisons, QTBUG-74209)
    if (numericMode)
        collator |= SORT_DIGITSASNUMBERS;

    if (ignorePunctuation)
        collator |= NORM_IGNORESYMBOLS;

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
}

int QCollator::compare(QStringView s1, QStringView s2) const
{
    if (!s1.size())
        return s2.size() ? -1 : 0;
    if (!s2.size())
        return +1;

    if (d->isC())
        return s1.compare(s2, d->caseSensitivity);

    d->ensureInitialized();

    //* from Windows documentation *
    // Returns one of the following values if successful. To maintain the C
    // runtime convention of comparing strings, the value 2 can be subtracted
    // from a nonzero return value. Then, the meaning of <0, ==0, and >0 is
    // consistent with the C runtime.
    // [...] The function returns 0 if it does not succeed.
    // https://docs.microsoft.com/en-us/windows/desktop/api/stringapiset/nf-stringapiset-comparestringex#return-value

    const QString locale = d->locale.bcp47Name();
    const int ret = CompareStringEx(reinterpret_cast<const wchar_t *>(locale.constData()),
                                    d->collator,
                                    reinterpret_cast<const wchar_t *>(s1.data()), s1.size(),
                                    reinterpret_cast<const wchar_t *>(s2.data()), s2.size(),
                                    nullptr, nullptr, 0);
    if (Q_LIKELY(ret))
        return ret - 2;

    switch (DWORD error = GetLastError()) {
    case ERROR_INVALID_FLAGS:
        qWarning("Unsupported flags (%d) used in QCollator", int(d->collator));
        break;
    case ERROR_INVALID_PARAMETER:
        qWarning("Invalid parameter for QCollator::compare()");
        break;
    default:
        qErrnoWarning(error, "Failed comparison in QCollator::compare()");
        break;
    }
    // We have no idea what to return, so pretend we think they're equal.
    // At least that way we'll be consistent if we get the same values swapped ...
    return 0;
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (string.isEmpty()) {
        // empty strings sort before everything and LCMapString doesn't
        // like them
        return QCollatorSortKey(nullptr);
    }
    d->ensureInitialized();

    if (d->isC())
        return QCollatorSortKey(new QCollatorSortKeyPrivate(string.toUtf8()));

    const QString localeName = d->locale.bcp47Name();
    auto callLcMapString = [&](LPWSTR lpDestStr, int cchDest) {
        // note: truncating sizes (QTBUG-105038)
        return LCMapStringEx(reinterpret_cast<const wchar_t *>(localeName.constData()),
                             LCMAP_SORTKEY | d->collator,
                             reinterpret_cast<const wchar_t*>(string.constData()), string.size(),
                             lpDestStr, cchDest, nullptr, nullptr, 0);
    };

    int size = callLcMapString(nullptr, 0);
    CollatorKeyType ret(size, Qt::Uninitialized);
    size = callLcMapString(reinterpret_cast<wchar_t*>(ret.data()), ret.size());
    if (size != ret.size())
        ret.truncate(size);
    if (size == 0)
        qErrnoWarning("Error when generating the ::sortKey by LCMapStringEx");

    return QCollatorSortKey(new QCollatorSortKeyPrivate(std::move(ret)));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    if (!d)
        return otherKey.d ? -1 : 0;
    if (!otherKey.d)
        return +1;
    return d->m_key.compare(otherKey.d->m_key);
}

QT_END_NAMESPACE
