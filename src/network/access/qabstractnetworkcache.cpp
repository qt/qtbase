// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qabstractnetworkcache.h"
#include "qabstractnetworkcache_p.h"
#include "qnetworkrequest_p.h"
#include "qhttpheadershelper_p.h"

#include <qdatastream.h>
#include <qdatetime.h>
#include <qurl.h>
#include <qhash.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QNetworkCacheMetaDataPrivate : public QSharedData
{

public:
    QNetworkCacheMetaDataPrivate()
        : QSharedData()
        , saveToDisk(true)
    {}

    bool operator==(const QNetworkCacheMetaDataPrivate &other) const
    {
        return
            url == other.url
            && lastModified == other.lastModified
            && expirationDate == other.expirationDate
            && saveToDisk == other.saveToDisk
            && QHttpHeadersHelper::compareStrict(headers, other.headers);
    }

    QUrl url;
    QDateTime lastModified;
    QDateTime expirationDate;
    QHttpHeaders headers;
    QNetworkCacheMetaData::AttributesMap attributes;
    bool saveToDisk;

    static void save(QDataStream &out, const QNetworkCacheMetaData &metaData);
    static void load(QDataStream &in, QNetworkCacheMetaData &metaData);
};
Q_GLOBAL_STATIC(QNetworkCacheMetaDataPrivate, metadata_shared_invalid)

/*!
    \class QNetworkCacheMetaData
    \since 4.5
    \ingroup shared
    \inmodule QtNetwork

    \brief The QNetworkCacheMetaData class provides cache information.

    QNetworkCacheMetaData provides information about a cache file including
    the url, when it was last modified, when the cache file was created, headers
    for file and if the file should be saved onto a disk.

    \sa QAbstractNetworkCache
*/

/*!
    \typedef QNetworkCacheMetaData::RawHeader

    Synonym for std::pair<QByteArray, QByteArray>
*/

/*!
    \typedef QNetworkCacheMetaData::RawHeaderList

    Synonym for QList<RawHeader>
*/

/*!
  \typedef  QNetworkCacheMetaData::AttributesMap

  Synonym for QHash<QNetworkRequest::Attribute, QVariant>
*/

/*!
    Constructs an invalid network cache meta data.

    \sa isValid()
 */
QNetworkCacheMetaData::QNetworkCacheMetaData()
    : d(new QNetworkCacheMetaDataPrivate)
{
}

/*!
    Destroys the network cache meta data.
 */
QNetworkCacheMetaData::~QNetworkCacheMetaData()
{
    // QSharedDataPointer takes care of freeing d
}

/*!
    Constructs a copy of the \a other QNetworkCacheMetaData.
 */
QNetworkCacheMetaData::QNetworkCacheMetaData(const QNetworkCacheMetaData &other)
    : d(other.d)
{
}

/*!
    Makes a copy of the \a other QNetworkCacheMetaData and returns a reference to the copy.
 */
