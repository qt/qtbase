/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qglobal.h"

#if !defined(QWS) && defined(Q_OS_MAC)
#   include "private/qcore_mac_p.h"
#   include <CoreFoundation/CoreFoundation.h>
#endif

#include "qplatformdefs.h"

#include "qdatastream.h"
#include "qdebug.h"
#include "qhashfunctions.h"
#include "qstring.h"
#include "qlocale.h"
#include "qlocale_p.h"
#include "qlocale_tools_p.h"
#if QT_CONFIG(datetimeparser)
#include "qdatetimeparser_p.h"
#endif
#include "qnamespace.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"
#include "qstringbuilder.h"
#include "private/qnumeric_p.h"
#include <cmath>
#ifndef QT_NO_SYSTEMLOCALE
#   include "qmutex.h"
#endif
#ifdef Q_OS_WIN
#   include <qt_windows.h>
#   include <time.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE
static QSystemLocale *_systemLocale = 0;
class QSystemLocaleSingleton: public QSystemLocale
{
public:
    QSystemLocaleSingleton() : QSystemLocale(true) {}
};

Q_GLOBAL_STATIC(QSystemLocaleSingleton, QSystemLocale_globalSystemLocale)
static QLocaleData *system_data = 0;
static QLocaleData globalLocaleData;
#endif

/******************************************************************************
** Helpers for accessing Qt locale database
*/

QT_BEGIN_INCLUDE_NAMESPACE
#include "qlocale_data_p.h"
QT_END_INCLUDE_NAMESPACE

QLocale::Language QLocalePrivate::codeToLanguage(QStringView code) Q_DECL_NOTHROW
{
    const auto len = code.size();
    if (len != 2 && len != 3)
        return QLocale::C;
    ushort uc1 = code[0].toLower().unicode();
    ushort uc2 = code[1].toLower().unicode();
    ushort uc3 = len > 2 ? code[2].toLower().unicode() : 0;

    const unsigned char *c = language_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Language((c - language_code_list)/3);
    }

    if (uc3 == 0) {
        // legacy codes
        if (uc1 == 'n' && uc2 == 'o') { // no -> nb
            Q_STATIC_ASSERT(QLocale::Norwegian == QLocale::NorwegianBokmal);
            return QLocale::Norwegian;
        }
        if (uc1 == 't' && uc2 == 'l') { // tl -> fil
            Q_STATIC_ASSERT(QLocale::Tagalog == QLocale::Filipino);
            return QLocale::Tagalog;
        }
        if (uc1 == 's' && uc2 == 'h') { // sh -> sr[_Latn]
            Q_STATIC_ASSERT(QLocale::SerboCroatian == QLocale::Serbian);
            return QLocale::SerboCroatian;
        }
        if (uc1 == 'm' && uc2 == 'o') { // mo -> ro
            Q_STATIC_ASSERT(QLocale::Moldavian == QLocale::Romanian);
            return QLocale::Moldavian;
        }
        // Android uses the following deprecated codes
        if (uc1 == 'i' && uc2 == 'w') // iw -> he
            return QLocale::Hebrew;
        if (uc1 == 'i' && uc2 == 'n') // in -> id
            return QLocale::Indonesian;
        if (uc1 == 'j' && uc2 == 'i') // ji -> yi
            return QLocale::Yiddish;
    }
    return QLocale::C;
}

QLocale::Script QLocalePrivate::codeToScript(QStringView code) Q_DECL_NOTHROW
{
    const auto len = code.size();
    if (len != 4)
        return QLocale::AnyScript;

    // script is titlecased in our data
    unsigned char c0 = code[0].toUpper().toLatin1();
    unsigned char c1 = code[1].toLower().toLatin1();
    unsigned char c2 = code[2].toLower().toLatin1();
    unsigned char c3 = code[3].toLower().toLatin1();

    const unsigned char *c = script_code_list;
    for (int i = 0; i < QLocale::LastScript; ++i, c += 4) {
        if (c0 == c[0] && c1 == c[1] && c2 == c[2] && c3 == c[3])
            return QLocale::Script(i);
    }
    return QLocale::AnyScript;
}

QLocale::Country QLocalePrivate::codeToCountry(QStringView code) Q_DECL_NOTHROW
{
    const auto len = code.size();
    if (len != 2 && len != 3)
        return QLocale::AnyCountry;

    ushort uc1 = code[0].toUpper().unicode();
    ushort uc2 = code[1].toUpper().unicode();
    ushort uc3 = len > 2 ? code[2].toUpper().unicode() : 0;

    const unsigned char *c = country_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Country((c - country_code_list)/3);
    }

    return QLocale::AnyCountry;
}

QLatin1String QLocalePrivate::languageToCode(QLocale::Language language)
{
    if (language == QLocale::AnyLanguage)
        return QLatin1String();
    if (language == QLocale::C)
        return QLatin1String("C");

    const unsigned char *c = language_code_list + 3*(uint(language));

    return QLatin1String(reinterpret_cast<const char*>(c), c[2] == 0 ? 2 : 3);

}

QLatin1String QLocalePrivate::scriptToCode(QLocale::Script script)
{
    if (script == QLocale::AnyScript || script > QLocale::LastScript)
        return QLatin1String();
    const unsigned char *c = script_code_list + 4*(uint(script));
    return QLatin1String(reinterpret_cast<const char *>(c), 4);
}

QLatin1String QLocalePrivate::countryToCode(QLocale::Country country)
{
    if (country == QLocale::AnyCountry)
        return QLatin1String();

    const unsigned char *c = country_code_list + 3*(uint(country));

    return QLatin1String(reinterpret_cast<const char*>(c), c[2] == 0 ? 2 : 3);
}

// http://www.unicode.org/reports/tr35/#Likely_Subtags
static bool addLikelySubtags(QLocaleId &localeId)
{
    // ### optimize with bsearch
    const int likely_subtags_count = sizeof(likely_subtags) / sizeof(likely_subtags[0]);
    const QLocaleId *p = likely_subtags;
    const QLocaleId *const e = p + likely_subtags_count;
    for ( ; p < e; p += 2) {
        if (localeId == p[0]) {
            localeId = p[1];
            return true;
        }
    }
    return false;
}

QLocaleId QLocaleId::withLikelySubtagsAdded() const
{
    // language_script_region
    if (language_id || script_id || country_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, script_id, country_id);
        if (addLikelySubtags(id))
            return id;
    }
    // language_script
    if (country_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, script_id, 0);
        if (addLikelySubtags(id)) {
            id.country_id = country_id;
            return id;
        }
    }
    // language_region
    if (script_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, 0, country_id);
        if (addLikelySubtags(id)) {
            id.script_id = script_id;
            return id;
        }
    }
    // language
    if (script_id && country_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, 0, 0);
        if (addLikelySubtags(id)) {
            id.script_id = script_id;
            id.country_id = country_id;
            return id;
        }
    }
    return *this;
}

