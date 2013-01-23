/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcollator_p.h"
#include "qstringlist.h"
#include "qstring.h"

#ifdef QT_USE_ICU
#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>
#endif

#include "qdebug.h"

QT_BEGIN_NAMESPACE


class QCollatorPrivate
{
public:
    QAtomicInt ref;
    QLocale locale;
    QCollator::Collation collation;

#ifdef QT_USE_ICU
    UCollator *collator;
#else
    void *collator;
#endif

    QStringList indexCharacters;

    void clear() {
#ifdef QT_USE_ICU
        if (collator)
            ucol_close(collator);
#endif
        collator = 0;
        indexCharacters.clear();
    }

    QCollatorPrivate()
        : collation(QCollator::Default),
          collator(0)
    { ref.store(1); }
    ~QCollatorPrivate();

private:
    Q_DISABLE_COPY(QCollatorPrivate)
};


QCollatorPrivate::~QCollatorPrivate()
{
    clear();
}

static const int collationStringsCount = 13;
static const char * const collationStrings[collationStringsCount] = {
    "default",
    "big5han",
    "dictionary",
    "direct",
    "gb2312han",
    "phonebook",
    "pinyin",
    "phonetic",
    "reformed",
    "standard",
    "stroke",
    "traditional",
    "unihan"
};

/*!
    \class QCollator
    \inmodule QtCore
    \brief The QCollator class compares strings according to a localized collation algorithm.

    \internal

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    QCollator is initialized with a QLocale and an optional collation strategy. It tries to
    initialize the collator with the specified values. The collator can then be used to compare
    and sort strings in a locale dependent fashion.

    A QCollator object can be used together with template based sorting algorithms such as std::sort
    to sort a list of QStrings.

    In addition to the locale and collation strategy, several optional flags can be set that influence
    the result of the collation.
*/

/*!
    Constructs a QCollator from \a locale and \a collation. If \a collation is not
    specified the default collation algorithm for the locale is being used. If
    \a locale is not specified QLocale::default() is being used.

    \sa setLocale, setCollation, setOptions
 */
QCollator::QCollator(const QLocale &locale, QCollator::Collation collation)
    : d(new QCollatorPrivate)
{
    d->locale = locale;
    if ((int)collation >= 0 && (int)collation < collationStringsCount)
        d->collation = collation;

    init();
}

/*!
    Creates a copy of \a other.
 */
QCollator::QCollator(const QCollator &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    Destroys the collator.
 */
QCollator::~QCollator()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    Assigns \a other to this collator.
 */
QCollator &QCollator::operator=(const QCollator &other)
{
    if (this != &other) {
        if (!d->ref.deref())
            delete d;
        d = other.d;
        d->ref.ref();
    }
    return *this;
}


/*!
    \internal
 */
void QCollator::init()
{
    Q_ASSERT((int)d->collation < collationStringsCount);
#ifdef QT_USE_ICU
    const char *collationString = collationStrings[(int)d->collation];
    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = (d->locale.bcp47Name().replace(QLatin1Char('-'), QLatin1Char('_')) + QLatin1String("@collation=") + QLatin1String(collationString)).toLatin1();
    d->collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status))
        qWarning("Could not create collator: %d", status);

    // enable normalization by default
    ucol_setAttribute(d->collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
#endif
}

/*!
    \internal
 */
void QCollator::detach()
{
    if (d->ref.load() != 1) {
        QCollatorPrivate *x = new QCollatorPrivate;
        x->ref.store(1);
        x->locale = d->locale;
        x->collation = d->collation;
        x->collator = 0;
        if (!d->ref.deref())
            delete d;
        d = x;
    }
}


/*!
    Sets the locale of the collator to \a locale.
 */
void QCollator::setLocale(const QLocale &locale)
{
    if (d->ref.load() != 1)
        detach();
    d->clear();
    d->locale = locale;

    init();
}

/*!
    Returns the locale of the collator.
 */
QLocale QCollator::locale() const
{
    return d->locale;
}

/*!
    \enum QCollator::Collation

    This enum can be used to specify an alternate collation algorithm to be used instead
    of the default algorithm for the locale.

    Possible values are:

    \value Default Use the default algorithm for the locale
    \value Big5Han
    \value Direct
    \value GB2312Han
    \value PhoneBook
    \value Pinyin
    \value Phonetic
    \value Reformed
    \value Standard
    \value Stroke
    \value Traditional
    \value UniHan
*/

