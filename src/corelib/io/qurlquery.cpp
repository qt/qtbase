// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qurlquery.h"
#include "qurl_p.h"

#include <QtCore/qhashfunctions.h>
#include <QtCore/qstringlist.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
  \class QUrlQuery
  \inmodule QtCore
  \since 5.0

  \brief The QUrlQuery class provides a way to manipulate a key-value pairs in
  a URL's query.

  \reentrant
  \ingroup io
  \ingroup network
  \ingroup shared

  It is used to parse the query strings found in URLs like the following:

  \image qurl-querystring.png

  Query strings like the above are used to transmit options in the URL and are
  usually decoded into multiple key-value pairs. The one above would contain
  two entries in its list, with keys "type" and "color". QUrlQuery can also be
  used to create a query string suitable for use in QUrl::setQuery() from the
  individual components of the query.

  The most common way of parsing a query string is to initialize it in the
  constructor by passing it the query string. Otherwise, the setQuery() method
  can be used to set the query to be parsed. That method can also be used to
  parse a query with non-standard delimiters, after having set them using the
  setQueryDelimiters() function.

  The encoded query string can be obtained again using query(). This will take
  all the internally-stored items and encode the string using the delimiters.

  \section1 Encoding

  All of the getter methods in QUrlQuery support an optional parameter of type
  QUrl::ComponentFormattingOptions, including query(), which dictate how to
  encode the data in question. Except for QUrl::FullyDecoded, the returned value must
  still be considered a percent-encoded string, as there are certain values
  which cannot be expressed in decoded form (like control characters, byte
  sequences not decodable to UTF-8). For that reason, the percent character is
  always represented by the string "%25".

  All of the setter methods and the query methods like hasQueryItem() in
  QUrlQuery take encoded forms only. Unlike in QUrl, there's no optional
  parameter to specify that the strings being passed are decoded. If
  improperly-encoded strings are passed to the setter or query methods,
  QUrlQuery will attempt to recover instead of failing. That is to say, all
  functions in this class parse their string arguments as if the
  QUrl::TolerantMode decoding mode was specified.

  Application code should strive to always ensure proper encoding and not rely
  on TolerantMode parsing fixing the strings. Notably, all user input must be
  first percent-encoded using QUrl::toPercentEncoding() or similar functions
  before being passed to the functions in this class.

  \section2 Handling of spaces and plus ("+")

  Web browsers usually encode spaces found in HTML FORM elements to a plus sign
  ("+") and plus signs to its percent-encoded form (%2B). However, the Internet
  specifications governing URLs do not consider spaces and the plus character
  equivalent.

  For that reason, QUrlQuery never encodes the space character to "+" and will
  never decode "+" to a space character. Instead, space characters will be
  rendered "%20" in encoded form.

  To support encoding like that of HTML forms, QUrlQuery also never decodes the
  "%2B" sequence to a plus sign nor encode a plus sign. In fact, any "%2B" or
  "+" sequences found in the keys, values, or query string are left exactly
  like written (except for the uppercasing of "%2b" to "%2B").

  \section2 Full decoding

  With QUrl::FullyDecoded formatting, all percent-encoded sequences will be
  decoded fully and the '%' character is used to represent itself.
  QUrl::FullyDecoded should be used with care, since it may cause data loss.
  See the documentation of QUrl::FullyDecoded for information on what data may
  be lost.

  This formatting mode should be used only when dealing with text presented to
  the user in contexts where percent-encoding is not desired. Note that
  QUrlQuery setters and query methods do not support the counterpart
  QUrl::DecodedMode parsing, so using QUrl::FullyDecoded to obtain a listing of
  keys may result in keys not found in the object.

  \section1 Non-standard delimiters

  By default, QUrlQuery uses an equal sign ("=") to separate a key from its
  value, and an ampersand ("&") to separate key-value pairs from each other. It
  is possible to change the delimiters that QUrlQuery uses for parsing and for
  reconstructing the query by calling setQueryDelimiters().

  Non-standard delimiters should be chosen from among what RFC 3986 calls
  "sub-delimiters". They are:

  \snippet code/src_corelib_io_qurlquery.cpp 0

  Use of other characters is not supported and may result in unexpected
  behaviour. QUrlQuery does not verify that you passed a valid delimiter.

  \sa QUrl
*/