QLocaleId QLocaleId::withLikelySubtagsRemoved() const
{
    QLocaleId max = withLikelySubtagsAdded();
    // language
    {
        QLocaleId id = QLocaleId::fromIds(language_id, 0, 0);
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    // language_region
    if (country_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, 0, country_id);
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    // language_script
    if (script_id) {
        QLocaleId id = QLocaleId::fromIds(language_id, script_id, 0);
        if (id.withLikelySubtagsAdded() == max)
            return id;
    }
    return max;
}

QByteArray QLocaleId::name(char separator) const
{
    if (language_id == QLocale::AnyLanguage)
        return QByteArray();
    if (language_id == QLocale::C)
        return QByteArrayLiteral("C");

    const unsigned char *lang = language_code_list + 3 * language_id;
    const unsigned char *script =
            (script_id != QLocale::AnyScript ? script_code_list + 4 * script_id : 0);
    const unsigned char *country =
            (country_id != QLocale::AnyCountry ? country_code_list + 3 * country_id : 0);
    char len = (lang[2] != 0 ? 3 : 2) + (script ? 4+1 : 0) + (country ? (country[2] != 0 ? 3 : 2)+1 : 0);
    QByteArray name(len, Qt::Uninitialized);
    char *uc = name.data();
    *uc++ = lang[0];
    *uc++ = lang[1];
    if (lang[2] != 0)
        *uc++ = lang[2];
    if (script) {
        *uc++ = separator;
        *uc++ = script[0];
        *uc++ = script[1];
        *uc++ = script[2];
        *uc++ = script[3];
    }
    if (country) {
        *uc++ = separator;
        *uc++ = country[0];
        *uc++ = country[1];
        if (country[2] != 0)
            *uc++ = country[2];
    }
    return name;
}

QByteArray QLocalePrivate::bcp47Name(char separator) const
{
    if (m_data->m_language_id == QLocale::AnyLanguage)
        return QByteArray();
    if (m_data->m_language_id == QLocale::C)
        return QByteArrayLiteral("en");

    QLocaleId localeId = QLocaleId::fromIds(m_data->m_language_id, m_data->m_script_id, m_data->m_country_id);
    return localeId.withLikelySubtagsRemoved().name(separator);
}

static const QLocaleData *findLocaleDataById(const QLocaleId &localeId)
{
    const uint idx = locale_index[localeId.language_id];

    const QLocaleData *data = locale_data + idx;

    if (idx == 0) // default language has no associated script or country
        return data;

    Q_ASSERT(data->m_language_id == localeId.language_id);

    if (localeId.script_id == QLocale::AnyScript && localeId.country_id == QLocale::AnyCountry)
        return data;

    if (localeId.script_id == QLocale::AnyScript) {
        do {
            if (data->m_country_id == localeId.country_id)
                return data;
            ++data;
        } while (data->m_language_id && data->m_language_id == localeId.language_id);
    } else if (localeId.country_id == QLocale::AnyCountry) {
        do {
            if (data->m_script_id == localeId.script_id)
                return data;
            ++data;
        } while (data->m_language_id && data->m_language_id == localeId.language_id);
    } else {
        do {
            if (data->m_script_id == localeId.script_id && data->m_country_id == localeId.country_id)
                return data;
            ++data;
        } while (data->m_language_id && data->m_language_id == localeId.language_id);
    }

    return 0;
}

const QLocaleData *QLocaleData::findLocaleData(QLocale::Language language, QLocale::Script script, QLocale::Country country)
{
    QLocaleId localeId = QLocaleId::fromIds(language, script, country);
    QLocaleId likelyId = localeId.withLikelySubtagsAdded();

    const uint idx = locale_index[likelyId.language_id];

    // Try a straight match with the likely data:
    if (const QLocaleData *const data = findLocaleDataById(likelyId))
        return data;
    QList<QLocaleId> tried;
    tried.push_back(likelyId);

    // No match; try again with likely country
    if (country != QLocale::AnyCountry
        && (language != QLocale::AnyLanguage || script != QLocale::AnyScript)) {
        localeId = QLocaleId::fromIds(language, script, QLocale::AnyCountry);
        likelyId = localeId.withLikelySubtagsAdded();
        if (!tried.contains(likelyId)) {
            if (const QLocaleData *const data = findLocaleDataById(likelyId))
                return data;
            tried.push_back(likelyId);
        }

        // No match; try again with any country
        if (!tried.contains(localeId)) {
            if (const QLocaleData *const data = findLocaleDataById(localeId))
                return data;
            tried.push_back(localeId);
        }
    }

    // No match; try again with likely script
    if (script != QLocale::AnyScript
        && (language != QLocale::AnyLanguage || country != QLocale::AnyCountry)) {
        localeId = QLocaleId::fromIds(language, QLocale::AnyScript, country);
        likelyId = localeId.withLikelySubtagsAdded();
        if (!tried.contains(likelyId)) {
            if (const QLocaleData *const data = findLocaleDataById(likelyId))
                return data;
            tried.push_back(likelyId);
        }

        // No match; try again with any script
        if (!tried.contains(localeId)) {
            if (const QLocaleData *const data = findLocaleDataById(localeId))
                return data;
            tried.push_back(localeId);
        }
    }

    // No match; return data at original index
    return locale_data + idx;
}

static bool parse_locale_tag(const QString &input, int &i, QString *result, const QString &separators)
{
    *result = QString(8, Qt::Uninitialized); // worst case according to BCP47
    QChar *pch = result->data();
    const QChar *uc = input.data() + i;
    const int l = input.length();
    int size = 0;
    for (; i < l && size < 8; ++i, ++size) {
        if (separators.contains(*uc))
            break;
        if (! ((uc->unicode() >= 'a' && uc->unicode() <= 'z') ||
               (uc->unicode() >= 'A' && uc->unicode() <= 'Z') ||
               (uc->unicode() >= '0' && uc->unicode() <= '9')) ) // latin only
            return false;
        *pch++ = *uc++;
    }
    result->truncate(size);
    return true;
}

bool qt_splitLocaleName(const QString &name, QString &lang, QString &script, QString &cntry)
{
    const int length = name.length();

    lang = script = cntry = QString();

    const QString separators = QStringLiteral("_-.@");
    enum ParserState { NoState, LangState, ScriptState, CountryState };
    ParserState state = LangState;
    for (int i = 0; i < length && state != NoState; ) {
        QString value;
        if (!parse_locale_tag(name, i, &value, separators) ||value.isEmpty())
            break;
        QChar sep = i < length ? name.at(i) : QChar();
        switch (state) {
        case LangState:
            if (!sep.isNull() && !separators.contains(sep)) {
                state = NoState;
                break;
            }
            lang = value;
            if (i == length) {
                // just language was specified
                state = NoState;
                break;
            }
            state = ScriptState;
            break;
        case ScriptState: {
            QString scripts = QString::fromLatin1((const char *)script_code_list, sizeof(script_code_list) - 1);
            if (value.length() == 4 && scripts.indexOf(value) % 4 == 0) {
                // script name is always 4 characters
                script = value;
                state = CountryState;
            } else {
                // it wasn't a script, maybe it is a country then?
                cntry = value;
                state = NoState;
            }
            break;
        }
        case CountryState:
            cntry = value;
            state = NoState;
            break;
        case NoState:
            // shouldn't happen
            qWarning("QLocale: This should never happen");
            break;
        }
        ++i;
    }
    return lang.length() == 2 || lang.length() == 3;
}

void QLocalePrivate::getLangAndCountry(const QString &name, QLocale::Language &lang,
                                       QLocale::Script &script, QLocale::Country &cntry)
{
    lang = QLocale::C;
    script = QLocale::AnyScript;
    cntry = QLocale::AnyCountry;

    QString lang_code;
    QString script_code;
    QString cntry_code;
    if (!qt_splitLocaleName(name, lang_code, script_code, cntry_code))
        return;

    lang = QLocalePrivate::codeToLanguage(lang_code);
    if (lang == QLocale::C)
        return;
    script = QLocalePrivate::codeToScript(script_code);
    cntry = QLocalePrivate::codeToCountry(cntry_code);
}

static const QLocaleData *findLocaleData(const QString &name)
{
    QLocale::Language lang;
    QLocale::Script script;
    QLocale::Country cntry;
    QLocalePrivate::getLangAndCountry(name, lang, script, cntry);

    return QLocaleData::findLocaleData(lang, script, cntry);
}

QString qt_readEscapedFormatString(QStringView format, int *idx)
{
    int &i = *idx;

    Q_ASSERT(format.at(i) == QLatin1Char('\''));
    ++i;
    if (i == format.size())
        return QString();
    if (format.at(i).unicode() == '\'') { // "''" outside of a quoted stirng
        ++i;
        return QLatin1String("'");
    }

    QString result;

    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            if (i + 1 < format.size() && format.at(i + 1).unicode() == '\'') {
                // "''" inside of a quoted string
                result.append(QLatin1Char('\''));
                i += 2;
            } else {
                break;
            }
        } else {
            result.append(format.at(i++));
        }
    }
    if (i < format.size())
        ++i;

    return result;
}

/*!
    \internal

    Counts the number of identical leading characters in \a s.

    If \a s is empty, returns 0.

    Otherwise, returns the number of consecutive \c{s.front()}
    characters at the start of \a s.

    \code
    qt_repeatCount(u"a");   // == 1
    qt_repeatCount(u"ab");  // == 1
    qt_repeatCount(u"aab"); // == 2
    \endcode
*/
int qt_repeatCount(QStringView s)
{
    if (s.isEmpty())
        return 0;
    const QChar c = s.front();
    qsizetype j = 1;
    while (j < s.size() && s.at(j) == c)
        ++j;
    return int(j);
}

static const QLocaleData *default_data = 0;
static QLocale::NumberOptions default_number_options = QLocale::DefaultNumberOptions;

static const QLocaleData *const c_data = locale_data;
static QLocalePrivate *c_private()
{
    static QLocalePrivate c_locale = { c_data, Q_BASIC_ATOMIC_INITIALIZER(1), QLocale::OmitGroupSeparator };
    return &c_locale;
}

#ifndef QT_NO_SYSTEMLOCALE
/******************************************************************************
** Default system locale behavior
*/

/*!
  Constructs a QSystemLocale object. The constructor will automatically
  install this object as the system locale and remove any earlier installed
  system locales.
*/
QSystemLocale::QSystemLocale()
{
    _systemLocale = this;

    if (system_data)
        system_data->m_language_id = 0;
}

/*!
    \internal
*/
QSystemLocale::QSystemLocale(bool)
{ }

/*!
  Deletes the object.
*/
QSystemLocale::~QSystemLocale()
{
    if (_systemLocale == this) {
        _systemLocale = 0;

        if (system_data)
            system_data->m_language_id = 0;
    }
}

static const QSystemLocale *systemLocale()
{
    if (_systemLocale)
        return _systemLocale;
    return QSystemLocale_globalSystemLocale();
}

void QLocalePrivate::updateSystemPrivate()
{
    const QSystemLocale *sys_locale = systemLocale();
    if (!system_data)
        system_data = &globalLocaleData;

    // tell the object that the system locale has changed.
    sys_locale->query(QSystemLocale::LocaleChanged, QVariant());

    *system_data = *sys_locale->fallbackUiLocale().d->m_data;

    QVariant res = sys_locale->query(QSystemLocale::LanguageId, QVariant());
    if (!res.isNull()) {
        system_data->m_language_id = res.toInt();
        system_data->m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::CountryId, QVariant());
    if (!res.isNull()) {
        system_data->m_country_id = res.toInt();
        system_data->m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::ScriptId, QVariant());
    if (!res.isNull())
        system_data->m_script_id = res.toInt();

    res = sys_locale->query(QSystemLocale::DecimalPoint, QVariant());
    if (!res.isNull())
        system_data->m_decimal = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::GroupSeparator, QVariant());
    if (!res.isNull())
        system_data->m_group = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::ZeroDigit, QVariant());
    if (!res.isNull())
        system_data->m_zero = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::NegativeSign, QVariant());
    if (!res.isNull())
        system_data->m_minus = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::PositiveSign, QVariant());
    if (!res.isNull())
        system_data->m_plus = res.toString().at(0).unicode();
}
#endif // !QT_NO_SYSTEMLOCALE

static const QLocaleData *systemData()
{
#ifndef QT_NO_SYSTEMLOCALE
    /*
      Copy over the information from the fallback locale and modify.

      This modifies (cross-thread) global state, so take care to only call it in
      one thread.
    */
    {
        static QBasicMutex systemDataMutex;
        systemDataMutex.lock();
        if (!system_data || system_data->m_language_id == 0)
            QLocalePrivate::updateSystemPrivate();
        systemDataMutex.unlock();
    }

    return system_data;
#else
    return locale_data;
#endif
}

static const QLocaleData *defaultData()
{
    if (!default_data)
        default_data = systemData();
    return default_data;
}

const QLocaleData *QLocaleData::c()
{
    Q_ASSERT(locale_index[QLocale::C] == 0);
    return c_data;
}

static inline QString getLocaleData(const ushort *data, int size)
{
    return size > 0 ? QString::fromRawData(reinterpret_cast<const QChar *>(data), size) : QString();
}

static QString getLocaleListData(const ushort *data, int size, int index)
{
    static const ushort separator = ';';
    while (index && size > 0) {
        while (*data != separator)
            ++data, --size;
        --index;
        ++data;
        --size;
    }
    const ushort *end = data;
    while (size > 0 && *end != separator)
        ++end, --size;
    return getLocaleData(data, end - data);
}


#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &ds, const QLocale &l)
{
    ds << l.name();
    return ds;
}

QDataStream &operator>>(QDataStream &ds, QLocale &l)
{
    QString s;
    ds >> s;
    l = QLocale(s);
    return ds;
}
#endif // QT_NO_DATASTREAM


static const int locale_data_size = sizeof(locale_data)/sizeof(QLocaleData) - 1;

Q_GLOBAL_STATIC_WITH_ARGS(QSharedDataPointer<QLocalePrivate>, defaultLocalePrivate,
                          (QLocalePrivate::create(defaultData(), default_number_options)))

static QLocalePrivate *localePrivateByName(const QString &name)
{
    if (name == QLatin1String("C"))
        return c_private();
    const QLocaleData *data = findLocaleData(name);
    return QLocalePrivate::create(data, data->m_language_id == QLocale::C ?
                                      QLocale::OmitGroupSeparator : QLocale::DefaultNumberOptions);
}

static QLocalePrivate *findLocalePrivate(QLocale::Language language, QLocale::Script script,
                                         QLocale::Country country)
{
    if (language == QLocale::C)
        return c_private();

    const QLocaleData *data = QLocaleData::findLocaleData(language, script, country);

    QLocale::NumberOptions numberOptions = QLocale::DefaultNumberOptions;

    // If not found, should default to system
    if (data->m_language_id == QLocale::C && language != QLocale::C) {
        numberOptions = default_number_options;
        data = defaultData();
    }
    return QLocalePrivate::create(data, numberOptions);
}


/*!
 \internal
*/
QLocale::QLocale(QLocalePrivate &dd)
    : d(&dd)
{}


/*!
    Constructs a QLocale object with the specified \a name,
    which has the format
    "language[_script][_country][.codeset][@modifier]" or "C", where:

    \list
    \li language is a lowercase, two-letter, ISO 639 language code (also some three-letter codes),
    \li script is a titlecase, four-letter, ISO 15924 script code,
    \li country is an uppercase, two-letter, ISO 3166 country code (also "419" as defined by United Nations),
    \li and codeset and modifier are ignored.
    \endlist

    The separator can be either underscore or a minus sign.

    If the string violates the locale format, or language is not
    a valid ISO 639 code, the "C" locale is used instead. If country
    is not present, or is not a valid ISO 3166 code, the most
    appropriate country is chosen for the specified language.

    The language, script and country codes are converted to their respective
    \c Language, \c Script and \c Country enums. After this conversion is
    performed, the constructor behaves exactly like QLocale(Country, Script,
    Language).

    This constructor is much slower than QLocale(Country, Script, Language).

    \sa bcp47Name()
*/