/*!
    Sets the collation algorithm to be used.

    \sa QCollator::Collation
 */
void QCollator::setCollation(QCollator::Collation collation)
{
    if ((int)collation < 0 || (int)collation >= collationStringsCount)
        return;

    if (d->ref.load() != 1)
        detach();
    d->clear();
    d->collation = collation;

    init();
}
/*!
    Returns the currently used collation algorithm.

    \sa QCollator::Collation
 */
QCollator::Collation QCollator::collation() const
{
    return d->collation;
}

/*!
    Returns a unique identifer for this collation object.

    This method is helpful to save and restore defined collation
    objects.

    \sa fromIdentifier
 */
QString QCollator::identifier() const
{
    QString id = d->locale.bcp47Name();
    if (d->collation != QCollator::Default) {
        id += QLatin1String("@collation=");
        id += QLatin1String(collationStrings[d->collation]);
    }
    // this ensures the ID is compatible with ICU
    id.replace('-', '_');
    return id;
}

/*!
    Creates a QCollator from a unique identifier and returns it.

    \sa identifier
 */
QCollator QCollator::fromIdentifier(const QString &identifier)
{
    QString localeString = identifier;
    QString collationString;
    int at = identifier.indexOf(QLatin1Char('@'));
    if (at >= 0) {
        localeString = identifier.left(at);
        collationString = identifier.mid(at + strlen("@collation="));
    }

    QLocale locale(localeString);
    Collation collation = Default;
    if (!collationString.isEmpty()) {
        for (int i = 0; i < collationStringsCount; ++i) {
            if (QLatin1String(collationStrings[i]) == collationString) {
                collation = Collation(i);
                break;
            }
        }
    }
    return QCollator(locale, collation);
}

/*!
     \enum QCollator::CasePreference

    This enum can be used to tailor the case preference during collation.

    \value CasePreferenceOff No case preference, use what is the standard for the locale
    \value CasePreferenceUpper Sort upper case characters before lower case
    \value CasePreferenceLower Sort lower case characters before upper case
*/

/*!
    Sets the case preference of the collator.

    \sa QCollator::CasePreference
 */
void QCollator::setCasePreference(CasePreference c)
{
    if (d->ref.load() != 1)
        detach();

#ifdef QT_USE_ICU
    UColAttributeValue val = UCOL_OFF;
    if (c == QCollator::CasePreferenceUpper)
        val = UCOL_UPPER_FIRST;
    else if (c == QCollator::CasePreferenceLower)
        val = UCOL_LOWER_FIRST;

    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(d->collator, UCOL_CASE_FIRST, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);
#else
    Q_UNUSED(c);
#endif
}

/*!
    Returns case preference of the collator.

    \sa QCollator::CasePreference
 */
QCollator::CasePreference QCollator::casePreference() const
{
#ifdef QT_USE_ICU
    UErrorCode status = U_ZERO_ERROR;
    switch (ucol_getAttribute(d->collator, UCOL_CASE_FIRST, &status)) {
    case UCOL_UPPER_FIRST:
        return QCollator::CasePreferenceUpper;
    case UCOL_LOWER_FIRST:
        return QCollator::CasePreferenceLower;
    case UCOL_OFF:
    default:
        break;
    }
#endif
    return QCollator::CasePreferenceOff;
}

/*!
    Enables numeric sorting mode when \a on is set to true.

    This will enable proper sorting of numeric digits, so that e.g. 100 sorts after 99.

    By default this mode is off.
 */
