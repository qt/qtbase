// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qcollator_p.h"
#include "qlocale_p.h"
#include "qstringlist.h"
#include "qstring.h"

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#if QT_CONFIG(icu)
#include <unicode/ures.h>
#endif


#include "qdebug.h"
#include "qlibrary.h"

QT_BEGIN_NAMESPACE

// There are two cases in here, using the "real" ICU (ie. QT_CONFIG(icu)) and
// using Android's flavor of ICU provided on the system (slightly confusingly !QT_CONFIG(icu)).
// The Android case itself consists of two variants as well, new enough versions
// where we can dlopen the native ICU, and a JNI fallback.

#if !QT_CONFIG(icu) && defined(Q_OS_ANDROID)
using ucol_open_t = UCollator* (*)(const char*, UErrorCode*);
using ucol_setAttribute_t = void (*)(UCollator *, UColAttribute, UColAttributeValue, UErrorCode*);
using ucol_close_t = void (*)(UCollator*);
using ucol_strcoll_t = UCollationResult (*)(const UCollator*, const UChar*, int32_t, const UChar*, int32_t);
using ucol_getSortKey_t = int32_t (*)(const UCollator*, const UChar*, int32_t, uint8_t*, int32_t);

struct {
    ucol_open_t open = nullptr;
    ucol_setAttribute_t setAttribute = nullptr;
    ucol_close_t close = nullptr;
    ucol_strcoll_t strcoll = nullptr;
    ucol_getSortKey_t getSortKey = nullptr;
} static s_ucol;

#define ucol_open s_ucol.open
#define ucol_setAttribute s_ucol.setAttribute
#define ucol_close s_ucol.close
#define ucol_strcoll s_ucol.strcoll
#define ucol_getSortKey s_ucol.getSortKey
#endif

void QCollatorPrivate::init()
{
    cleanup();
    if (isC())
        return;

#if !QT_CONFIG(icu) && defined(Q_OS_ANDROID)
    static bool icuLoaded = []() {
        // available on Android API 33 or higher only
        QLibrary icuLib(QStringLiteral("libicu"));
        if (!icuLib.load()) {
            qWarning().nospace() << "ICU loading failed: " << icuLib.errorString()
                << ". Fallback collator will be used instead, not all features will be available.";
            return false;
        }

        s_ucol.open = reinterpret_cast<ucol_open_t>(icuLib.resolve("ucol_open"));
        s_ucol.setAttribute = reinterpret_cast<ucol_setAttribute_t>(icuLib.resolve("ucol_setAttribute"));
        s_ucol.close = reinterpret_cast<ucol_close_t>(icuLib.resolve("ucol_close"));
        s_ucol.strcoll = reinterpret_cast<ucol_strcoll_t>(icuLib.resolve("ucol_strcoll"));
        s_ucol.getSortKey = reinterpret_cast<ucol_getSortKey_t>(icuLib.resolve("ucol_getSortKey"));
        return s_ucol.open && s_ucol.setAttribute && s_ucol.close && s_ucol.strcoll && s_ucol.getSortKey;
    }();

    if (!icuLoaded) {
        // on older Android versions, fall back to ICU Java API
        // that works but has more overhead and offers less control
        int strength = -1;
        if (options.testFlag(Opt::DiacriticInsensitive) && options.testFlag(Opt::CaseInsensitive))
            strength = UCOL_PRIMARY;
        else if (options.testFlag(Opt::CaseInsensitive))
            strength = UCOL_SECONDARY;
        fallbackCollator = QtJniTypes::QtCollator::callStaticMethod<QtJniTypes::Collator>("getCollator", locale.bcp47Name(), strength);
        dirty = false;
        return;
    }
#endif

    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = QLocalePrivate::get(locale)->bcp47Name('_');
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status)) {
        qCWarning(lcQCollator, "Could not create collator: %d", status);
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
    if (options.testFlag(Opt::DiacriticInsensitive)) {
        // UCOL_PRIMARY ignores both diacritics and case
        status = U_ZERO_ERROR;
        ucol_setAttribute(collator, UCOL_STRENGTH, UCOL_PRIMARY, &status);
        if (U_FAILURE(status)) {
            qCWarning(lcQCollator,
                      "ucol_setAttribute: Diacritic and case insensitivity failed: %d", status);
        }

        if (!options.testFlag(Opt::CaseInsensitive)) {
            // Re-add case distinction if CaseInsensitive hasn't been set
            status = U_ZERO_ERROR;
            ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_ON, &status);
            if (U_FAILURE(status)) {
                qCWarning(lcQCollator,
                          "ucol_setAttribute: Diacritic insensitivity with case distinction failed:"
                          " %d", status);
            }
        }
    } else {
        const UColAttributeValue strength
                = options.testFlag(Opt::CaseInsensitive) ? UCOL_SECONDARY : UCOL_DEFAULT_STRENGTH;
        // Case sensitivity setting only
        status = U_ZERO_ERROR;
        ucol_setAttribute(collator, UCOL_STRENGTH, strength, &status);
        if (U_FAILURE(status))
            qCWarning(lcQCollator, "ucol_setAttribute: Case sensitivity failed: %d", status);
    }

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION,
                      options.testFlag(Opt::NumericSort) ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qCWarning(lcQCollator, "ucol_setAttribute: numeric collation failed: %d", status);

    status = U_ZERO_ERROR;
    ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING,
                      options.testFlag(Opt::IgnorePunctuation) ? UCOL_SHIFTED
                                                               : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qCWarning(lcQCollator, "ucol_setAttribute: Alternate handling failed: %d", status);

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

    if (!d)
        d = new QCollatorPrivate(QLocale().collation());

    d->ensureInitialized();

    if (d->collator) {
        // truncating sizes (QTBUG-105038)
        return ucol_strcoll(d->collator,
                            reinterpret_cast<const UChar *>(s1.data()), s1.size(),
                            reinterpret_cast<const UChar *>(s2.data()), s2.size());
    }
#if !QT_CONFIG(icu) && defined(Q_OS_ANDROID)
    else if (d->fallbackCollator.isValid()) {
        return d->fallbackCollator.callMethod<int>("compare", s1, s2);
    }
#endif

    return QtPrivate::compareStrings(s1, s2, caseSensitivity());
}

QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    if (!d)
        d = new QCollatorPrivate(QLocale().collation());

    d->ensureInitialized();

    if (d->isC())
        return QCollatorPrivate::sortKeyFromData(string.toUtf8());

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
        return QCollatorPrivate::sortKeyFromData(std::move(result));
    }
#if !QT_CONFIG(icu) && defined(Q_OS_ANDROID)
    else if (d->fallbackCollator.isValid()) {
        return QCollatorPrivate::sortKeyFromData(QtJniTypes::QtCollator::callStaticMethod<QByteArray>("getCollationKey", d->fallbackCollator, string));
    }
#endif

    return QCollatorPrivate::sortKeyFromData(QByteArray());
}

int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const noexcept
{
    return d->m_key.compare(otherKey.d->m_key);
}

QT_END_NAMESPACE