QLocale::QLocale(const QString &name)
    : d(localePrivateByName(name))
{
}

/*!
    Constructs a QLocale object initialized with the default locale. If
    no default locale was set using setDefault(), this locale will
    be the same as the one returned by system().

    \sa setDefault()
*/

QLocale::QLocale()
    : d(*defaultLocalePrivate)
{
    // Make sure system data is up to date
    systemData();
}

/*!
    Constructs a QLocale object with the specified \a language and \a
    country.

    \list
    \li If the language/country pair is found in the database, it is used.
    \li If the language is found but the country is not, or if the country
       is \c AnyCountry, the language is used with the most
       appropriate available country (for example, Germany for German),
    \li If neither the language nor the country are found, QLocale
       defaults to the default locale (see setDefault()).
    \endlist

    The language and country that are actually used can be queried
    using language() and country().

    \sa setDefault(), language(), country()
*/

QLocale::QLocale(Language language, Country country)
    : d(findLocalePrivate(language, QLocale::AnyScript, country))
{
}

/*!
    \since 4.8

    Constructs a QLocale object with the specified \a language, \a script and
    \a country.

    \list
    \li If the language/script/country is found in the database, it is used.
    \li If both \a script is AnyScript and \a country is AnyCountry, the
       language is used with the most appropriate available script and country
       (for example, Germany for German),
    \li If either \a script is AnyScript or \a country is AnyCountry, the
       language is used with the first locale that matches the given \a script
       and \a country.
    \li If neither the language nor the country are found, QLocale
       defaults to the default locale (see setDefault()).
    \endlist

    The language, script and country that are actually used can be queried
    using language(), script() and country().

    \sa setDefault(), language(), script(), country()
*/

QLocale::QLocale(Language language, Script script, Country country)
    : d(findLocalePrivate(language, script, country))
{
}

/*!
    Constructs a QLocale object as a copy of \a other.
*/

QLocale::QLocale(const QLocale &other)
{
    d = other.d;
}

/*!
    Destructor
*/

QLocale::~QLocale()
{
}

/*!
    Assigns \a other to this QLocale object and returns a reference
    to this QLocale object.
*/

QLocale &QLocale::operator=(const QLocale &other)
{
    d = other.d;
    return *this;
}

bool QLocale::operator==(const QLocale &other) const
{
    return d->m_data == other.d->m_data && d->m_numberOptions == other.d->m_numberOptions;
}

bool QLocale::operator!=(const QLocale &other) const
{
    return d->m_data != other.d->m_data || d->m_numberOptions != other.d->m_numberOptions;
}

/*!
    \fn void QLocale::swap(QLocale &other)
    \since 5.6

    Swaps locale \a other with this locale. This operation is very fast and
    never fails.
*/

/*!
    \since 5.6
    \relates QLocale

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/
uint qHash(const QLocale &key, uint seed) Q_DECL_NOTHROW
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.d->m_data);
    seed = hash(seed, key.d->m_numberOptions);
    return seed;
}

/*!
    \since 4.2

    Sets the \a options related to number conversions for this
    QLocale instance.
*/
void QLocale::setNumberOptions(NumberOptions options)
{
    d->m_numberOptions = options;
}

/*!
    \since 4.2

    Returns the options related to number conversions for this
    QLocale instance.

    By default, no options are set for the standard locales.
*/
QLocale::NumberOptions QLocale::numberOptions() const
{
    return static_cast<NumberOptions>(d->m_numberOptions);
}

/*!
    \since 4.8

    Returns \a str quoted according to the current locale using the given
    quotation \a style.
*/
QString QLocale::quoteString(const QString &str, QuotationStyle style) const
{
    return quoteString(QStringRef(&str), style);
}

/*!
    \since 4.8

    \overload
*/
QString QLocale::quoteString(const QStringRef &str, QuotationStyle style) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res;
        if (style == QLocale::AlternateQuotation)
            res = systemLocale()->query(QSystemLocale::StringToAlternateQuotation, QVariant::fromValue(str));
        if (res.isNull() || style == QLocale::StandardQuotation)
            res = systemLocale()->query(QSystemLocale::StringToStandardQuotation, QVariant::fromValue(str));
        if (!res.isNull())
            return res.toString();
    }
#endif

    if (style == QLocale::StandardQuotation)
        return QChar(d->m_data->m_quotation_start) % str % QChar(d->m_data->m_quotation_end);
    else
        return QChar(d->m_data->m_alternate_quotation_start) % str % QChar(d->m_data->m_alternate_quotation_end);
}

/*!
    \since 4.8

    Returns a string that represents a join of a given \a list of strings with
    a separator defined by the locale.
*/
QString QLocale::createSeparatedList(const QStringList &list) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res;
        res = systemLocale()->query(QSystemLocale::ListToSeparatedString, QVariant::fromValue(list));

        if (!res.isNull())
            return res.toString();
    }
#endif

    const int size = list.size();
    if (size == 1) {
        return list.at(0);
    } else if (size == 2) {
        QString format = getLocaleData(list_pattern_part_data + d->m_data->m_list_pattern_part_two_idx, d->m_data->m_list_pattern_part_two_size);
        return format.arg(list.at(0), list.at(1));
    } else if (size > 2) {
        QString formatStart = getLocaleData(list_pattern_part_data + d->m_data->m_list_pattern_part_start_idx, d->m_data->m_list_pattern_part_start_size);
        QString formatMid = getLocaleData(list_pattern_part_data + d->m_data->m_list_pattern_part_mid_idx, d->m_data->m_list_pattern_part_mid_size);
        QString formatEnd = getLocaleData(list_pattern_part_data + d->m_data->m_list_pattern_part_end_idx, d->m_data->m_list_pattern_part_end_size);
        QString result = formatStart.arg(list.at(0), list.at(1));
        for (int i = 2; i < size - 1; ++i)
            result = formatMid.arg(result, list.at(i));
        result = formatEnd.arg(result, list.at(size - 1));
        return result;
    }

    return QString();
}

/*!
    \nonreentrant

    Sets the global default locale to \a locale. These
    values are used when a QLocale object is constructed with
    no arguments. If this function is not called, the system's
    locale is used.

    \warning In a multithreaded application, the default locale
    should be set at application startup, before any non-GUI threads
    are created.

    \sa system(), c()
*/

void QLocale::setDefault(const QLocale &locale)
{
    default_data = locale.d->m_data;
    default_number_options = locale.numberOptions();

    if (defaultLocalePrivate.exists()) {
        // update the cached private
        *defaultLocalePrivate = locale.d;
    }
}

/*!
    Returns the language of this locale.

    \sa script(), country(), languageToString(), bcp47Name()
*/
QLocale::Language QLocale::language() const
{
    return Language(d->languageId());
}

/*!
    \since 4.8

    Returns the script of this locale.

    \sa language(), country(), languageToString(), scriptToString(), bcp47Name()
*/
QLocale::Script QLocale::script() const
{
    return Script(d->m_data->m_script_id);
}

/*!
    Returns the country of this locale.

    \sa language(), script(), countryToString(), bcp47Name()
*/
QLocale::Country QLocale::country() const
{
    return Country(d->countryId());
}

/*!
    Returns the language and country of this locale as a
    string of the form "language_country", where
    language is a lowercase, two-letter ISO 639 language code,
    and country is an uppercase, two- or three-letter ISO 3166 country code.

    Note that even if QLocale object was constructed with an explicit script,
    name() will not contain it for compatibility reasons. Use bcp47Name() instead
    if you need a full locale name.

    \sa QLocale(), language(), script(), country(), bcp47Name()
*/

QString QLocale::name() const
{
    Language l = language();
    if (l == C)
        return d->languageCode();

    Country c = country();
    if (c == AnyCountry)
        return d->languageCode();

    return d->languageCode() + QLatin1Char('_') + d->countryCode();
}

static qlonglong toIntegral_helper(const QLocaleData *d, QStringView str, bool *ok,
                                   QLocale::NumberOptions mode, qlonglong)
{
    return d->stringToLongLong(str, 10, ok, mode);
}

static qulonglong toIntegral_helper(const QLocaleData *d, QStringView str, bool *ok,
                                    QLocale::NumberOptions mode, qulonglong)
{
    return d->stringToUnsLongLong(str, 10, ok, mode);
}

template <typename T> static inline
T toIntegral_helper(const QLocalePrivate *d, QStringView str, bool *ok)
{
    // ### Qt6: use std::conditional<std::is_unsigned<T>::value, qulonglong, qlonglong>::type
    const bool isUnsigned = T(0) < T(-1);
    typedef typename QtPrivate::QConditional<isUnsigned, qulonglong, qlonglong>::Type Int64;

    // we select the right overload by the last, unused parameter
    Int64 val = toIntegral_helper(d->m_data, str, ok, d->m_numberOptions, Int64());
    if (T(val) != val) {
        if (ok)
            *ok = false;
        val = 0;
    }
    return T(val);
}


/*!
    \since 4.8

    Returns the dash-separated language, script and country (and possibly other BCP47 fields)
    of this locale as a string.

    Unlike the uiLanguages() the returned value of the bcp47Name() represents
    the locale name of the QLocale data but not the language the user-interface
    should be in.

    This function tries to conform the locale name to BCP47.

    \sa language(), country(), script(), uiLanguages()
*/
QString QLocale::bcp47Name() const
{
    return QString::fromLatin1(d->bcp47Name());
}

/*!
    Returns a QString containing the name of \a language.

    \sa countryToString(), scriptToString(), bcp47Name()
*/

QString QLocale::languageToString(Language language)
{
    if (uint(language) > uint(QLocale::LastLanguage))
        return QLatin1String("Unknown");
    return QLatin1String(language_name_list + language_name_index[language]);
}

/*!
    Returns a QString containing the name of \a country.

    \sa languageToString(), scriptToString(), country(), bcp47Name()
*/

QString QLocale::countryToString(Country country)
{
    if (uint(country) > uint(QLocale::LastCountry))
        return QLatin1String("Unknown");
    return QLatin1String(country_name_list + country_name_index[country]);
}

/*!
    \since 4.8

    Returns a QString containing the name of \a script.

    \sa languageToString(), countryToString(), script(), bcp47Name()
*/
QString QLocale::scriptToString(QLocale::Script script)
{
    if (uint(script) > uint(QLocale::LastScript))
        return QLatin1String("Unknown");
    return QLatin1String(script_name_list + script_name_index[script]);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    Returns the short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()
*/

short QLocale::toShort(const QString &s, bool *ok) const
{
    return toIntegral_helper<short>(d, s, ok);
}

/*!
    Returns the unsigned short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()
*/

ushort QLocale::toUShort(const QString &s, bool *ok) const
{
    return toIntegral_helper<ushort>(d, s, ok);
}

/*!
    Returns the int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()
*/

int QLocale::toInt(const QString &s, bool *ok) const
{
    return toIntegral_helper<int>(d, s, ok);
}

/*!
    Returns the unsigned int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()
*/

uint QLocale::toUInt(const QString &s, bool *ok) const
{
    return toIntegral_helper<uint>(d, s, ok);
}

/*!
    Returns the long long int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()
*/


qlonglong QLocale::toLongLong(const QString &s, bool *ok) const
{
    return toIntegral_helper<qlonglong>(d, s, ok);
}

/*!
    Returns the unsigned long long int represented by the localized
    string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()
*/

qulonglong QLocale::toULongLong(const QString &s, bool *ok) const
{
    return toIntegral_helper<qulonglong>(d, s, ok);
}

/*!
    Returns the float represented by the localized string \a s, or 0.0
    if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()
*/

float QLocale::toFloat(const QString &s, bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(s, ok), ok);
}

