// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qlocale_p.h"

#include "qdatetime.h"
#include "qjniobject.h"
#include "qreadwritelock.h"
#include "qstringlist.h"
#include "qvariant.h"

#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE

Q_DECLARE_JNI_CLASS(Locale, "java/util/Locale")
Q_DECLARE_JNI_CLASS(Resources, "android/content/res/Resources")
Q_DECLARE_JNI_CLASS(Configuration, "android/content/res/Configuration")
Q_DECLARE_JNI_CLASS(LocaleList, "android/os/LocaleList")
Q_DECLARE_JNI_CLASS(DateFormat, "java/text/DateFormat")

namespace {

using namespace Qt::StringLiterals;
using namespace QtJniTypes;

struct LocaleInfo
{
    QLocale::Language language = QLocale::AnyLanguage;
    QLocale::Script script = QLocale::AnyScript;
    QLocale::Territory territory = QLocale::AnyTerritory;
    QString timeFormatShort;
    QString timeFormatLong;
    QString dateFormatShort;
    QString dateFormatLong;
};

struct QSystemLocaleData
{
    void readLocaleFromJava();
    LocaleInfo snapshot()
    {
        QReadLocker locker(&lock);
        return localeInfo;
    }

    QReadWriteLock lock;
    LocaleInfo localeInfo;
};

// Translate the Java SimpleDateFormat tokens Qt doesn't accept to Qt's
// (E/EE/EEE → ddd, EEEE → dddd, y/yyy+ → yyyy; a → aP to keep the locale's
// own AM/PM casing rather than force lower-case). Other tokens pass through.
// See https://developer.android.com/reference/java/text/SimpleDateFormat
QString qtPatternFromJava(QStringView pattern)
{
    QString out;
    out.reserve(pattern.size());
    bool inQuoted = false;
    while (!pattern.isEmpty()) {
        const QChar ch = pattern.front();
        qsizetype consumed = 1;
        if (ch == u'\'') {
            inQuoted = !inQuoted;
            out.append(ch);
        } else if (inQuoted) {
            out.append(ch);
        } else if (ch == u'E' || ch == u'y' || ch == u'a') {
            while (consumed < pattern.size() && pattern[consumed] == ch)
                ++consumed;
            if (ch == u'E')
                out.append(consumed >= 4 ? "dddd"_L1 : "ddd"_L1);
            else if (ch == u'y')
                out.append(consumed == 2 ? "yy"_L1 : "yyyy"_L1);
            else // 'a'
                out.append("aP"_L1);
        } else {
            out.append(ch);
        }
        pattern = pattern.sliced(consumed);
    }
    return out;
}

// Qt pattern via java.text.DateFormat.get{Date,Time}Instance(style, locale).
QString patternForStyle(QSystemLocale::QueryType type, const Locale &locale)
{
    // https://docs.oracle.com/javase/8/docs/api/java/text/DateFormat.html
    constexpr jint SHORT = 3;
    constexpr jint MEDIUM = 2;
    constexpr jint FULL = 0;

    const char *factory = nullptr;
    jint style = 0;
    switch (type) {
    case QSystemLocale::DateFormatShort: factory = "getDateInstance"; style = SHORT;  break;
    case QSystemLocale::DateFormatLong:  factory = "getDateInstance"; style = FULL;   break;
    case QSystemLocale::TimeFormatShort: factory = "getTimeInstance"; style = SHORT;  break;
    case QSystemLocale::TimeFormatLong:  factory = "getTimeInstance"; style = MEDIUM; break;
    default: Q_UNREACHABLE(); break;
    }

    const auto dateFormat = DateFormat::callStaticMethod<DateFormat>(factory, style, locale);
    return qtPatternFromJava(dateFormat.callMethod<QString>("toPattern"));
}

void QSystemLocaleData::readLocaleFromJava()
{
    const QJniObject context = QtAndroidPrivate::context();
    const Locale androidLocale = [context]{
        if (context.isValid()) {
            const auto resources = context.callMethod<Resources>("getResources");
            const auto configuration = resources.callMethod<Configuration>("getConfiguration");
            return configuration.getField<Locale>("locale");
        }
        return Locale::callStaticMethod<Locale>("getDefault");
    }();

    LocaleInfo updated;
    updated.language = QLocale::codeToLanguage(androidLocale.callMethod<QString>("getLanguage"));
    updated.script = QLocale::codeToScript(androidLocale.callMethod<QString>("getScript"));
    updated.territory = QLocale::codeToTerritory(androidLocale.callMethod<QString>("getCountry"));
    // QLocale does not yet support variants (QTBUG-81051)
    // updated.variant = androidLocale.callMethod<QString>("getVariant");

    updated.dateFormatShort = patternForStyle(QSystemLocale::DateFormatShort, androidLocale);
    updated.dateFormatLong  = patternForStyle(QSystemLocale::DateFormatLong,  androidLocale);
    updated.timeFormatShort = patternForStyle(QSystemLocale::TimeFormatShort, androidLocale);
    updated.timeFormatLong  = patternForStyle(QSystemLocale::TimeFormatLong,  androidLocale);
    // QTBUG-145605: locale-correct date/time gluing is possible with
    // java.text.DateFormat.getDateTimeInstance(dateStyle, timeStyle, locale).
    // QLocale handles DateTimeFormat{Short,Long} gluing, so they are deliberately left out.

    QWriteLocker locker(&lock);
    localeInfo = updated;
}

Q_GLOBAL_STATIC(QSystemLocaleData, qSystemLocaleData)

} // unnamed namespace

QLocale QSystemLocale::fallbackLocale() const
{
    QSystemLocaleData *d = qSystemLocaleData();
    if (!d)
        return QLocale(QLocale::C);

    const LocaleInfo info = d->snapshot();
    return QLocale(info.language, info.script, info.territory);
}

QVariant QSystemLocale::query(QueryType type, QVariant &&in) const
{
    Q_UNUSED(in);
    QSystemLocaleData *d = qSystemLocaleData();
    if (!d)
        return QVariant();

    if (type == LocaleChanged) {
        d->readLocaleFromJava();
        return QVariant();
    }

    const LocaleInfo info = d->snapshot();

    switch (type) {
    case LanguageId:
        return QVariant::fromValue(static_cast<int>(info.language));
    case TerritoryId:
        return QVariant::fromValue(static_cast<int>(info.territory));
    case ScriptId:
        return QVariant::fromValue(static_cast<int>(info.script));
    case Collation:
        return QLocale(info.language, info.script, info.territory).name();
    case TimeFormatShort:
        return info.timeFormatShort;
    case TimeFormatLong:
        return info.timeFormatLong;
    case DateFormatShort:
        return info.dateFormatShort;
    case DateFormatLong:
        return info.dateFormatLong;
    case UILanguages: {
        const LocaleList localeList = LocaleList::callStaticMethod<LocaleList>("getDefault");
        if (!localeList.isValid())
            return QVariant();
        QString tags = localeList.callMethod<QString>("toLanguageTags");
        // Some devices wrap the list in [] - strip those if present.
        if (tags.startsWith(u'[') && tags.endsWith(u']'))
            tags = tags.mid(1, tags.length() - 2);
        return tags.split(u',');
    }
    case LocaleChanged:
        Q_UNREACHABLE(); // handled before the switch
    default:
        break;
    }
    return QVariant();
}

#endif // QT_NO_SYSTEMLOCALE

QT_END_NAMESPACE