/*!
    \fn QUrlQuery &QUrlQuery::operator=(QUrlQuery &&other)

    Move-assigns \a other to this QUrlQuery instance.

    \since 5.2
*/

/*!
    \fn QUrlQuery::QUrlQuery(std::initializer_list<QPair<QString, QString>> list)

    \since 5.13

    Constructs a QUrlQuery object from the \a list of key/value pair.
*/

typedef QList<QPair<QString, QString> > Map;

class QUrlQueryPrivate : public QSharedData
{
public:
    QUrlQueryPrivate(const QString &query = QString())
        : valueDelimiter(QUrlQuery::defaultQueryValueDelimiter()),
          pairDelimiter(QUrlQuery::defaultQueryPairDelimiter())
    { if (!query.isEmpty()) setQuery(query); }

    QString recodeFromUser(const QString &input) const;
    QString recodeToUser(const QString &input, QUrl::ComponentFormattingOptions encoding) const;

    void setQuery(const QString &query);

    void addQueryItem(const QString &key, const QString &value)
    { itemList.append(qMakePair(recodeFromUser(key), recodeFromUser(value))); }
    int findRecodedKey(const QString &key, int from = 0) const
    {
        for (int i = from; i < itemList.size(); ++i)
            if (itemList.at(i).first == key)
                return i;
        return itemList.size();
    }
    Map::const_iterator findKey(const QString &key) const
    { return itemList.constBegin() + findRecodedKey(recodeFromUser(key)); }
    Map::iterator findKey(const QString &key)
    { return itemList.begin() + findRecodedKey(recodeFromUser(key)); }

    Map itemList;
    QChar valueDelimiter;
    QChar pairDelimiter;
};

template<> void QSharedDataPointer<QUrlQueryPrivate>::detach()
{
    if (d && d->ref.loadRelaxed() == 1)
        return;
    QUrlQueryPrivate *x = (d ? new QUrlQueryPrivate(*d)
                             : new QUrlQueryPrivate);
    x->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = x;
}

// Here's how we do the encoding in QUrlQuery
// The RFC says these are the delimiters:
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="
// And the definition of query is:
//    query         = *( pchar / "/" / "?" )
//    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
//
// The strict definition of query says that it can have unencoded any
// unreserved, sub-delim, ":", "@", "/" and "?". Or, by exclusion, excluded
// delimiters are "#", "[" and "]" -- if those are present, they must be
// percent-encoded. The fact that "[" and "]" should be encoded is probably a
// mistake in the spec, so we ignore it and leave the decoded.
//
// The internal storage in the Map is equivalent to PrettyDecoded. That means
// the getter methods, when called with the default encoding value, will not
// have to recode anything (except for toString()).
//
// QUrlQuery handling of delimiters is quite simple: we never touch any of
// them, except for the "#" character and the pair and value delimiters. Those
// are always kept in their decoded forms.
//
// But when recreating the query string, in toString(), we must take care of
// the special delimiters: the pair and value delimiters, as well as the "#"
// character if unambiguous decoding is requested.

#define decode(x) ushort(x)
#define leave(x)  ushort(0x100 | (x))
#define encode(x) ushort(0x200 | (x))

inline QString QUrlQueryPrivate::recodeFromUser(const QString &input) const
{
    // note: duplicated in setQuery()
    QString output;
    ushort prettyDecodedActions[] = {
        decode(pairDelimiter.unicode()),
        decode(valueDelimiter.unicode()),
        decode('#'),
        0
    };
    if (qt_urlRecode(output, input,
                     QUrl::DecodeReserved,
                     prettyDecodedActions))
        return output;
    return input;
}

inline bool idempotentRecodeToUser(QUrl::ComponentFormattingOptions encoding)
{
    return encoding == QUrl::PrettyDecoded;
}

inline QString QUrlQueryPrivate::recodeToUser(const QString &input, QUrl::ComponentFormattingOptions encoding) const
{
    // our internal formats are stored in "PrettyDecoded" form
    // and there are no ambiguous characters
    if (idempotentRecodeToUser(encoding))
        return input;

    if (!(encoding & QUrl::EncodeDelimiters)) {
        QString output;
        if (qt_urlRecode(output, input,
                         encoding, nullptr))
            return output;
        return input;
    }

    // re-encode the "#" character and the query delimiter pair
    ushort actions[] = { encode(pairDelimiter.unicode()), encode(valueDelimiter.unicode()),
                         encode('#'), 0 };
    QString output;
    if (qt_urlRecode(output, input, encoding, actions))
        return output;
    return input;
}