/*!
    Returns the double represented by the localized string \a s, or
    0.0 if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    \snippet code/src_corelib_tools_qlocale.cpp 3

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()
*/

double QLocale::toDouble(const QString &s, bool *ok) const
{
    return d->m_data->stringToDouble(s, ok, d->m_numberOptions);
}

/*!
    Returns the short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()

    \since 5.1
*/

short QLocale::toShort(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<short>(d, s, ok);
}

/*!
    Returns the unsigned short int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()

    \since 5.1
*/

ushort QLocale::toUShort(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<ushort>(d, s, ok);
}

/*!
    Returns the int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()

    \since 5.1
*/

int QLocale::toInt(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<int>(d, s, ok);
}

/*!
    Returns the unsigned int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()

    \since 5.1
*/

uint QLocale::toUInt(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<uint>(d, s, ok);
}

/*!
    Returns the long long int represented by the localized string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()

    \since 5.1
*/


qlonglong QLocale::toLongLong(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<qlonglong>(d, s, ok);
}

/*!
    Returns the unsigned long long int represented by the localized
    string \a s.

    If the conversion fails the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()

    \since 5.1
*/

qulonglong QLocale::toULongLong(const QStringRef &s, bool *ok) const
{
    return toIntegral_helper<qulonglong>(d, s, ok);
}

/*!
    Returns the float represented by the localized string \a s, or 0.0
    if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()

    \since 5.1
*/

float QLocale::toFloat(const QStringRef &s, bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(s, ok), ok);
}

/*!
    Returns the double represented by the localized string \a s, or
    0.0 if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function does not fall back to the 'C' locale if the string
    cannot be interpreted in this locale.

    \snippet code/src_corelib_tools_qlocale.cpp 3

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()

    \since 5.1
*/

double QLocale::toDouble(const QStringRef &s, bool *ok) const
{
    return d->m_data->stringToDouble(s, ok, d->m_numberOptions);
}
#endif // QT_STRINGVIEW_LEVEL < 2

/*!
    Returns the short int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()

    \since 5.10
*/

short QLocale::toShort(QStringView s, bool *ok) const
{
    return toIntegral_helper<short>(d, s, ok);
}

/*!
    Returns the unsigned short int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()

    \since 5.10
*/

ushort QLocale::toUShort(QStringView s, bool *ok) const
{
    return toIntegral_helper<ushort>(d, s, ok);
}

/*!
    Returns the int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()

    \since 5.10
*/

int QLocale::toInt(QStringView s, bool *ok) const
{
    return toIntegral_helper<int>(d, s, ok);
}

/*!
    Returns the unsigned int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()

    \since 5.10
*/

uint QLocale::toUInt(QStringView s, bool *ok) const
{
    return toIntegral_helper<uint>(d, s, ok);
}

/*!
    Returns the long long int represented by the localized string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()

    \since 5.10
*/


qlonglong QLocale::toLongLong(QStringView s, bool *ok) const
{
    return toIntegral_helper<qlonglong>(d, s, ok);
}

/*!
    Returns the unsigned long long int represented by the localized
    string \a s.

    If the conversion fails, the function returns 0.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()

    \since 5.10
*/

qulonglong QLocale::toULongLong(QStringView s, bool *ok) const
{
    return toIntegral_helper<qulonglong>(d, s, ok);
}

/*!
    Returns the float represented by the localized string \a s, or 0.0
    if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()

    \since 5.10
*/

float QLocale::toFloat(QStringView s, bool *ok) const
{
    return QLocaleData::convertDoubleToFloat(toDouble(s, ok), ok);
}

/*!
    Returns the double represented by the localized string \a s, or
    0.0 if the conversion failed.

    If \a ok is not \c nullptr, failure is reported by setting *\a{ok}
    to \c false, and success by setting *\a{ok} to \c true.

    Unlike QString::toDouble(), this function does not fall back to
    the "C" locale if the string cannot be interpreted in this
    locale.

    \snippet code/src_corelib_tools_qlocale.cpp 3-qstringview

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()

    \since 5.10
*/

double QLocale::toDouble(QStringView s, bool *ok) const
{
    return d->m_data->stringToDouble(s, ok, d->m_numberOptions);
}

/*!
    Returns a localized string representation of \a i.

    \sa toLongLong()
*/

QString QLocale::toString(qlonglong i) const
{
    int flags = d->m_numberOptions & OmitGroupSeparator
                    ? 0
                    : QLocaleData::ThousandsGroup;

    return d->m_data->longLongToString(i, -1, 10, -1, flags);
}

/*!
    \overload

    \sa toULongLong()
*/

QString QLocale::toString(qulonglong i) const
{
    int flags = d->m_numberOptions & OmitGroupSeparator
                    ? 0
                    : QLocaleData::ThousandsGroup;

    return d->m_data->unsLongLongToString(i, -1, 10, -1, flags);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    Returns a localized string representation of the given \a date in the
    specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDate::toString()
*/

QString QLocale::toString(const QDate &date, const QString &format) const
{
    return d->dateTimeToString(format, QDateTime(), date, QTime(), this);
}
#endif

/*!
    \since 5.10

    Returns a localized string representation of the given \a date in the
    specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDate::toString()
*/
QString QLocale::toString(const QDate &date, QStringView format) const
{
    return d->dateTimeToString(format, QDateTime(), date, QTime(), this);
}

/*!
    Returns a localized string representation of the given \a date according
    to the specified \a format.
*/

QString QLocale::toString(const QDate &date, FormatType format) const
{
    if (!date.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateToStringLong : QSystemLocale::DateToStringShort,
                                             date);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = dateFormat(format);
    return toString(date, format_str);
}

static bool timeFormatContainsAP(QStringView format)
{
    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            qt_readEscapedFormatString(format, &i);
            continue;
        }

        if (format.at(i).toLower().unicode() == 'a')
            return true;

        ++i;
    }
    return false;
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    Returns a localized string representation of the given \a time according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QTime::toString()
*/
QString QLocale::toString(const QTime &time, const QString &format) const
{
    return d->dateTimeToString(format, QDateTime(), QDate(), time, this);
}
#endif

/*!
    \since 5.10

    Returns a localized string representation of the given \a time according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QTime::toString()
*/
QString QLocale::toString(const QTime &time, QStringView format) const
{
    return d->dateTimeToString(format, QDateTime(), QDate(), time, this);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    \since 4.4

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDateTime::toString(), QDate::toString(), QTime::toString()
*/

QString QLocale::toString(const QDateTime &dateTime, const QString &format) const
{
    return d->dateTimeToString(format, dateTime, QDate(), QTime(), this);
}
#endif

/*!
    \since 5.10

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.

    \sa QDateTime::toString(), QDate::toString(), QTime::toString()
*/
QString QLocale::toString(const QDateTime &dateTime, QStringView format) const
{
    return d->dateTimeToString(format, dateTime, QDate(), QTime(), this);
}

/*!
    \since 4.4

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
*/

QString QLocale::toString(const QDateTime &dateTime, FormatType format) const
{
    if (!dateTime.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeToStringLong
                                             : QSystemLocale::DateTimeToStringShort,
                                             dateTime);
        if (!res.isNull())
            return res.toString();
    }
#endif

    const QString format_str = dateTimeFormat(format);
    return toString(dateTime, format_str);
}


/*!
    Returns a localized string representation of the given \a time in the
    specified \a format.
*/

QString QLocale::toString(const QTime &time, FormatType format) const
{
    if (!time.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeToStringLong : QSystemLocale::TimeToStringShort,
                                             time);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = timeFormat(format);
    return toString(time, format_str);
}

/*!
    \since 4.1

    Returns the date format used for the current locale.

    If \a format is LongFormat the format will be a long version.
    Otherwise it uses a shorter version.

    \sa QDate::toString(), QDate::fromString()
*/

QString QLocale::dateFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateFormatLong : QSystemLocale::DateFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (format) {
    case LongFormat:
        idx = d->m_data->m_long_date_format_idx;
        size = d->m_data->m_long_date_format_size;
        break;
    default:
        idx = d->m_data->m_short_date_format_idx;
        size = d->m_data->m_short_date_format_size;
        break;
    }
    return getLocaleData(date_format_data + idx, size);
}

/*!
    \since 4.1

    Returns the time format used for the current locale.

    If \a format is LongFormat the format will be a long version.
    Otherwise it uses a shorter version.

    \sa QTime::toString(), QTime::fromString()
*/

QString QLocale::timeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeFormatLong : QSystemLocale::TimeFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (format) {
    case LongFormat:
        idx = d->m_data->m_long_time_format_idx;
        size = d->m_data->m_long_time_format_size;
        break;
    default:
        idx = d->m_data->m_short_time_format_idx;
        size = d->m_data->m_short_time_format_size;
        break;
    }
    return getLocaleData(time_format_data + idx, size);
}

/*!
    \since 4.4

    Returns the date time format used for the current locale.

    If \a format is ShortFormat the format will be a short version.
    Otherwise it uses a longer version.

    \sa QDateTime::toString(), QDateTime::fromString()
*/

QString QLocale::dateTimeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeFormatLong
                                             : QSystemLocale::DateTimeFormatShort,
                                             QVariant());
        if (!res.isNull()) {
            return res.toString();
        }
    }
#endif
    return dateFormat(format) + QLatin1Char(' ') + timeFormat(format);
}