QNetworkCacheMetaData &QNetworkCacheMetaData::operator=(const QNetworkCacheMetaData &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkCacheMetaData::swap(QNetworkCacheMetaData &other)
    \since 5.0
    \memberswap{metadata instance}
 */

/*!
    Returns \c true if this meta data is equal to the \a other meta data; otherwise returns \c false.

    \sa operator!=()
 */
bool QNetworkCacheMetaData::operator==(const QNetworkCacheMetaData &other) const
{
    if (d == other.d)
        return true;
    if (d && other.d)
        return *d == *other.d;
    return false;
}

/*!
    \fn bool QNetworkCacheMetaData::operator!=(const QNetworkCacheMetaData &other) const

    Returns \c true if this meta data is not equal to the \a other meta data; otherwise returns \c false.

    \sa operator==()
 */

/*!
    Returns \c true if this network cache meta data has attributes that have been set otherwise false.
 */
bool QNetworkCacheMetaData::isValid() const
{
    return !(*d == *metadata_shared_invalid());
}

/*!
    Returns is this cache should be allowed to be stored on disk.

    Some cache implementations can keep these cache items in memory for performance reasons,
    but for security reasons they should not be written to disk.

    Specifically with http, documents with Cache-control set to no-store or any
    https document that doesn't have "Cache-control: public" set will
    set the saveToDisk to false.

    \sa setSaveToDisk()
 */
bool QNetworkCacheMetaData::saveToDisk() const
{
    return d->saveToDisk;
}

/*!
    Sets whether this network cache meta data and associated content should be
    allowed to be stored on disk to \a allow.

    \sa saveToDisk()
 */
void QNetworkCacheMetaData::setSaveToDisk(bool allow)
{
    d->saveToDisk = allow;
}

/*!
    Returns the URL this network cache meta data is referring to.

    \sa setUrl()
 */
QUrl QNetworkCacheMetaData::url() const
{
    return d->url;
}

/*!
    Sets the URL this network cache meta data to be \a url.

    The password and fragment are removed from the url.

    \sa url()
 */
void QNetworkCacheMetaData::setUrl(const QUrl &url)
{
    auto *p = d.data();
    p->url = url;
    p->url.setPassword(QString());
    p->url.setFragment(QString());
}

/*!
    Returns a list of all raw headers that are set in this meta data.
    The list is in the same order that the headers were set.

    \sa setRawHeaders(), headers()
 */
QNetworkCacheMetaData::RawHeaderList QNetworkCacheMetaData::rawHeaders() const
{
    return QNetworkHeadersPrivate::fromHttpToRaw(d->headers);
}

/*!
    Sets the raw headers to \a list.

    \sa rawHeaders(), setHeaders()
 */
void QNetworkCacheMetaData::setRawHeaders(const RawHeaderList &list)
{
    d->headers = QNetworkHeadersPrivate::fromRawToHttp(list);
}

/*!
    \since 6.8

    Returns headers in form of QHttpHeaders that are set in this meta data.

    \sa setHeaders()
*/
QHttpHeaders QNetworkCacheMetaData::headers() const
{
    return d->headers;
}

/*!
    \since 6.8

    Sets the headers of this network cache meta data to \a headers.

    \sa headers()
*/
void QNetworkCacheMetaData::setHeaders(const QHttpHeaders &headers)
{
    d->headers = headers;
}

/*!
    Returns the date and time when the meta data was last modified.
 */
QDateTime QNetworkCacheMetaData::lastModified() const
{
    return d->lastModified;
}

/*!
    Sets the date and time when the meta data was last modified to \a dateTime.
 */
void QNetworkCacheMetaData::setLastModified(const QDateTime &dateTime)
{
    d->lastModified = dateTime;
}

/*!
    Returns the date and time when the meta data expires.
 */
QDateTime QNetworkCacheMetaData::expirationDate() const
{
    return d->expirationDate;
}

/*!
    Sets the date and time when the meta data expires to \a dateTime.
 */
void QNetworkCacheMetaData::setExpirationDate(const QDateTime &dateTime)
{
    d->expirationDate = dateTime;
}

/*!
    \since 4.6

    Returns all the attributes stored with this cache item.

    \sa setAttributes(), QNetworkRequest::Attribute
*/
QNetworkCacheMetaData::AttributesMap QNetworkCacheMetaData::attributes() const
{
    return d->attributes;
}

/*!
    \since 4.6

    Sets all attributes of this cache item to be the map \a attributes.

    \sa attributes(), QNetworkRequest::setAttribute()
*/
void QNetworkCacheMetaData::setAttributes(const AttributesMap &attributes)
{
    d->attributes = attributes;
}

/*!
    \relates QNetworkCacheMetaData
    \since 4.5

    Writes \a metaData to the \a out stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QNetworkCacheMetaData &metaData)
{
    QNetworkCacheMetaDataPrivate::save(out, metaData);
    return out;
}

static inline QDataStream &operator<<(QDataStream &out, const QNetworkCacheMetaData::AttributesMap &hash)
{
    out << quint32(hash.size());
    QNetworkCacheMetaData::AttributesMap::ConstIterator it = hash.begin();
    QNetworkCacheMetaData::AttributesMap::ConstIterator end = hash.end();
    while (it != end) {
        out << int(it.key()) << it.value();
        ++it;
    }
    return out;
}

void QNetworkCacheMetaDataPrivate::save(QDataStream &out, const QNetworkCacheMetaData &metaData)
{
    // note: if you change the contents of the meta data here
    // remember to bump the cache version in qnetworkdiskcache.cpp CurrentCacheVersion
    out << metaData.url();
    out << metaData.expirationDate();
    out << metaData.lastModified();
    out << metaData.saveToDisk();
    out << metaData.attributes();
    out << metaData.rawHeaders();
}

/*!
    \relates QNetworkCacheMetaData
    \since 4.5

    Reads a QNetworkCacheMetaData from the stream \a in into \a metaData.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QNetworkCacheMetaData &metaData)
{
    QNetworkCacheMetaDataPrivate::load(in, metaData);
    return in;
}

static inline QDataStream &operator>>(QDataStream &in, QNetworkCacheMetaData::AttributesMap &hash)
{
    hash.clear();
    QDataStream::Status oldStatus = in.status();
    in.resetStatus();
    hash.clear();

    quint32 n;
    in >> n;

    for (quint32 i = 0; i < n; ++i) {
        if (in.status() != QDataStream::Ok)
            break;

        int k;
        QVariant t;
        in >> k >> t;
        hash.insert(QNetworkRequest::Attribute(k), t);
    }

    if (in.status() != QDataStream::Ok)
        hash.clear();
    if (oldStatus != QDataStream::Ok)
        in.setStatus(oldStatus);
    return in;
}

void QNetworkCacheMetaDataPrivate::load(QDataStream &in, QNetworkCacheMetaData &metaData)
{
    auto *p = metaData.d.data();
    in >> p->url;
    in >> p->expirationDate;
    in >> p->lastModified;
    in >> p->saveToDisk;
    in >> p->attributes;
    QNetworkCacheMetaData::RawHeaderList list; in >> list;
    metaData.setRawHeaders(list);
}

/*!
    \class QAbstractNetworkCache
    \since 4.5
    \inmodule QtNetwork

    \brief The QAbstractNetworkCache class provides the interface for cache implementations.

    QAbstractNetworkCache is the base class for every standard cache that is used by
    QNetworkAccessManager.  QAbstractNetworkCache is an abstract class and cannot be
    instantiated.

    \sa QNetworkDiskCache
*/

/*!
    Constructs an abstract network cache with the given \a parent.
*/
QAbstractNetworkCache::QAbstractNetworkCache(QObject *parent)
    : QObject(*new QAbstractNetworkCachePrivate, parent)
{
}

/*!
    \internal
*/
QAbstractNetworkCache::QAbstractNetworkCache(QAbstractNetworkCachePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Destroys the cache.

    Any operations that have not been inserted are discarded.

    \sa insert()
 */
QAbstractNetworkCache::~QAbstractNetworkCache()
{
}

/*!
    \fn QNetworkCacheMetaData QAbstractNetworkCache::metaData(const QUrl &url) = 0
    Returns the meta data for the url \a url.

    If the url is valid and the cache contains the data for url,
    a valid QNetworkCacheMetaData is returned.

    In the base class this is a pure virtual function.

    \sa updateMetaData(), data()
*/

/*!
    \fn void QAbstractNetworkCache::updateMetaData(const QNetworkCacheMetaData &metaData) = 0
    Updates the cache meta date for the metaData's url to \a metaData

    If the cache does not contains a cache item for the url then no action is taken.

    In the base class this is a pure virtual function.

    \sa metaData(), prepare()
*/

/*!
    \fn QIODevice *QAbstractNetworkCache::data(const QUrl &url) = 0
    Returns the data associated with \a url.

    It is up to the application that requests the data to delete
    the QIODevice when done with it.

    If there is no cache for \a url, the url is invalid, or if there
    is an internal cache error \nullptr is returned.

    In the base class this is a pure virtual function.

    \sa metaData(), prepare()
*/

/*!
    \fn bool QAbstractNetworkCache::remove(const QUrl &url) = 0
    Removes the cache entry for \a url, returning true if success otherwise false.

    In the base class this is a pure virtual function.

    \sa clear(), prepare()
*/

/*!
    \fn QIODevice *QAbstractNetworkCache::prepare(const QNetworkCacheMetaData &metaData) = 0
    Returns the device that should be populated with the data for
    the cache item \a metaData.  When all of the data has been written
    insert() should be called.  If metaData is invalid or the url in
    the metadata is invalid \nullptr is returned.

    The cache owns the device and will take care of deleting it when
    it is inserted or removed.

    To cancel a prepared inserted call remove() on the metadata's url.

    In the base class this is a pure virtual function.

    \sa remove(), updateMetaData(), insert()
*/

/*!
    \fn void QAbstractNetworkCache::insert(QIODevice *device) = 0
    Inserts the data in \a device and the prepared meta data into the cache.
    After this function is called the data and meta data should be retrievable
    using data() and metaData().

    To cancel a prepared inserted call remove() on the metadata's url.

    In the base class this is a pure virtual function.

    \sa prepare(), remove()
*/

/*!
    \fn qint64 QAbstractNetworkCache::cacheSize() const = 0
    Returns the current size taken up by the cache.  Depending upon
    the cache implementation this might be disk or memory size.

    In the base class this is a pure virtual function.

    \sa clear()
*/

/*!
    \fn void QAbstractNetworkCache::clear() = 0
    Removes all items from the cache.  Unless there was failures
    clearing the cache cacheSize() should return 0 after a call to clear.

    In the base class this is a pure virtual function.

    \sa cacheSize(), remove()
*/

QT_END_NAMESPACE

#include "moc_qabstractnetworkcache.cpp"