void QCollator::setNumericMode(bool on)
{
    if (d->ref.load() != 1)
        detach();

#ifdef QT_USE_ICU
    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(d->collator, UCOL_NUMERIC_COLLATION, on ? UCOL_ON : UCOL_OFF, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: numeric collation failed: %d", status);
#else
    Q_UNUSED(on);
#endif
}

/*!
    Returns true if numeric sorting is enabled, false otherwise.

    \sa setNumericMode
 */
bool QCollator::numericMode() const
{
#ifdef QT_USE_ICU
    UErrorCode status;
    if (ucol_getAttribute(d->collator, UCOL_NUMERIC_COLLATION, &status) == UCOL_ON)
        return true;
#endif
    return false;
}

/*!
    If set to true, punctuation characters and symbols are ignored when determining sort order.

    The default is locale dependent.
 */
void QCollator::setIgnorePunctuation(bool on)
{
    if (d->ref.load() != 1)
        detach();

#ifdef QT_USE_ICU
    UErrorCode status;
    ucol_setAttribute(d->collator, UCOL_ALTERNATE_HANDLING, on ? UCOL_SHIFTED : UCOL_NON_IGNORABLE, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Alternate handling failed: %d", status);
#else
    Q_UNUSED(on);
#endif
}

/*!
    Returns true if punctuation characters and symbols are ignored when determining sort order.

    \sa setIgnorePunctuation
 */
bool QCollator::ignorePunctuation() const
{
#ifdef QT_USE_ICU
    UErrorCode status;
    if (ucol_getAttribute(d->collator, UCOL_ALTERNATE_HANDLING, &status) == UCOL_SHIFTED)
        return true;
#endif
    return false;
}

/*!
    Compares \a s1 with \a s2. Returns -1, 0 or 1 depending on whether \a s1 is
    smaller, equal or larger than \a s2.
 */
int QCollator::compare(const QString &s1, const QString &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

/*!
    \overload

    Compares \a s1 with \a s2. Returns -1, 0 or 1 depending on whether \a s1 is
    smaller, equal or larger than \a s2.
 */
int QCollator::compare(const QStringRef &s1, const QStringRef &s2) const
{
    return compare(s1.constData(), s1.size(), s2.constData(), s2.size());
}

/*!
    \overload

    Compares \a s1 with \a s2. \a len1 and \a len2 specify the length of the
    QChar arrays pointer to by \a s1 and \a s2.

    Returns -1, 0 or 1 depending on whether \a s1 is smaller, equal or larger than \a s2.
 */
int QCollator::compare(const QChar *s1, int len1, const QChar *s2, int len2) const
{
#ifdef QT_USE_ICU
    const UCollationResult result =
            ucol_strcoll(d->collator, (const UChar *)s1, len1, (const UChar *)s2, len2);
    return result;
#else
    return QString::compare_helper((const QChar *)s1, len1, (const QChar *)s2, len2, Qt::CaseInsensitive);
#endif
}

/*!
    Returns a sortKey for \a string. The sortkey can be used as a placeholder
    for the string that can be then sorted using regular strcmp based sorting.

    Creating the sort key is usually somewhat slower, then using the compare()
    methods directly. But if the string is compared repeatedly (e.g. when sorting
    a whole list of strings), it's usually faster to create the sort keys for each
    string and then sort using the keys.
 */
QByteArray QCollator::sortKey(const QString &string) const
{
#ifdef QT_USE_ICU
    QByteArray result(16 + string.size() + (string.size() >> 2), Qt::Uninitialized);
    int size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                               string.size(), (uint8_t *)result.data(), result.size());
    if (size > result.size()) {
        result.resize(size);
        size = ucol_getSortKey(d->collator, (const UChar *)string.constData(),
                               string.size(), (uint8_t *)result.data(), result.size());
    }
    result.truncate(size);
    return result;
#else
    return string.toLower().toUtf8();
#endif
}

static QStringList englishIndexCharacters()
{
    QString chars = QString::fromLatin1("A B C D E F G H I J K L M N O P Q R S T U V W X Y Z");
    return chars.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

/*!
    Returns a string list of primary index characters. This is useful when presenting the
    sorted list in a user interface with section headers.
*/
QStringList QCollator::indexCharacters() const
{
    if (!d->indexCharacters.isEmpty())
        return d->indexCharacters;

#ifdef QT_USE_ICU
    QByteArray id = identifier().toLatin1();

    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *res = ures_open(NULL, id, &status);

    if (U_FAILURE(status)) {
        d->indexCharacters = englishIndexCharacters();
    } else {

        qint32 len = 0;
        status = U_ZERO_ERROR;
        const UChar *val = ures_getStringByKey(res, "ExemplarCharactersIndex", &len, &status);
        if (U_FAILURE(status)) {
            d->indexCharacters = englishIndexCharacters();
        } else {
            QString chars(reinterpret_cast<const QChar *>(val), len);
            chars.remove('[');
            chars.remove(']');
            chars.remove('{');
            chars.remove('}');
            d->indexCharacters = chars.split(QLatin1Char(' '), QString::SkipEmptyParts);
        }
    }

    ures_close(res);
#else
    d->indexCharacters = englishIndexCharacters();
#endif

    return d->indexCharacters;
}

QT_END_NAMESPACE