/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. The format of the time string is chosen according to the
    \a format parameter (see timeFormat()).

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
#if QT_CONFIG(datestring)
QTime QLocale::toTime(const QString &string, FormatType format) const
{
    return toTime(string, timeFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. The format of the date string is chosen according to the
    \a format parameter (see dateFormat()).

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
#if QT_CONFIG(datestring)
QDate QLocale::toDate(const QString &string, FormatType format) const
{
    return toDate(string, dateFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time. The format of the date/time string is chosen according to the
    \a format parameter (see dateTimeFormat()).

    If the string could not be parsed, returns an invalid QDateTime.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/

#if QT_CONFIG(datestring)
QDateTime QLocale::toDateTime(const QString &string, FormatType format) const
{
    return toDateTime(string, dateTimeFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. See QTime::fromString() for information on what is a valid
    format string.

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
#if QT_CONFIG(datestring)
QTime QLocale::toTime(const QString &string, const QString &format) const
{
    QTime time;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QVariant::Time, QDateTimeParser::FromString);
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format))
        dt.fromString(string, 0, &time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return time;
}
#endif

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. See QDate::fromString() for information on the expressions
    that can be used with this function.

    This function searches month names and the names of the days of
    the week in the current locale.

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
#if QT_CONFIG(datestring)
QDate QLocale::toDate(const QString &string, const QString &format) const
{
    QDate date;
#if QT_CONFIG(datetimeparser)
    QDateTimeParser dt(QVariant::Date, QDateTimeParser::FromString);
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format))
        dt.fromString(string, &date, 0);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return date;
}
#endif

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time.  See QDateTime::fromString() for information on the expressions
    that can be used with this function.

    \note The month and day names used must be given in the user's local
    language.

    If the string could not be parsed, returns an invalid QDateTime.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/
#if QT_CONFIG(datestring)
QDateTime QLocale::toDateTime(const QString &string, const QString &format) const
{
#if QT_CONFIG(datetimeparser)
    QTime time;
    QDate date;

    QDateTimeParser dt(QVariant::DateTime, QDateTimeParser::FromString);
    dt.setDefaultLocale(*this);
    if (dt.parseFormat(format) && dt.fromString(string, &date, &time))
        return QDateTime(date, time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return QDateTime(QDate(), QTime(-1, -1, -1));
}
#endif


/*!
    \since 4.1

    Returns the decimal point character of this locale.
*/
QChar QLocale::decimalPoint() const
{
    return d->decimal();
}

/*!
    \since 4.1

    Returns the group separator character of this locale.
*/
QChar QLocale::groupSeparator() const
{
    return d->group();
}

/*!
    \since 4.1

    Returns the percent character of this locale.
*/
QChar QLocale::percent() const
{
    return d->percent();
}

/*!
    \since 4.1

    Returns the zero digit character of this locale.
*/
QChar QLocale::zeroDigit() const
{
    return d->zero();
}

/*!
    \since 4.1

    Returns the negative sign character of this locale.
*/
QChar QLocale::negativeSign() const
{
    return d->minus();
}

/*!
    \since 4.5

    Returns the positive sign character of this locale.
*/
QChar QLocale::positiveSign() const
{
    return d->plus();
}

/*!
    \since 4.1

    Returns the exponential character of this locale.
*/
QChar QLocale::exponential() const
{
    return d->exponential();
}

static bool qIsUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static char qToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    else
        return c;
}

/*!
    \overload

    \a f and \a prec have the same meaning as in QString::number(double, char, int).

    \sa toDouble()
*/

QString QLocale::toString(double i, char f, int prec) const
{
    QLocaleData::DoubleForm form = QLocaleData::DFDecimal;
    uint flags = 0;

    if (qIsUpper(f))
        flags = QLocaleData::CapitalEorX;
    f = qToLower(f);

    switch (f) {
        case 'f':
            form = QLocaleData::DFDecimal;
            break;
        case 'e':
            form = QLocaleData::DFExponent;
            break;
        case 'g':
            form = QLocaleData::DFSignificantDigits;
            break;
        default:
            break;
    }

    if (!(d->m_numberOptions & OmitGroupSeparator))
        flags |= QLocaleData::ThousandsGroup;
    if (!(d->m_numberOptions & OmitLeadingZeroInExponent))
        flags |= QLocaleData::ZeroPadExponent;
    if (d->m_numberOptions & IncludeTrailingZeroesAfterDot)
        flags |= QLocaleData::AddTrailingZeroes;
    return d->m_data->doubleToString(i, prec, form, -1, flags);
}

/*!
    \fn QLocale QLocale::c()

    Returns a QLocale object initialized to the "C" locale.

    \sa system()
*/

/*!
    Returns a QLocale object initialized to the system locale.

    On Windows and Mac, this locale will use the decimal/grouping characters and date/time
    formats specified in the system configuration panel.

    \sa c()
*/

QLocale QLocale::system()
{
    return QLocale(*QLocalePrivate::create(systemData()));
}


/*!
    \since 4.8

    Returns a list of valid locale objects that match the given \a language, \a
    script and \a country.

    Getting a list of all locales:
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);

    Getting a list of locales suitable for Russia:
    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::Russia);
*/
QList<QLocale> QLocale::matchingLocales(QLocale::Language language,
                                        QLocale::Script script,
                                        QLocale::Country country)
{
    if (uint(language) > QLocale::LastLanguage || uint(script) > QLocale::LastScript ||
            uint(country) > QLocale::LastCountry)
        return QList<QLocale>();

    if (language == QLocale::C)
        return QList<QLocale>() << QLocale(QLocale::C);

    QList<QLocale> result;
    if (language == QLocale::AnyLanguage && script == QLocale::AnyScript && country == QLocale::AnyCountry)
        result.reserve(locale_data_size);
    const QLocaleData *data = locale_data + locale_index[language];
    while ( (data != locale_data + locale_data_size)
            && (language == QLocale::AnyLanguage || data->m_language_id == uint(language))) {
        if ((script == QLocale::AnyScript || data->m_script_id == uint(script))
            && (country == QLocale::AnyCountry || data->m_country_id == uint(country))) {
            result.append(QLocale(*(data->m_language_id == C ? c_private()
                                    : QLocalePrivate::create(data))));
        }
        ++data;
    }
    return result;
}

/*!
    \obsolete
    \since 4.3

    Returns the list of countries that have entries for \a language in Qt's locale
    database. If the result is an empty list, then \a language is not represented in
    Qt's locale database.

    \sa matchingLocales()
*/
QList<QLocale::Country> QLocale::countriesForLanguage(Language language)
{
    QList<Country> result;
    if (language == C) {
        result << AnyCountry;
        return result;
    }

    unsigned language_id = language;
    const QLocaleData *data = locale_data + locale_index[language_id];
    while (data->m_language_id == language_id) {
        const QLocale::Country country = static_cast<Country>(data->m_country_id);
        if (!result.contains(country))
            result.append(country);
        ++data;
    }

    return result;
}

/*!
    \since 4.2

    Returns the localized name of \a month, in the format specified
    by \a type.

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::monthName(int month, FormatType type) const
{
    if (month < 1 || month > 12)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::MonthNameLong : QSystemLocale::MonthNameShort,
                                             month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d->m_data->m_long_month_names_idx;
        size = d->m_data->m_long_month_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d->m_data->m_short_month_names_idx;
        size = d->m_data->m_short_month_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d->m_data->m_narrow_month_names_idx;
        size = d->m_data->m_narrow_month_names_size;
        break;
    default:
        return QString();
    }
    return getLocaleListData(months_data + idx, size, month - 1);
}

/*!
    \since 4.5

    Returns the localized name of \a month that is used as a
    standalone text, in the format specified by \a type.

    If the locale information doesn't specify the standalone month
    name then return value is the same as in monthName().

    \sa monthName(), standaloneDayName()
*/
QString QLocale::standaloneMonthName(int month, FormatType type) const
{
    if (month < 1 || month > 12)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::StandaloneMonthNameLong : QSystemLocale::StandaloneMonthNameShort,
                                             month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d->m_data->m_standalone_long_month_names_idx;
        size = d->m_data->m_standalone_long_month_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d->m_data->m_standalone_short_month_names_idx;
        size = d->m_data->m_standalone_short_month_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d->m_data->m_standalone_narrow_month_names_idx;
        size = d->m_data->m_standalone_narrow_month_names_size;
        break;
    default:
        return QString();
    }
    QString name = getLocaleListData(months_data + idx, size, month - 1);
    if (name.isEmpty())
        return monthName(month, type);
    return name;
}

/*!
    \since 4.2

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on), in the format specified
    by \a type.

    \sa monthName(), standaloneDayName()
*/
QString QLocale::dayName(int day, FormatType type) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::DayNameLong : QSystemLocale::DayNameShort,
                                             day);
        if (!res.isNull())
            return res.toString();
    }
#endif
    if (day == 7)
        day = 0;

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d->m_data->m_long_day_names_idx;
        size = d->m_data->m_long_day_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d->m_data->m_short_day_names_idx;
        size = d->m_data->m_short_day_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d->m_data->m_narrow_day_names_idx;
        size = d->m_data->m_narrow_day_names_size;
        break;
    default:
        return QString();
    }
    return getLocaleListData(days_data + idx, size, day);
}

/*!
    \since 4.5

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on) that is used as a
    standalone text, in the format specified by \a type.

    If the locale information does not specify the standalone day
    name then return value is the same as in dayName().

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::standaloneDayName(int day, FormatType type) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::DayNameLong : QSystemLocale::DayNameShort,
                                             day);
        if (!res.isNull())
            return res.toString();
    }
#endif
    if (day == 7)
        day = 0;

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d->m_data->m_standalone_long_day_names_idx;
        size = d->m_data->m_standalone_long_day_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d->m_data->m_standalone_short_day_names_idx;
        size = d->m_data->m_standalone_short_day_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d->m_data->m_standalone_narrow_day_names_idx;
        size = d->m_data->m_standalone_narrow_day_names_size;
        break;
    default:
        return QString();
    }
    QString name = getLocaleListData(days_data + idx, size, day);
    if (name.isEmpty())
        return dayName(day == 0 ? 7 : day, type);
    return name;
}

/*!
    \since 4.8

    Returns the first day of the week according to the current locale.
*/
Qt::DayOfWeek QLocale::firstDayOfWeek() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::FirstDayOfWeek, QVariant());
        if (!res.isNull())
            return static_cast<Qt::DayOfWeek>(res.toUInt());
    }
#endif
    return static_cast<Qt::DayOfWeek>(d->m_data->m_first_day_of_week);
}

QLocale::MeasurementSystem QLocalePrivate::measurementSystem() const
{
    for (int i = 0; i < ImperialMeasurementSystemsCount; ++i) {
        if (ImperialMeasurementSystems[i].languageId == m_data->m_language_id
            && ImperialMeasurementSystems[i].countryId == m_data->m_country_id) {
            return ImperialMeasurementSystems[i].system;
        }
    }
    return QLocale::MetricSystem;
}

/*!
    \since 4.8

    Returns a list of days that are considered weekdays according to the current locale.
*/
QList<Qt::DayOfWeek> QLocale::weekdays() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::Weekdays, QVariant());
        if (!res.isNull())
            return static_cast<QList<Qt::DayOfWeek> >(res.value<QList<Qt::DayOfWeek> >());
    }
#endif
    QList<Qt::DayOfWeek> weekdays;
    quint16 weekendStart = d->m_data->m_weekend_start;
    quint16 weekendEnd = d->m_data->m_weekend_end;
    for (int day = Qt::Monday; day <= Qt::Sunday; day++) {
        if ((weekendEnd >= weekendStart && (day < weekendStart || day > weekendEnd)) ||
            (weekendEnd < weekendStart && (day > weekendEnd && day < weekendStart)))
                weekdays << static_cast<Qt::DayOfWeek>(day);
    }
    return weekdays;
}

/*!
    \since 4.4

    Returns the measurement system for the locale.
*/
QLocale::MeasurementSystem QLocale::measurementSystem() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::MeasurementSystem, QVariant());
        if (!res.isNull())
            return MeasurementSystem(res.toInt());
    }
#endif

    return d->measurementSystem();
}

