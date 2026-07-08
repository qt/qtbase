// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qcollator_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

// This backend implements collation on top of the JavaScript Intl.Collator API.
//
// Note: Intl.Collator exposes only compare(); it has no equivalent of ICU's
// sort keys. sortKey() therefore falls back to calling compare() and does
// not provide a performance benefit.
void QCollatorPrivate::init()
{
    cleanup();

    // Build the Intl.Collator options object from the QCollator settings.
    // The Intl.Collator sensitivity option maps to case/accent handling:
    //   "variant" - case- and accent-sensitive (Qt::CaseSensitive)
    //   "accent"  - accent-sensitive, case-insensitive (Qt::CaseInsensitive)
    emscripten::val collatorOptions = emscripten::val::object();
    if (options.testFlag(Opt::NumericSort))
        collatorOptions.set("numeric", true);
    collatorOptions.set("sensitivity",
                        options.testFlag(Opt::CaseInsensitive) ? "accent" : "variant");
    if (options.testFlag(Opt::IgnorePunctuation))
        collatorOptions.set("ignorePunctuation", true);

    // An empty/undefined locale list makes Intl.Collator use the host's
    // default locale; otherwise pass the BCP 47 tag from the QLocale.
    const QString bcp47 = locale.bcp47Name();
    const emscripten::val locales = bcp47.isEmpty()
            ? emscripten::val::undefined()
            : emscripten::val(bcp47.toStdString());

    const emscripten::val intl = emscripten::val::global("Intl");
    collator = new emscripten::val(intl["Collator"].new_(locales, collatorOptions));

    dirty = false;
}

void QCollatorPrivate::cleanup()
{
    delete collator;
    collator = NoCollator;
}

namespace {
// Shared comparison core for QCollator::compare() and QCollatorSortKey::compare().
// An undefined \a collator selects a plain code-unit comparison (used for the C
// locale); otherwise the strings are ordered by the given Intl.Collator. Returns a
// result normalized to the -1/0/+1 range QCollator uses.
int compareImpl(const emscripten::val &collator, QStringView s1, QStringView s2,
                Qt::CaseSensitivity cs)
{
    if (s1.isEmpty())
        return s2.isEmpty() ? 0 : -1;
    if (s2.isEmpty())
        return +1;
    if (collator.isUndefined())
        return s1.compare(s2, cs);

    const int result = collator.call<int>("compare",
            s1.toString().toStdString(), s2.toString().toStdString());
    return result < 0 ? -1 : (result > 0 ? 1 : 0);
}
} // unnamed namespace

int QCollator::compare(QStringView s1, QStringView s2) const
{
    if (!d)
        d = new QCollatorPrivate(QLocale().collation());

    if (d->isC())
        return compareImpl(emscripten::val::undefined(), s1, s2, caseSensitivity());

    d->ensureInitialized();
    return compareImpl(*d->collator, s1, s2, caseSensitivity());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (!d)
        d = new QCollatorPrivate(QLocale().collation());

    // Create a sort key which captures the string and the collator. Note
    // that this does not provide a performance benefit, but does implement
    // correct sorting.
    QCollatorSortKeyWasm key;
    key.string = string;
    if (!d->isC()) {
        d->ensureInitialized();
        key.collator = *d->collator;
    }
    return QCollatorPrivate::sortKeyFromData(std::move(key));
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const noexcept
{
    return compareImpl(d->m_key.collator, d->m_key.string, otherKey.d->m_key.string, Qt::CaseSensitive);
}

QT_END_NAMESPACE