void QUrlQueryPrivate::setQuery(const QString &query)
{
    ushort prettyDecodedActions[] = {
        decode(pairDelimiter.unicode()),
        decode(valueDelimiter.unicode()),
        decode('#'),
        0
    };

    itemList.clear();
    const QChar *pos = query.constData();
    const QChar *const end = pos + query.size();
    while (pos != end) {
        const QChar *begin = pos;
        const QChar *delimiter = nullptr;
        while (pos != end) {
            // scan for the component parts of this pair
            if (!delimiter && *pos == valueDelimiter)
                delimiter = pos;
            if (*pos == pairDelimiter)
                break;
            ++pos;
        }
        if (!delimiter)
            delimiter = pos;

        // pos is the end of this pair (the end of the string or the pair delimiter)
        // delimiter points to the value delimiter or to the end of this pair

        QString key;
        if (!qt_urlRecode(key, QStringView{begin, delimiter},
                          QUrl::DecodeReserved,
                          prettyDecodedActions))
            key = QString(begin, delimiter - begin);

        if (delimiter == pos) {
            // the value delimiter wasn't found, store a null value
            itemList.append(qMakePair(key, QString()));
        } else if (delimiter + 1 == pos) {
            // if the delimiter was found but the value is empty, store empty-but-not-null
            itemList.append(qMakePair(key, QString(0, Qt::Uninitialized)));
        } else {
            QString value;
            if (!qt_urlRecode(value, QStringView{delimiter + 1, pos},
                              QUrl::DecodeReserved,
                              prettyDecodedActions))
                value = QString(delimiter + 1, pos - delimiter - 1);
            itemList.append(qMakePair(key, value));
        }

        if (pos != end)
            ++pos;
    }
}

// allow QUrlQueryPrivate to detach from null
template <> inline QUrlQueryPrivate *
QSharedDataPointer<QUrlQueryPrivate>::clone()
{
    return d ? new QUrlQueryPrivate(*d) : new QUrlQueryPrivate;
}

/*!
    Constructs an empty QUrlQuery object. A query can be set afterwards by
    calling setQuery() or items can be added by using addQueryItem().

    \sa setQuery(), addQueryItem()
*/
QUrlQuery::QUrlQuery()
    : d(nullptr)
{
}

/*!
    Constructs a QUrlQuery object and parses the \a queryString query string,
    using the default query delimiters. To parse a query string using other
    delimiters, you should first set them using setQueryDelimiters() and then
    set the query with setQuery().
*/
QUrlQuery::QUrlQuery(const QString &queryString)
    : d(queryString.isEmpty() ? nullptr : new QUrlQueryPrivate(queryString))
{
}

/*!
    Constructs a QUrlQuery object and parses the query string found in the \a
    url URL, using the default query delimiters. To parse a query string using
    other delimiters, you should first set them using setQueryDelimiters() and
    then set the query with setQuery().

    \sa QUrl::query()
*/
QUrlQuery::QUrlQuery(const QUrl &url)
    : d(nullptr)
{
    // use internals to avoid unnecessary recoding
    // ### FIXME: actually do it
    if (url.hasQuery())
        d = new QUrlQueryPrivate(url.query());
}

/*!
    Copies the contents of the \a other QUrlQuery object, including the query
    delimiters.
*/
QUrlQuery::QUrlQuery(const QUrlQuery &other)
    : d(other.d)
{
}

/*!
    \since 6.5
    Moves the contents of the \a other QUrlQuery object, including the query
    delimiters.
*/
QUrlQuery::QUrlQuery(QUrlQuery &&other) noexcept
    : d(std::move(other.d))
{
}