/*!
  \since 4.7

  Returns the text direction of the language.
*/
Qt::LayoutDirection QLocale::textDirection() const
{
    switch (script()) {
    case QLocale::AdlamScript:
    case QLocale::ArabicScript:
    case QLocale::AvestanScript:
    case QLocale::CypriotScript:
    case QLocale::HatranScript:
    case QLocale::HebrewScript:
    case QLocale::ImperialAramaicScript:
    case QLocale::InscriptionalPahlaviScript:
    case QLocale::InscriptionalParthianScript:
    case QLocale::KharoshthiScript:
    case QLocale::LydianScript:
    case QLocale::MandaeanScript:
    case QLocale::ManichaeanScript:
    case QLocale::MendeKikakuiScript:
    case QLocale::MeroiticCursiveScript:
    case QLocale::MeroiticScript:
    case QLocale::NabataeanScript:
    case QLocale::NkoScript:
    case QLocale::OldHungarianScript:
    case QLocale::OldNorthArabianScript:
    case QLocale::OldSouthArabianScript:
    case QLocale::OrkhonScript:
    case QLocale::PalmyreneScript:
    case QLocale::PhoenicianScript:
    case QLocale::PsalterPahlaviScript:
    case QLocale::SamaritanScript:
    case QLocale::SyriacScript:
    case QLocale::ThaanaScript:
        return Qt::RightToLeft;
    default:
        break;
    }
    return Qt::LeftToRight;
}

/*!
  \since 4.8

  Returns an uppercase copy of \a str.

  If Qt Core is using the ICU libraries, they will be used to perform
  the transformation according to the rules of the current locale.
  Otherwise the conversion may be done in a platform-dependent manner,
  with QString::toUpper() as a generic fallback.

  \sa QString::toUpper()
*/
QString QLocale::toUpper(const QString &str) const
{
#if QT_CONFIG(icu)
    bool ok = true;
    QString result = QIcu::toUpper(d->bcp47Name('_'), str, &ok);
    if (ok)
        return result;
    // else fall through and use Qt's toUpper
#endif
    return str.toUpper();
}

/*!
  \since 4.8

  Returns a lowercase copy of \a str.

  If Qt Core is using the ICU libraries, they will be used to perform
  the transformation according to the rules of the current locale.
  Otherwise the conversion may be done in a platform-dependent manner,
  with QString::toLower() as a generic fallback.

  \sa QString::toLower()
*/
QString QLocale::toLower(const QString &str) const
{
#if QT_CONFIG(icu)
    bool ok = true;
    const QString result = QIcu::toLower(d->bcp47Name('_'), str, &ok);
    if (ok)
        return result;
    // else fall through and use Qt's toUpper
#endif
    return str.toLower();
}


/*!
    \since 4.5

    Returns the localized name of the "AM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa pmText()
*/
QString QLocale::amText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::AMText, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(am_data + d->m_data->m_am_idx, d->m_data->m_am_size);
}

/*!
    \since 4.5

    Returns the localized name of the "PM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa amText()
*/
QString QLocale::pmText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::PMText, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(pm_data + d->m_data->m_pm_idx, d->m_data->m_pm_size);
}


