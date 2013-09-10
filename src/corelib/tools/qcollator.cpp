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

#include "qcollator.h"
#include "qstringlist.h"
#include "qstring.h"

#ifdef QT_USE_ICU
#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>
#endif

QT_BEGIN_NAMESPACE


class QCollatorPrivate
{
public:
    QAtomicInt ref;
    QLocale locale;

#ifdef QT_USE_ICU
    UCollator *collator;
#else
    void *collator;
#endif

    void clear() {
#ifdef QT_USE_ICU
        if (collator)
            ucol_close(collator);
#endif
        collator = 0;
    }

    QCollatorPrivate()
        : ref(1),
          collator(0)
    {
    }
    ~QCollatorPrivate();

    void init();

private:
    Q_DISABLE_COPY(QCollatorPrivate)
};

class QCollatorSortKeyPrivate : public QSharedData
{
    friend class QCollator;
public:
    QCollatorSortKeyPrivate(const QByteArray &key) :
        QSharedData(),
        m_key(key)
    {
    }

    QByteArray m_key;

private:
    Q_DISABLE_COPY(QCollatorSortKeyPrivate)
};

QCollatorPrivate::~QCollatorPrivate()
{
    clear();
}

/*!
    \class QCollator
    \inmodule QtCore
    \brief The QCollator class compares strings according to a localized collation algorithm.

    \since 5.2

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

    QCollator currently depends on Qt being compiled with ICU support enabled.
*/

/*!
    Constructs a QCollator from \a locale. If \a locale is not specified QLocale::default()
    is being used.

    \sa setLocale()
 */
QCollator::QCollator(const QLocale &locale)
    : d(new QCollatorPrivate)
{
    d->locale = locale;
    d->init();
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
void QCollatorPrivate::init()
{
#ifdef QT_USE_ICU
    UErrorCode status = U_ZERO_ERROR;
    QByteArray name = locale.bcp47Name().replace(QLatin1Char('-'), QLatin1Char('_')).toLocal8Bit();
    collator = ucol_open(name.constData(), &status);
    if (U_FAILURE(status))
        qWarning("Could not create collator: %d", status);

    // enable normalization by default
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
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

    d->init();
}

/*!
    Returns the locale of the collator.
 */
QLocale QCollator::locale() const
{
    return d->locale;
}

/*!
    Sets the case \a cs of the collator.
 */
void QCollator::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if (d->ref.load() != 1)
        detach();

#ifdef QT_USE_ICU
    UColAttributeValue val = (cs == Qt::CaseSensitive) ? UCOL_UPPER_FIRST : UCOL_OFF;

    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(d->collator, UCOL_CASE_FIRST, val, &status);
    if (U_FAILURE(status))
        qWarning("ucol_setAttribute: Case First failed: %d", status);
#else
    Q_UNUSED(cs);
#endif
}

/*!
    Returns case preference of the collator.
 */
Qt::CaseSensitivity QCollator::caseSensitivity() const
{
#ifdef QT_USE_ICU
    UErrorCode status = U_ZERO_ERROR;
    UColAttributeValue attribute = ucol_getAttribute(d->collator, UCOL_CASE_FIRST, &status);
    return (attribute == UCOL_OFF) ? Qt::CaseInsensitive : Qt::CaseSensitive;
#endif
    return Qt::CaseSensitive;
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

    \sa setNumericMode()
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
    If \a on is set to true, punctuation characters and symbols are ignored when determining sort order.

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

    \sa setIgnorePunctuation()
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
    Returns a sortKey for \a string.

    Creating the sort key is usually somewhat slower, than using the compare()
    methods directly. But if the string is compared repeatedly (e.g. when sorting
    a whole list of strings), it's usually faster to create the sort keys for each
    string and then sort using the keys.
 */
QCollatorSortKey QCollator::sortKey(const QString &string) const
{
    QByteArray key;
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
    key = result;
#else
    key = string.toLatin1();
#endif
    return QCollatorSortKey(new QCollatorSortKeyPrivate(key));
}

/*!
    \fn bool QCollator::operator()(const QString &s1, const QString &s2) const
    \internal
*/

/*!
    \class QCollatorSortKey
    \inmodule QtCore
    \brief The QCollatorSortKey class can be used to speed up string collation.

    \since 5.2

    The QCollatorSortKey class is always created by QCollator::sortKey()
    and is used for fast strings collation, for example when collating many strings.

    \reentrant
    \ingroup i18n
    \ingroup string-processing
    \ingroup shared

    \sa QCollator, QCollator::sortKey()
*/

/*!
    \internal
 */
QCollatorSortKey::QCollatorSortKey(QCollatorSortKeyPrivate *d)
    : d(d)
{
}

/*!
    Constructs a copy of the \a other collator sorting key.
 */
QCollatorSortKey::QCollatorSortKey(const QCollatorSortKey& other)
    : d(other.d)
{
}

/*!
    Destroys the collator key.
 */
QCollatorSortKey::~QCollatorSortKey()
{
}

/*!
    Assigns \a other to this collator key.
 */
QCollatorSortKey& QCollatorSortKey::operator=(const QCollatorSortKey &other)
{
    if (this != &other) {
        d = other.d;
    }
    return *this;
}

/*!
    According to the QCollator that created the key, returns true if the
    key should be sorted before than \a otherKey; otherwise returns false.

    \sa compare()
 */
bool QCollatorSortKey::operator<(const QCollatorSortKey &otherKey) const
{
    return d->m_key < otherKey.d->m_key;
}

/*!
    Compares the key to \a otherKey. Returns a negative value if the key
    is less than \a otherKey, 0 if the key is equal to \a otherKey or a
    positive value if the key is greater than \a otherKey.

    \sa operator<()
 */
int QCollatorSortKey::compare(const QCollatorSortKey &otherKey) const
{
    return qstrcmp(d->m_key, otherKey.d->m_key);
}

QT_END_NAMESPACE