/*!
    Copies the contents of the \a other QUrlQuery object, including the query
    delimiters.
*/
QUrlQuery &QUrlQuery::operator =(const QUrlQuery &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QUrlQuery::swap(QUrlQuery &other)

    Swaps this URL query instance with \a other. This function is very
    fast and never fails.
*/

/*!
    Destroys this QUrlQuery object.
*/
QUrlQuery::~QUrlQuery()
{
    // d auto-deletes
}

/*!
    Returns \c true if this object and the \a other object contain the same
    contents, in the same order, and use the same query delimiters.
*/
bool QUrlQuery::operator ==(const QUrlQuery &other) const
{
    if (d == other.d)
        return true;
    if (d && other.d)
        // keep in sync with qHash(QUrlQuery):
        return d->valueDelimiter == other.d->valueDelimiter &&
                d->pairDelimiter == other.d->pairDelimiter &&
                d->itemList == other.d->itemList;

    const QUrlQueryPrivate *x = d ? d.data() : other.d.data();
    return x->valueDelimiter == defaultQueryValueDelimiter() &&
            x->pairDelimiter == defaultQueryPairDelimiter() &&
            x->itemList.isEmpty();
}

/*!
    \since 5.6
    \relates QUrlQuery

    Returns the hash value for \a key,
    using \a seed to seed the calculation.
*/
size_t qHash(const QUrlQuery &key, size_t seed) noexcept
{
    if (const QUrlQueryPrivate *d = key.d) {
        QtPrivate::QHashCombine hash;
        // keep in sync with operator==:
        seed = hash(seed, d->valueDelimiter);
        seed = hash(seed, d->pairDelimiter);
        seed = hash(seed, d->itemList);
    }
    return seed;
}

/*!
    Returns \c true if this QUrlQuery object contains no key-value pairs, such as
    after being default-constructed or after parsing an empty query string.

    \sa setQuery(), clear()
*/
bool QUrlQuery::isEmpty() const
{
    return d ? d->itemList.isEmpty() : true;
}

/*!
    \internal
*/
bool QUrlQuery::isDetached() const
{
    return d && d->ref.loadRelaxed() == 1;
}

/*!
    Clears this QUrlQuery object by removing all of the key-value pairs
    currently stored. If the query delimiters have been changed, this function
    will leave them with their changed values.

    \sa isEmpty(), setQueryDelimiters()
*/
void QUrlQuery::clear()
{
    if (d.constData())
        d->itemList.clear();
}

/*!
    Parses the query string in \a queryString and sets the internal items to
    the values found there. If any delimiters have been specified with
    setQueryDelimiters(), this function will use them instead of the default
    delimiters to parse the string.
*/
void QUrlQuery::setQuery(const QString &queryString)
{
    d->setQuery(queryString);
}

static void recodeAndAppend(QString &to, const QString &input,
                            QUrl::ComponentFormattingOptions encoding, const ushort *tableModifications)
{
    if (!qt_urlRecode(to, input, encoding, tableModifications))
        to += input;
}

/*!
    Returns the reconstructed query string, formed from the key-value pairs
    currently stored in this QUrlQuery object and separated by the query
    delimiters chosen for this object. The keys and values are encoded using
    the options given by the \a encoding parameter.

    For this function, the only ambiguous delimiter is the hash ("#"), as in
    URLs it is used to separate the query string from the fragment that may
    follow.

    The order of the key-value pairs in the returned string is exactly the same
    as in the original query.

    \sa setQuery(), QUrl::setQuery(), QUrl::fragment(), {encoding}{Encoding}
*/
QString QUrlQuery::query(QUrl::ComponentFormattingOptions encoding) const
{
    if (!d)
        return QString();

    // unlike the component encoding, for the whole query we need to modify a little:
    //  - the "#" character is unambiguous, so we encode it in EncodeDelimiters mode
    //  - the query delimiter pair must always be encoded

    // start with what's always encoded
    ushort tableActions[] = {
        encode(d->pairDelimiter.unicode()),  // 0
        encode(d->valueDelimiter.unicode()), // 1
        0,                                   // 2
        0
    };
    if (encoding & QUrl::EncodeDelimiters) {
        tableActions[2] = encode('#');
    }

    QString result;
    Map::const_iterator it = d->itemList.constBegin();
    Map::const_iterator end = d->itemList.constEnd();

    {
        int size = 0;
        for ( ; it != end; ++it)
            size += it->first.size() + 1 + it->second.size() + 1;
        result.reserve(size + size / 4);
    }

    for (it = d->itemList.constBegin(); it != end; ++it) {
        if (!result.isEmpty())
            result += QChar(d->pairDelimiter);
        recodeAndAppend(result, it->first, encoding, tableActions);
        if (!it->second.isNull()) {
            result += QChar(d->valueDelimiter);
            recodeAndAppend(result, it->second, encoding, tableActions);
        }
    }
    return result;
}

/*!
    Sets the characters used for delimiting between keys and values,
    and between key-value pairs in the URL's query string. The default
    value delimiter is '=' and the default pair delimiter is '&'.

    \image qurl-querystring.png

    \a valueDelimiter will be used for separating keys from values,
    and \a pairDelimiter will be used to separate key-value pairs.
    Any occurrences of these delimiting characters in the encoded
    representation of the keys and values of the query string are
    percent encoded when returned in query().

    If \a valueDelimiter is set to '(' and \a pairDelimiter is ')',
    the above query string would instead be represented like this:

    \snippet code/src_corelib_io_qurl.cpp 4

    \note Non-standard delimiters should be chosen from among what RFC 3986 calls
    "sub-delimiters". They are:

    \snippet code/src_corelib_io_qurlquery.cpp 0

    Use of other characters is not supported and may result in unexpected
    behaviour. This method does not verify that you passed a valid delimiter.

    \sa queryValueDelimiter(), queryPairDelimiter()
*/
void QUrlQuery::setQueryDelimiters(QChar valueDelimiter, QChar pairDelimiter)
{
    d->valueDelimiter = valueDelimiter;
    d->pairDelimiter = pairDelimiter;
}

/*!
    Returns the character used to delimit between keys and values when
    reconstructing the query string in query() or when parsing in setQuery().

    \sa setQueryDelimiters(), queryPairDelimiter()
*/
QChar QUrlQuery::queryValueDelimiter() const
{
    return d ? d->valueDelimiter : defaultQueryValueDelimiter();
}

/*!
    Returns the character used to delimit between keys-value pairs when
    reconstructing the query string in query() or when parsing in setQuery().

    \sa setQueryDelimiters(), queryValueDelimiter()
*/
QChar QUrlQuery::queryPairDelimiter() const
{
    return d ? d->pairDelimiter : defaultQueryPairDelimiter();
}

/*!
    Sets the items in this QUrlQuery object to \a query. The order of the
    elements in \a query is preserved.

    \note This method does not treat spaces (ASCII 0x20) and plus ("+") signs
    as the same, like HTML forms do. If you need spaces to be represented as
    plus signs, use actual plus signs.

    \note The keys and values are expected to be in percent-encoded form.

    \sa queryItems(), isEmpty()
*/
void QUrlQuery::setQueryItems(const QList<QPair<QString, QString> > &query)
{
    clear();
    if (query.isEmpty())
        return;

    QUrlQueryPrivate *dd = d;
    QList<QPair<QString, QString> >::const_iterator it = query.constBegin(),
            end = query.constEnd();
    for ( ; it != end; ++it)
        dd->addQueryItem(it->first, it->second);
}

/*!
    Returns the query string of the URL, as a map of keys and values, using the
    options specified in \a encoding to encode the items. The order of the
    elements is the same as the one found in the query string or set with
    setQueryItems().

    \sa setQueryItems(), {encoding}{Encoding}
*/
QList<QPair<QString, QString> > QUrlQuery::queryItems(QUrl::ComponentFormattingOptions encoding) const
{
    if (!d)
        return QList<QPair<QString, QString> >();
    if (idempotentRecodeToUser(encoding))
        return d->itemList;

    QList<QPair<QString, QString> > result;
    Map::const_iterator it = d->itemList.constBegin();
    Map::const_iterator end = d->itemList.constEnd();
    result.reserve(d->itemList.size());
    for ( ; it != end; ++it)
        result << qMakePair(d->recodeToUser(it->first, encoding),
                            d->recodeToUser(it->second, encoding));
    return result;
}

/*!
    Returns \c true if there is a query string pair whose key is equal
    to \a key from the URL.

    \note The key expected to be in percent-encoded form.

    \sa addQueryItem(), queryItemValue()
*/
bool QUrlQuery::hasQueryItem(const QString &key) const
{
    if (!d)
        return false;
    return d->findKey(key) != d->itemList.constEnd();
}

/*!
    Appends the pair \a key = \a value to the end of the query string of the
    URL. This method does not overwrite existing items that might exist with
    the same key.

    \note This method does not treat spaces (ASCII 0x20) and plus ("+") signs
    as the same, like HTML forms do. If you need spaces to be represented as
    plus signs, use actual plus signs.

    \note The key and value strings are expected to be in percent-encoded form.

    \sa hasQueryItem(), queryItemValue()
*/
void QUrlQuery::addQueryItem(const QString &key, const QString &value)
{
    d->addQueryItem(key, value);
}

/*!
    Returns the query value associated with key \a key from the URL, using the
    options specified in \a encoding to encode the return value. If the key \a
    key is not found, this function returns an empty string. If you need to
    distinguish between an empty value and a non-existent key, you should check
    for the key's presence first using hasQueryItem().

    If the key \a key is multiply defined, this function will return the first
    one found, in the order they were present in the query string or added
    using addQueryItem().

    \note The key is expected to be in percent-encoded form.

    \sa addQueryItem(), allQueryItemValues(), {encoding}{Encoding}
*/
QString QUrlQuery::queryItemValue(const QString &key, QUrl::ComponentFormattingOptions encoding) const
{
    QString result;
    if (d) {
        Map::const_iterator it = d->findKey(key);
        if (it != d->itemList.constEnd())
            result = d->recodeToUser(it->second, encoding);
    }
    return result;
}

/*!
    Returns the a list of query string values whose key is equal to \a key from
    the URL, using the options specified in \a encoding to encode the return
    value. If the key \a key is not found, this function returns an empty list.

    \note The key is expected to be in percent-encoded form.

    \sa queryItemValue(), addQueryItem()
*/
QStringList QUrlQuery::allQueryItemValues(const QString &key, QUrl::ComponentFormattingOptions encoding) const
{
    QStringList result;
    if (d) {
        QString encodedKey = d->recodeFromUser(key);
        int idx = d->findRecodedKey(encodedKey);
        while (idx < d->itemList.size()) {
            result << d->recodeToUser(d->itemList.at(idx).second, encoding);
            idx = d->findRecodedKey(encodedKey, idx + 1);
        }
    }
    return result;
}

/*!
    Removes the query string pair whose key is equal to \a key from the URL. If
    there are multiple items with a key equal to \a key, it removes the first
    item in the order they were present in the query string or added with
    addQueryItem().

    \note The key is expected to be in percent-encoded form.

    \sa removeAllQueryItems()
*/
void QUrlQuery::removeQueryItem(const QString &key)
{
    if (d.constData()) {
        auto *p = d.data();
        Map::iterator it = p->findKey(key);
        if (it != p->itemList.end())
            p->itemList.erase(it);
    }
}

/*!
    Removes all the query string pairs whose key is equal to \a key
    from the URL.

    \note The key is expected to be in percent-encoded form.

    \sa removeQueryItem()
*/
void QUrlQuery::removeAllQueryItems(const QString &key)
{
    if (d.constData()) {
        auto *p = d.data();
        const QString encodedKey = p->recodeFromUser(key);
        auto firstEqualsEncodedKey = [&encodedKey](const QPair<QString, QString> &item) {
            return item.first == encodedKey;
        };
        p->itemList.removeIf(firstEqualsEncodedKey);
    }
}

/*!
    \fn QUrlQuery::defaultQueryValueDelimiter()
    Returns the default character for separating keys from values in the query,
    an equal sign ("=").

    \note Prior to Qt 6, this function returned QChar.

    \sa setQueryDelimiters(), queryValueDelimiter(), defaultQueryPairDelimiter()
*/

/*!
    \fn QUrlQuery::defaultQueryPairDelimiter()
    Returns the default character for separating keys-value pairs from each
    other, an ampersand ("&").

    \note Prior to Qt 6, this function returned QChar.

    \sa setQueryDelimiters(), queryPairDelimiter(), defaultQueryValueDelimiter()
*/

/*!
    \typedef QUrlQuery::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QUrlQuery::data_ptr()
    \internal
*/

/*!
    \fn QString QUrlQuery::toString(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const

    Returns this QUrlQuery as a QString. \a encoding can be used to specify the URL string encoding of the return value.
*/

/*!
    \fn bool QUrlQuery::operator!=(const QUrlQuery &other) const

    Returns \c true if \a other is not equal to this QUrlQuery. Otherwise, returns \c false.

    \sa operator==()
*/
QT_END_NAMESPACE

#undef decode
#undef leave
#undef encode