QString QLocalePrivate::dateTimeToString(QStringView format, const QDateTime &datetime,
                                         const QDate &dateOnly, const QTime &timeOnly,
                                         const QLocale *q) const
{
    QDate date;
    QTime time;
    bool formatDate = false;
    bool formatTime = false;
    if (datetime.isValid()) {
        date = datetime.date();
        time = datetime.time();
        formatDate = true;
        formatTime = true;
    } else if (dateOnly.isValid()) {
        date = dateOnly;
        formatDate = true;
    } else if (timeOnly.isValid()) {
        time = timeOnly;
        formatTime = true;
    } else {
        return QString();
    }

    QString result;

    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            result.append(qt_readEscapedFormatString(format, &i));
            continue;
        }

        const QChar c = format.at(i);
        int repeat = qt_repeatCount(format.mid(i));
        bool used = false;
        if (formatDate) {
            switch (c.unicode()) {
            case 'y':
                used = true;
                if (repeat >= 4)
                    repeat = 4;
                else if (repeat >= 2)
                    repeat = 2;

                switch (repeat) {
                case 4: {
                    const int yr = date.year();
                    const int len = (yr < 0) ? 5 : 4;
                    result.append(m_data->longLongToString(yr, -1, 10, len, QLocaleData::ZeroPadded));
                    break;
                }
                case 2:
                    result.append(m_data->longLongToString(date.year() % 100, -1, 10, 2,
                                                   QLocaleData::ZeroPadded));
                    break;
                default:
                    repeat = 1;
                    result.append(c);
                    break;
                }
                break;

            case 'M':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(date.month()));
                    break;
                case 2:
                    result.append(m_data->longLongToString(date.month(), -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                case 3:
                    result.append(q->monthName(date.month(), QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(q->monthName(date.month(), QLocale::LongFormat));
                    break;
                }
                break;

            case 'd':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(date.day()));
                    break;
                case 2:
                    result.append(m_data->longLongToString(date.day(), -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                case 3:
                    result.append(q->dayName(date.dayOfWeek(), QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(q->dayName(date.dayOfWeek(), QLocale::LongFormat));
                    break;
                }
                break;

            default:
                break;
            }
        }
        if (!used && formatTime) {
            switch (c.unicode()) {
            case 'h': {
                used = true;
                repeat = qMin(repeat, 2);
                int hour = time.hour();
                if (timeFormatContainsAP(format)) {
                    if (hour > 12)
                        hour -= 12;
                    else if (hour == 0)
                        hour = 12;
                }

                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(hour));
                    break;
                case 2:
                    result.append(m_data->longLongToString(hour, -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                }
                break;
            }
            case 'H':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(time.hour()));
                    break;
                case 2:
                    result.append(m_data->longLongToString(time.hour(), -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 'm':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(time.minute()));
                    break;
                case 2:
                    result.append(m_data->longLongToString(time.minute(), -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 's':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(m_data->longLongToString(time.second()));
                    break;
                case 2:
                    result.append(m_data->longLongToString(time.second(), -1, 10, 2, QLocaleData::ZeroPadded));
                    break;
                }
                break;

            case 'a':
                used = true;
                if (i + 1 < format.size() && format.at(i + 1).unicode() == 'p') {
                    repeat = 2;
                } else {
                    repeat = 1;
                }
                result.append(time.hour() < 12 ? q->amText().toLower() : q->pmText().toLower());
                break;

            case 'A':
                used = true;
                if (i + 1 < format.size() && format.at(i + 1).unicode() == 'P') {
                    repeat = 2;
                } else {
                    repeat = 1;
                }
                result.append(time.hour() < 12 ? q->amText().toUpper() : q->pmText().toUpper());
                break;

            case 'z':
                used = true;
                if (repeat >= 3) {
                    repeat = 3;
                } else {
                    repeat = 1;
                }

                // note: the millisecond component is treated like the decimal part of the seconds
                // so ms == 2 is always printed as "002", but ms == 200 can be either "2" or "200"
                result.append(m_data->longLongToString(time.msec(), -1, 10, 3, QLocaleData::ZeroPadded));
                if (repeat == 1) {
                    if (result.endsWith(zero()))
                        result.chop(1);
                    if (result.endsWith(zero()))
                        result.chop(1);
                }

                break;

            case 't':
                used = true;
                repeat = 1;
                // If we have a QDateTime use the time spec otherwise use the current system tzname
                if (formatDate) {
                    result.append(datetime.timeZoneAbbreviation());
                } else {
                    result.append(QDateTime::currentDateTime().timeZoneAbbreviation());
                }
                break;

            default:
                break;
            }
        }
        if (!used) {
            result.append(QString(repeat, c));
        }
        i += repeat;
    }

    return result;
}

QString QLocaleData::doubleToString(double d, int precision, DoubleForm form,
                                    int width, unsigned flags) const
{
    return doubleToString(m_zero, m_plus, m_minus, m_exponential, m_group, m_decimal,
                          d, precision, form, width, flags);
}

QString QLocaleData::doubleToString(const QChar _zero, const QChar plus, const QChar minus,
                                    const QChar exponential, const QChar group, const QChar decimal,
                                    double d, int precision, DoubleForm form, int width, unsigned flags)
{
    if (precision != QLocale::FloatingPointShortest && precision < 0)
        precision = 6;
    if (width < 0)
        width = 0;

    bool negative = false;
    QString num_str;

    int decpt;
    int bufSize = 1;
    if (precision == QLocale::FloatingPointShortest)
        bufSize += DoubleMaxSignificant;
    else if (form == DFDecimal) // optimize for numbers between -512k and 512k
        bufSize += ((d > (1 << 19) || d < -(1 << 19)) ? DoubleMaxDigitsBeforeDecimal : 6) +
                precision;
    else // Add extra digit due to different interpretations of precision. Also, "nan" has to fit.
        bufSize += qMax(2, precision) + 1;

    QVarLengthArray<char> buf(bufSize);
    int length;

    doubleToAscii(d, form, precision, buf.data(), bufSize, negative, length, decpt);

    if (qstrncmp(buf.data(), "inf", 3) == 0 || qstrncmp(buf.data(), "nan", 3) == 0) {
        num_str = QString::fromLatin1(buf.data(), length);
    } else { // Handle normal numbers
        QString digits = QString::fromLatin1(buf.data(), length);

        if (_zero.unicode() != '0') {
            ushort z = _zero.unicode() - '0';
            for (int i = 0; i < digits.length(); ++i)
                reinterpret_cast<ushort *>(digits.data())[i] += z;
        }

        bool always_show_decpt = (flags & ForcePoint);
        switch (form) {
            case DFExponent: {
                num_str = exponentForm(_zero, decimal, exponential, group, plus, minus,
                                       digits, decpt, precision, PMDecimalDigits,
                                       always_show_decpt, flags & ZeroPadExponent);
                break;
            }
            case DFDecimal: {
                num_str = decimalForm(_zero, decimal, group,
                                      digits, decpt, precision, PMDecimalDigits,
                                      always_show_decpt, flags & ThousandsGroup);
                break;
            }
            case DFSignificantDigits: {
                PrecisionMode mode = (flags & AddTrailingZeroes) ?
                            PMSignificantDigits : PMChopTrailingZeros;

                int cutoff = precision < 0 ? 6 : precision;
                // Find out which representation is shorter
                if (precision == QLocale::FloatingPointShortest && decpt > 0) {
                    cutoff = digits.length() + 4; // 'e', '+'/'-', one digit exponent
                    if (decpt <= 10) {
                        ++cutoff;
                    } else {
                        cutoff += decpt > 100 ? 2 : 1;
                    }
                    if (!always_show_decpt && digits.length() > decpt)
                        ++cutoff; // decpt shown in exponent form, but not in decimal form
                }

                if (decpt != digits.length() && (decpt <= -4 || decpt > cutoff))
                    num_str = exponentForm(_zero, decimal, exponential, group, plus, minus,
                                           digits, decpt, precision, mode,
                                           always_show_decpt, flags & ZeroPadExponent);
                else
                    num_str = decimalForm(_zero, decimal, group,
                                          digits, decpt, precision, mode,
                                          always_show_decpt, flags & ThousandsGroup);
                break;
            }
        }

        if (isZero(d))
            negative = false;

        // pad with zeros. LeftAdjusted overrides this flag). Also, we don't
        // pad special numbers
        if (flags & QLocaleData::ZeroPadded && !(flags & QLocaleData::LeftAdjusted)) {
            int num_pad_chars = width - num_str.length();
            // leave space for the sign
            if (negative
                    || flags & QLocaleData::AlwaysShowSign
                    || flags & QLocaleData::BlankBeforePositive)
                --num_pad_chars;

            for (int i = 0; i < num_pad_chars; ++i)
                num_str.prepend(_zero);
        }
    }

    // add sign
    if (negative)
        num_str.prepend(minus);
    else if (flags & QLocaleData::AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & QLocaleData::BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    if (flags & QLocaleData::CapitalEorX)
        num_str = std::move(num_str).toUpper();

    return num_str;
}

QString QLocaleData::longLongToString(qlonglong l, int precision,
                                            int base, int width,
                                            unsigned flags) const
{
    return longLongToString(m_zero, m_group, m_plus, m_minus,
                                            l, precision, base, width, flags);
}

QString QLocaleData::longLongToString(const QChar zero, const QChar group,
                                         const QChar plus, const QChar minus,
                                         qlonglong l, int precision,
                                         int base, int width,
                                         unsigned flags)
{
    bool precision_not_specified = false;
    if (precision == -1) {
        precision_not_specified = true;
        precision = 1;
    }

    bool negative = l < 0;
    if (base != 10) {
        // these are not supported by sprintf for octal and hex
        flags &= ~AlwaysShowSign;
        flags &= ~BlankBeforePositive;
        negative = false; // neither are negative numbers
    }

QT_WARNING_PUSH
    /* "unary minus operator applied to unsigned type, result still unsigned" */
QT_WARNING_DISABLE_MSVC(4146)
    /*
      Negating std::numeric_limits<qlonglong>::min() hits undefined behavior, so
      taking an absolute value has to cast to unsigned to change sign.
     */
    QString num_str = qulltoa(negative ? -qulonglong(l) : qulonglong(l), base, zero);
QT_WARNING_POP

    uint cnt_thousand_sep = 0;
    if (flags & ThousandsGroup && base == 10) {
        for (int i = num_str.length() - 3; i > 0; i -= 3) {
            num_str.insert(i, group);
            ++cnt_thousand_sep;
        }
    }

    for (int i = num_str.length()/* - cnt_thousand_sep*/; i < precision; ++i)
        num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));

    if ((flags & ShowBase)
            && base == 8
            && (num_str.isEmpty() || num_str[0].unicode() != QLatin1Char('0')))
        num_str.prepend(QLatin1Char('0'));

    // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
    // when precision is not specified in the format string
    bool zero_padded = flags & ZeroPadded
                        && !(flags & LeftAdjusted)
                        && precision_not_specified;

    if (zero_padded) {
        int num_pad_chars = width - num_str.length();

        // leave space for the sign
        if (negative
                || flags & AlwaysShowSign
                || flags & BlankBeforePositive)
            --num_pad_chars;

        // leave space for optional '0x' in hex form
        if (base == 16 && (flags & ShowBase))
            num_pad_chars -= 2;
        // leave space for optional '0b' in binary form
        else if (base == 2 && (flags & ShowBase))
            num_pad_chars -= 2;

        for (int i = 0; i < num_pad_chars; ++i)
            num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));
    }

    if (flags & CapitalEorX)
        num_str = std::move(num_str).toUpper();

    if (base == 16 && (flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0X" : "0x"));
    if (base == 2 && (flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0B" : "0b"));

    // add sign
    if (negative)
        num_str.prepend(minus);
    else if (flags & AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    return num_str;
}

QString QLocaleData::unsLongLongToString(qulonglong l, int precision,
                                            int base, int width,
                                            unsigned flags) const
{
    return unsLongLongToString(m_zero, m_group, m_plus,
                                               l, precision, base, width, flags);
}

QString QLocaleData::unsLongLongToString(const QChar zero, const QChar group,
                                            const QChar plus,
                                            qulonglong l, int precision,
                                            int base, int width,
                                            unsigned flags)
{
    const QChar resultZero = base == 10 ? zero : QChar(QLatin1Char('0'));
    QString num_str = l ? qulltoa(l, base, zero) : QString(resultZero);

    bool precision_not_specified = false;
    if (precision == -1) {
        if (flags == NoFlags)
            return num_str; // fast-path: nothing below applies, so we're done.

        precision_not_specified = true;
        precision = 1;
    }

    uint cnt_thousand_sep = 0;
    if (flags & ThousandsGroup && base == 10) {
        for (int i = num_str.length() - 3; i > 0; i -=3) {
            num_str.insert(i, group);
            ++cnt_thousand_sep;
        }
    }

    const int zeroPadding = precision - num_str.length()/* + cnt_thousand_sep*/;
    if (zeroPadding > 0)
        num_str.prepend(QString(zeroPadding, resultZero));

    if ((flags & ShowBase)
            && base == 8
            && (num_str.isEmpty() || num_str.at(0).unicode() != QLatin1Char('0')))
        num_str.prepend(QLatin1Char('0'));

    // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
    // when precision is not specified in the format string
    bool zero_padded = flags & ZeroPadded
                        && !(flags & LeftAdjusted)
                        && precision_not_specified;

    if (zero_padded) {
        int num_pad_chars = width - num_str.length();

        // leave space for optional '0x' in hex form
        if (base == 16 && flags & ShowBase)
            num_pad_chars -= 2;
        // leave space for optional '0b' in binary form
        else if (base == 2 && flags & ShowBase)
            num_pad_chars -= 2;

        if (num_pad_chars > 0)
            num_str.prepend(QString(num_pad_chars, resultZero));
    }

    if (flags & CapitalEorX)
        num_str = std::move(num_str).toUpper();

    if (base == 16 && flags & ShowBase)
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0X" : "0x"));
    else if (base == 2 && flags & ShowBase)
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0B" : "0b"));

    // add sign
    if (flags & AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    return num_str;
}

/*
    Converts a number in locale to its representation in the C locale.
    Only has to guarantee that a string that is a correct representation of
    a number will be converted. If junk is passed in, junk will be passed
    out and the error will be detected during the actual conversion to a
    number. We can't detect junk here, since we don't even know the base
    of the number.
*/
bool QLocaleData::numberToCLocale(QStringView s, QLocale::NumberOptions number_options,
                                  CharBuff *result) const
{
    const QChar *uc = s.data();
    auto l = s.size();
    decltype(l) idx = 0;

    // Skip whitespace
    while (idx < l && uc[idx].isSpace())
        ++idx;
    if (idx == l)
        return false;

    // Check trailing whitespace
    for (; idx < l; --l) {
        if (!uc[l - 1].isSpace())
            break;
    }

    int group_cnt = 0; // counts number of group chars
    int decpt_idx = -1;
    int last_separator_idx = -1;
    int start_of_digits_idx = -1;
    int exponent_idx = -1;

    while (idx < l) {
        const QChar in = uc[idx];

        char out = digitToCLocale(in);
        if (out == 0) {
            if (in == m_list)
                out = ';';
            else if (in == m_percent)
                out = '%';
            // for handling base-x numbers
            else if (in.unicode() >= 'A' && in.unicode() <= 'Z')
                out = in.toLower().toLatin1();
            else if (in.unicode() >= 'a' && in.unicode() <= 'z')
                out = in.toLatin1();
            else
                break;
        } else if (out == '.') {
            // Fail if more than one decimal point or point after e
            if (decpt_idx != -1 || exponent_idx != -1)
                return false;
            decpt_idx = idx;
        } else if (out == 'e' || out == 'E') {
            exponent_idx = idx;
        }

        if (number_options & QLocale::RejectLeadingZeroInExponent) {
            if (exponent_idx != -1 && out == '0' && idx < l - 1) {
                // After the exponent there can only be '+', '-' or digits.
                // If we find a '0' directly after some non-digit, then that is a leading zero.
                if (result->last() < '0' || result->last() > '9')
                    return false;
            }
        }

        if (number_options & QLocale::RejectTrailingZeroesAfterDot) {
            // If we've seen a decimal point and the last character after the exponent is 0, then
            // that is a trailing zero.
            if (decpt_idx >= 0 && idx == exponent_idx && result->last() == '0')
                    return false;
        }

        if (!(number_options & QLocale::RejectGroupSeparator)) {
            if (start_of_digits_idx == -1 && out >= '0' && out <= '9') {
                start_of_digits_idx = idx;
            } else if (out == ',') {
                // Don't allow group chars after the decimal point or exponent
                if (decpt_idx != -1 || exponent_idx != -1)
                    return false;

                // check distance from the last separator or from the beginning of the digits
                // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
                if (last_separator_idx != -1 && idx - last_separator_idx != 4)
                    return false;
                if (last_separator_idx == -1 && (start_of_digits_idx == -1 || idx - start_of_digits_idx > 3))
                    return false;

                last_separator_idx = idx;
                ++group_cnt;

                // don't add the group separator
                ++idx;
                continue;
            } else if (out == '.' || out == 'e' || out == 'E') {
                // check distance from the last separator
                // ### FIXME: Some locales allow other groupings! See https://en.wikipedia.org/wiki/Thousands_separator
                if (last_separator_idx != -1 && idx - last_separator_idx != 4)
                    return false;

                // stop processing separators
                last_separator_idx = -1;
            }
        }

        result->append(out);

        ++idx;
    }

    if (!(number_options & QLocale::RejectGroupSeparator)) {
        // group separator post-processing
        // did we end in a separator?
        if (last_separator_idx + 1 == idx)
            return false;
        // were there enough digits since the last separator?
        if (last_separator_idx != -1 && idx - last_separator_idx != 4)
            return false;
    }

    if (number_options & QLocale::RejectTrailingZeroesAfterDot) {
        // In decimal form, the last character can be a trailing zero if we've seen a decpt.
        if (decpt_idx != -1 && exponent_idx == -1 && result->last() == '0')
            return false;
    }

    result->append('\0');
    return idx == l;
}

bool QLocaleData::validateChars(QStringView str, NumberMode numMode, QByteArray *buff,
                                int decDigits, QLocale::NumberOptions number_options) const
{
    buff->clear();
    buff->reserve(str.length());

    const bool scientific = numMode == DoubleScientificMode;
    bool lastWasE = false;
    bool lastWasDigit = false;
    int eCnt = 0;
    int decPointCnt = 0;
    bool dec = false;
    int decDigitCnt = 0;

    for (qsizetype i = 0; i < str.size(); ++i) {
        char c = digitToCLocale(str.at(i));

        if (c >= '0' && c <= '9') {
            if (numMode != IntegerMode) {
                // If a double has too many digits after decpt, it shall be Invalid.
                if (dec && decDigits != -1 && decDigits < ++decDigitCnt)
                    return false;
            }

            // The only non-digit character after the 'e' can be '+' or '-'.
            // If a zero is directly after that, then the exponent is zero-padded.
            if ((number_options & QLocale::RejectLeadingZeroInExponent) && c == '0' && eCnt > 0 &&
                    !lastWasDigit)
                return false;

            lastWasDigit = true;
        } else {
            switch (c) {
                case '.':
                    if (numMode == IntegerMode) {
                        // If an integer has a decimal point, it shall be Invalid.
                        return false;
                    } else {
                        // If a double has more than one decimal point, it shall be Invalid.
                        if (++decPointCnt > 1)
                            return false;
#if 0
                        // If a double with no decimal digits has a decimal point, it shall be
                        // Invalid.
                        if (decDigits == 0)
                            return false;
#endif                  // On second thoughts, it shall be Valid.

                        dec = true;
                    }
                    break;

                case '+':
                case '-':
                    if (scientific) {
                        // If a scientific has a sign that's not at the beginning or after
                        // an 'e', it shall be Invalid.
                        if (i != 0 && !lastWasE)
                            return false;
                    } else {
                        // If a non-scientific has a sign that's not at the beginning,
                        // it shall be Invalid.
                        if (i != 0)
                            return false;
                    }
                    break;

                case ',':
                    //it can only be placed after a digit which is before the decimal point
                    if ((number_options & QLocale::RejectGroupSeparator) || !lastWasDigit ||
                            decPointCnt > 0)
                        return false;
                    break;

                case 'e':
                    if (scientific) {
                        // If a scientific has more than one 'e', it shall be Invalid.
                        if (++eCnt > 1)
                            return false;
                        dec = false;
                    } else {
                        // If a non-scientific has an 'e', it shall be Invalid.
                        return false;
                    }
                    break;

                default:
                    // If it's not a valid digit, it shall be Invalid.
                    return false;
            }
            lastWasDigit = false;
        }

        lastWasE = c == 'e';
        if (c != ',')
            buff->append(c);
    }

    return true;
}

double QLocaleData::stringToDouble(QStringView str, bool *ok,
                                   QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0.0;
    }
    int processed = 0;
    bool nonNullOk = false;
    double d = asciiToDouble(buff.constData(), buff.length() - 1, nonNullOk, processed);
    if (ok)
        *ok = nonNullOk;
    return d;
}

qlonglong QLocaleData::stringToLongLong(QStringView str, int base, bool *ok,
                                        QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    return bytearrayToLongLong(buff.constData(), base, ok);
}

qulonglong QLocaleData::stringToUnsLongLong(QStringView str, int base, bool *ok,
                                            QLocale::NumberOptions number_options) const
{
    CharBuff buff;
    if (!numberToCLocale(str, number_options, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    return bytearrayToUnsLongLong(buff.constData(), base, ok);
}

double QLocaleData::bytearrayToDouble(const char *num, bool *ok)
{
    bool nonNullOk = false;
    int len = static_cast<int>(strlen(num));
    Q_ASSERT(len >= 0);
    int processed = 0;
    double d = asciiToDouble(num, len, nonNullOk, processed);
    if (ok)
        *ok = nonNullOk;
    return d;
}

qlonglong QLocaleData::bytearrayToLongLong(const char *num, int base, bool *ok)
{
    bool _ok;
    const char *endptr;

    if (*num == '\0') {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    qlonglong l = qstrtoll(num, &endptr, base, &_ok);

    if (!_ok) {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    if (*endptr != '\0') {
        // we stopped at a non-digit character after converting some digits
        if (ok != 0)
            *ok = false;
        return 0;
    }

    if (ok != 0)
        *ok = true;
    return l;
}

qulonglong QLocaleData::bytearrayToUnsLongLong(const char *num, int base, bool *ok)
{
    bool _ok;
    const char *endptr;
    qulonglong l = qstrtoull(num, &endptr, base, &_ok);

    if (!_ok || *endptr != '\0') {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    if (ok != 0)
        *ok = true;
    return l;
}

/*!
    \since 4.8

    \enum QLocale::CurrencySymbolFormat

    Specifies the format of the currency symbol.

    \value CurrencyIsoCode a ISO-4217 code of the currency.
    \value CurrencySymbol a currency symbol.
    \value CurrencyDisplayName a user readable name of the currency.
*/

/*!
    \since 4.8
    Returns a currency symbol according to the \a format.
*/
QString QLocale::currencySymbol(QLocale::CurrencySymbolFormat format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::CurrencySymbol, format);
        if (!res.isNull())
            return res.toString();
    }
#endif
    quint32 idx, size;
    switch (format) {
    case CurrencySymbol:
        idx = d->m_data->m_currency_symbol_idx;
        size = d->m_data->m_currency_symbol_size;
        return getLocaleData(currency_symbol_data + idx, size);
    case CurrencyDisplayName:
        idx = d->m_data->m_currency_display_name_idx;
        size = d->m_data->m_currency_display_name_size;
        return getLocaleListData(currency_display_name_data + idx, size, 0);
    case CurrencyIsoCode: {
        int len = 0;
        const QLocaleData *data = this->d->m_data;
        for (; len < 3; ++len)
            if (!data->m_currency_iso_code[len])
                break;
        return len ? QString::fromLatin1(data->m_currency_iso_code, len) : QString();
    }
    }
    return QString();
}

/*!
    \since 4.8

    Returns a localized string representation of \a value as a currency.
    If the \a symbol is provided it is used instead of the default currency symbol.

    \sa currencySymbol()
*/
QString QLocale::toCurrencyString(qlonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocalePrivate *d = this->d;
    quint8 idx = d->m_data->m_currency_format_idx;
    quint8 size = d->m_data->m_currency_format_size;
    if (d->m_data->m_currency_negative_format_size && value < 0) {
        idx = d->m_data->m_currency_negative_format_idx;
        size = d->m_data->m_currency_negative_format_size;
        value = -value;
    }
    QString str = toString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

/*!
    \since 4.8
    \overload
*/
QString QLocale::toCurrencyString(qulonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocaleData *data = this->d->m_data;
    quint8 idx = data->m_currency_format_idx;
    quint8 size = data->m_currency_format_size;
    QString str = toString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
/*!
    \since 4.8
    \overload
*/
QString QLocale::toCurrencyString(double value, const QString &symbol) const
{
    return toCurrencyString(value, symbol, d->m_data->m_currency_digits);
}
#endif

/*!
    \since 5.7
    \overload toCurrencyString()

    Returns a localized string representation of \a value as a currency.
    If the \a symbol is provided it is used instead of the default currency symbol.
    If the \a precision is provided it is used to set the precision of the currency value.

    \sa currencySymbol()
 */
QString QLocale::toCurrencyString(double value, const QString &symbol, int precision) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocaleData *data = this->d->m_data;
    quint8 idx = data->m_currency_format_idx;
    quint8 size = data->m_currency_format_size;
    if (data->m_currency_negative_format_size && value < 0) {
        idx = data->m_currency_negative_format_idx;
        size = data->m_currency_negative_format_size;
        value = -value;
    }
    QString str = toString(value, 'f', precision == -1 ? d->m_data->m_currency_digits : precision);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

/*!
  \fn QString QLocale::toCurrencyString(float i, const QString &symbol) const
  \fn QString QLocale::toCurrencyString(float i, const QString &symbol, int precision) const
  \overload toCurrencyString()
*/

/*!
    \since 5.10

    \enum QLocale::DataSizeFormat

    Specifies the format for representation of data quantities.

    \omitvalue DataSizeBase1000
    \omitvalue DataSizeSIQuantifiers
    \value DataSizeIecFormat            format using base 1024 and IEC prefixes: KiB, MiB, GiB, ...
    \value DataSizeTraditionalFormat    format using base 1024 and SI prefixes: kB, MB, GB, ...
    \value DataSizeSIFormat             format using base 1000 and SI prefixes: kB, MB, GB, ...

    \sa formattedDataSize()
*/

/*!
    \since 5.10

    Converts a size in bytes to a human-readable localized string, comprising a
    number and a quantified unit. The quantifier is chosen such that the number
    is at least one, and as small as possible. For example if \a bytes is
    16384, \a precision is 2, and \a format is \l DataSizeIecFormat (the
    default), this function returns "16.00 KiB"; for 1330409069609 bytes it
    returns "1.21 GiB"; and so on. If \a format is \l DataSizeIecFormat or
    \l DataSizeTraditionalFormat, the given number of bytes is divided by a
    power of 1024, with result less than 1024; for \l DataSizeSIFormat, it is
    divided by a power of 1000, with result less than 1000.
    \c DataSizeIecFormat uses the new IEC standard quantifiers Ki, Mi and so on,
    whereas \c DataSizeSIFormat uses the older SI quantifiers k, M, etc., and
    \c DataSizeTraditionalFormat abuses them.
*/
QString QLocale::formattedDataSize(qint64 bytes, int precision, DataSizeFormats format)
{
    int power, base = 1000;
    if (!bytes) {
        power = 0;
    } else if (format & DataSizeBase1000) {
        power = int(std::log10(qAbs(bytes)) / 3);
    } else { // Compute log2(bytes) / 10:
        power = int((63 - qCountLeadingZeroBits(quint64(qAbs(bytes)))) / 10);
        base = 1024;
    }
    // Only go to doubles if we'll be using a quantifier:
    const QString number = power
        ? toString(bytes / std::pow(double(base), power), 'f', qMin(precision, 3 * power))
        : toString(bytes);

    // We don't support sizes in units larger than exbibytes because
    // the number of bytes would not fit into qint64.
    Q_ASSERT(power <= 6 && power >= 0);
    QString unit;
    if (power > 0) {
        quint16 index, size;
        if (format & DataSizeSIQuantifiers) {
            index = d->m_data->m_byte_si_quantified_idx;
            size = d->m_data->m_byte_si_quantified_size;
        } else {
            index = d->m_data->m_byte_iec_quantified_idx;
            size = d->m_data->m_byte_iec_quantified_size;
        }
        unit = getLocaleListData(byte_unit_data + index, size, power - 1);
    } else {
        unit = getLocaleData(byte_unit_data + d->m_data->m_byte_idx, d->m_data->m_byte_size);
    }

    return number + QLatin1Char(' ') + unit;
}

/*!
    \since 4.8

    Returns an ordered list of locale names for translation purposes in
    preference order (like "en-Latn-US", "en-US", "en").

    The return value represents locale names that the user expects to see the
    UI translation in.

    Most like you do not need to use this function directly, but just pass the
    QLocale object to the QTranslator::load() function.

    The first item in the list is the most preferred one.

    \sa QTranslator, bcp47Name()
*/
QStringList QLocale::uiLanguages() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::UILanguages, QVariant());
        if (!res.isNull()) {
            QStringList result = res.toStringList();
            if (!result.isEmpty())
                return result;
        }
    }
#endif
    QLocaleId id = QLocaleId::fromIds(d->m_data->m_language_id, d->m_data->m_script_id, d->m_data->m_country_id);
    const QLocaleId max = id.withLikelySubtagsAdded();
    const QLocaleId min = max.withLikelySubtagsRemoved();

    QStringList uiLanguages;
    uiLanguages.append(QString::fromLatin1(min.name()));
    if (id.script_id) {
        id.script_id = 0;
        if (id != min && id.withLikelySubtagsAdded() == max)
            uiLanguages.append(QString::fromLatin1(id.name()));
    }
    if (max != min && max != id)
        uiLanguages.append(QString::fromLatin1(max.name()));
    return uiLanguages;
}

/*!
    \since 4.8

    Returns a native name of the language for the locale. For example
    "Schwiizertüütsch" for Swiss-German locale.

    \sa nativeCountryName(), languageToString()
*/
QString QLocale::nativeLanguageName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::NativeLanguageName, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(endonyms_data + d->m_data->m_language_endonym_idx, d->m_data->m_language_endonym_size);
}

/*!
    \since 4.8

    Returns a native name of the country for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName(), countryToString()
*/
QString QLocale::nativeCountryName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d->m_data == systemData()) {
        QVariant res = systemLocale()->query(QSystemLocale::NativeCountryName, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(endonyms_data + d->m_data->m_country_endonym_idx, d->m_data->m_country_endonym_size);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QLocale &l)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace().noquote()
        << "QLocale(" << QLocale::languageToString(l.language())
        << ", " << QLocale::scriptToString(l.script())
        << ", " << QLocale::countryToString(l.country()) << ')';
    return dbg;
}
#endif
QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qlocale.cpp"
#endif
