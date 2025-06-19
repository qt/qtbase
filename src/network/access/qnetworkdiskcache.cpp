// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

//#define QNETWORKDISKCACHE_DEBUG


#include "qnetworkdiskcache.h"
#include "qnetworkdiskcache_p.h"

#include <qfile.h>
#include <qdir.h>
#include <qdatastream.h>
#include <qdatetime.h>
#include <qdirlisting.h>
#include <qurl.h>
#include <qcryptographichash.h>
#include <qdebug.h>

#include <memory>

#define CACHE_POSTFIX ".d"_L1
#define CACHE_VERSION 8
#define DATA_DIR "data"_L1

#define MAX_COMPRESSION_SIZE (1024 * 1024 * 3)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QNetworkDiskCache
    \since 4.5
    \inmodule QtNetwork

    \brief The QNetworkDiskCache class provides a very basic disk cache.

    QNetworkDiskCache stores each url in its own file inside of the
    cacheDirectory using QDataStream.  Files with a text MimeType
    are compressed using qCompress.  Data is written to disk only in insert()
    and updateMetaData().

    Currently you cannot share the same cache files with more than
    one disk cache.

    QNetworkDiskCache by default limits the amount of space that the cache will
    use on the system to 50MB.

    Note you have to set the cache directory before it will work.

    A network disk cache can be enabled by:

    \snippet code/src_network_access_qnetworkdiskcache.cpp 0

    When sending requests, to control the preference of when to use the cache
    and when to use the network, consider the following:

    \snippet code/src_network_access_qnetworkdiskcache.cpp 1

    To check whether the response came from the cache or from the network, the
    following can be applied:

    \snippet code/src_network_access_qnetworkdiskcache.cpp 2
*/

/*!
    Creates a new disk cache. The \a parent argument is passed to
    QAbstractNetworkCache's constructor.
 */
QNetworkDiskCache::QNetworkDiskCache(QObject *parent)
    : QAbstractNetworkCache(*new QNetworkDiskCachePrivate, parent)
{
}

/*!
    Destroys the cache object.  This does not clear the disk cache.
 */
QNetworkDiskCache::~QNetworkDiskCache()
{
    Q_D(QNetworkDiskCache);
    qDeleteAll(d->inserting);
}

/*!
    Returns the location where cached files will be stored.
*/
QString QNetworkDiskCache::cacheDirectory() const
{
    Q_D(const QNetworkDiskCache);
    return d->cacheDirectory;
}

/*!
    Sets the directory where cached files will be stored to \a cacheDir

    QNetworkDiskCache will create this directory if it does not exists.

    Prepared cache items will be stored in the new cache directory when
    they are inserted.

    \sa QStandardPaths::CacheLocation
*/
void QNetworkDiskCache::setCacheDirectory(const QString &cacheDir)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::setCacheDirectory()" << cacheDir;
#endif
    Q_D(QNetworkDiskCache);
    if (cacheDir.isEmpty())
        return;
    d->cacheDirectory = cacheDir;
    QDir dir(d->cacheDirectory);
    d->cacheDirectory = dir.absolutePath();
    if (!d->cacheDirectory.endsWith(u'/'))
        d->cacheDirectory += u'/';

    d->dataDirectory = d->cacheDirectory + DATA_DIR + QString::number(CACHE_VERSION) + u'/';
    d->prepareLayout();
}

/*!
    \reimp
*/
qint64 QNetworkDiskCache::cacheSize() const
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug("QNetworkDiskCache::cacheSize()");
#endif
    Q_D(const QNetworkDiskCache);
    if (d->cacheDirectory.isEmpty())
        return 0;
    if (d->currentCacheSize < 0) {
        QNetworkDiskCache *that = const_cast<QNetworkDiskCache*>(this);
        that->d_func()->currentCacheSize = that->expire();
    }
    return d->currentCacheSize;
}

/*!
    \reimp
*/
QIODevice *QNetworkDiskCache::prepare(const QNetworkCacheMetaData &metaData)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::prepare()" << metaData.url();
#endif
    Q_D(QNetworkDiskCache);
    if (!metaData.isValid() || !metaData.url().isValid() || !metaData.saveToDisk())
        return nullptr;

    if (d->cacheDirectory.isEmpty()) {
        qWarning("QNetworkDiskCache::prepare() The cache directory is not set");
        return nullptr;
    }

    const auto sizeValue = metaData.headers().value(QHttpHeaders::WellKnownHeader::ContentLength);
    const qint64 size = sizeValue.toLongLong();
    if (size > (maximumCacheSize() * 3)/4)
        return nullptr;

    std::unique_ptr<QCacheItem> cacheItem = std::make_unique<QCacheItem>();
    cacheItem->metaData = metaData;

    QIODevice *device = nullptr;
    if (cacheItem->canCompress()) {
        cacheItem->data.open(QBuffer::ReadWrite);
        device = &(cacheItem->data);
    } else {
        QString fileName = d->cacheFileName(cacheItem->metaData.url());
        cacheItem->file = new(std::nothrow) QSaveFile(fileName, &cacheItem->data);
        if (!cacheItem->file || !cacheItem->file->open(QFileDevice::WriteOnly)) {
            qWarning("QNetworkDiskCache::prepare() unable to open temporary file");
            cacheItem.reset();
            return nullptr;
        }
        cacheItem->writeHeader(cacheItem->file);
        device = cacheItem->file;
    }
    d->inserting[device] = cacheItem.release();
    return device;
}

/*!
    \reimp
*/
void QNetworkDiskCache::insert(QIODevice *device)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::insert()" << device;
#endif
    Q_D(QNetworkDiskCache);
    const auto it = d->inserting.constFind(device);
    if (Q_UNLIKELY(it == d->inserting.cend())) {
        qWarning() << "QNetworkDiskCache::insert() called on a device we don't know about" << device;
        return;
    }

    d->storeItem(it.value());
    delete it.value();
    d->inserting.erase(it);
}


/*!
    Create subdirectories and other housekeeping on the filesystem.
    Prevents too many files from being present in any single directory.
*/
void QNetworkDiskCachePrivate::prepareLayout()
{
    QDir helper;

    //Create directory and subdirectories 0-F
    helper.mkpath(dataDirectory);
    for (uint i = 0; i < 16 ; i++) {
        QString str = QString::number(i, 16);
        QString subdir = dataDirectory + str;
        helper.mkdir(subdir);
    }
}


void QNetworkDiskCachePrivate::storeItem(QCacheItem *cacheItem)
{
    Q_Q(QNetworkDiskCache);
    Q_ASSERT(cacheItem->metaData.saveToDisk());

    QString fileName = cacheFileName(cacheItem->metaData.url());
    Q_ASSERT(!fileName.isEmpty());

    if (QFile::exists(fileName)) {
        if (!removeFile(fileName)) {
            qWarning() << "QNetworkDiskCache: couldn't remove the cache file " << fileName;
            return;
        }
    }

    currentCacheSize = q->expire();
    if (!cacheItem->file) {
        cacheItem->file = new QSaveFile(fileName, &cacheItem->data);
        if (cacheItem->file->open(QFileDevice::WriteOnly)) {
            cacheItem->writeHeader(cacheItem->file);
            cacheItem->writeCompressedData(cacheItem->file);
        }
    }

    if (cacheItem->file
        && cacheItem->file->isOpen()
        && cacheItem->file->error() == QFileDevice::NoError) {
        // We have to call size() here instead of inside the if-body because
        // commit() invalidates the file-engine, and size() will create a new
        // one, pointing at an empty filename.
        qint64 size = cacheItem->file->size();
        if (cacheItem->file->commit())
            currentCacheSize += size;
        // Delete and unset the QSaveFile, it's invalid now.
        delete std::exchange(cacheItem->file, nullptr);
    }
    if (cacheItem->metaData.url() == lastItem.metaData.url())
        lastItem.reset();
}

/*!
    \reimp
*/
bool QNetworkDiskCache::remove(const QUrl &url)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::remove()" << url;
#endif
    Q_D(QNetworkDiskCache);

    // remove is also used to cancel insertions, not a common operation
    for (auto it = d->inserting.cbegin(), end = d->inserting.cend(); it != end; ++it) {
        QCacheItem *item = it.value();
        if (item && item->metaData.url() == url) {
            delete item;
            d->inserting.erase(it);
            return true;
        }
    }

    if (d->lastItem.metaData.url() == url)
        d->lastItem.reset();
    return d->removeFile(d->cacheFileName(url));
}

/*!
    Put all of the misc file removing into one function to be extra safe
 */
bool QNetworkDiskCachePrivate::removeFile(const QString &file)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::removFile()" << file;
#endif
    if (file.isEmpty())
        return false;
    QFileInfo info(file);
    QString fileName = info.fileName();
    if (!fileName.endsWith(CACHE_POSTFIX))
        return false;
    qint64 size = info.size();
    if (QFile::remove(file)) {
        currentCacheSize -= size;
        return true;
    }
    return false;
}

/*!
    \reimp
*/
QNetworkCacheMetaData QNetworkDiskCache::metaData(const QUrl &url)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::metaData()" << url;
#endif
    Q_D(QNetworkDiskCache);
    if (d->lastItem.metaData.url() == url)
        return d->lastItem.metaData;
    return fileMetaData(d->cacheFileName(url));
}

/*!
    Returns the QNetworkCacheMetaData for the cache file \a fileName.

    If \a fileName is not a cache file QNetworkCacheMetaData will be invalid.
 */
QNetworkCacheMetaData QNetworkDiskCache::fileMetaData(const QString &fileName) const
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::fileMetaData()" << fileName;
#endif
    Q_D(const QNetworkDiskCache);
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return QNetworkCacheMetaData();
    if (!d->lastItem.read(&file, false)) {
        file.close();
        QNetworkDiskCachePrivate *that = const_cast<QNetworkDiskCachePrivate*>(d);
        that->removeFile(fileName);
    }
    return d->lastItem.metaData;
}

/*!
    \reimp
*/
QIODevice *QNetworkDiskCache::data(const QUrl &url)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::data()" << url;
#endif
    Q_D(QNetworkDiskCache);
    std::unique_ptr<QBuffer> buffer;
    if (!url.isValid())
        return nullptr;
    if (d->lastItem.metaData.url() == url && d->lastItem.data.isOpen()) {
        buffer.reset(new QBuffer);
        buffer->setData(d->lastItem.data.data());
    } else {
        QFile file(d->cacheFileName(url));
        if (!file.open(QFile::ReadOnly | QIODevice::Unbuffered))
            return nullptr;

        if (!d->lastItem.read(&file, true)) {
            file.close(); // On Windows the file can't be removed if it's open
            remove(url);
            return nullptr;
        }
        if (d->lastItem.data.isOpen()) {
            // compressed
            buffer.reset(new QBuffer);
            buffer->setData(d->lastItem.data.data());
        } else {
            buffer.reset(new QBuffer);
            buffer->setData(file.readAll());
        }
    }
    buffer->open(QBuffer::ReadOnly);
    return buffer.release();
}

/*!
    \reimp
*/
void QNetworkDiskCache::updateMetaData(const QNetworkCacheMetaData &metaData)
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug() << "QNetworkDiskCache::updateMetaData()" << metaData.url();
#endif
    QUrl url = metaData.url();
    QIODevice *oldDevice = data(url);
    if (!oldDevice) {
#if defined(QNETWORKDISKCACHE_DEBUG)
        qDebug("QNetworkDiskCache::updateMetaData(), no device!");
#endif
        return;
    }

    QIODevice *newDevice = prepare(metaData);
    if (!newDevice) {
#if defined(QNETWORKDISKCACHE_DEBUG)
        qDebug() << "QNetworkDiskCache::updateMetaData(), no new device!" << url;
#endif
        return;
    }
    char data[1024];
    while (!oldDevice->atEnd()) {
        qint64 s = oldDevice->read(data, 1024);
        newDevice->write(data, s);
    }
    delete oldDevice;
    insert(newDevice);
}

/*!
    Returns the current maximum size for the disk cache.

    \sa setMaximumCacheSize()
 */
qint64 QNetworkDiskCache::maximumCacheSize() const
{
    Q_D(const QNetworkDiskCache);
    return d->maximumCacheSize;
}

/*!
    Sets the maximum size of the disk cache to be \a size.

    If the new size is smaller then the current cache size then the cache will call expire().

    \sa maximumCacheSize()
 */
void QNetworkDiskCache::setMaximumCacheSize(qint64 size)
{
    Q_D(QNetworkDiskCache);
    bool expireCache = (size < d->maximumCacheSize);
    d->maximumCacheSize = size;
    if (expireCache)
        d->currentCacheSize = expire();
}

/*!
    Cleans the cache so that its size is under the maximum cache size.
    Returns the current size of the cache.

    When the current size of the cache is greater than the maximumCacheSize()
    older cache files are removed until the total size is less then 90% of
    maximumCacheSize() starting with the oldest ones first using the file
    creation date to determine how old a cache file is.

    Subclasses can reimplement this function to change the order that cache
    files are removed taking into account information in the application
    knows about that QNetworkDiskCache does not, for example the number of times
    a cache is accessed.

    \note cacheSize() calls expire if the current cache size is unknown.

    \sa maximumCacheSize(), fileMetaData()
 */
qint64 QNetworkDiskCache::expire()
{
    Q_D(QNetworkDiskCache);
    if (d->currentCacheSize >= 0 && d->currentCacheSize < maximumCacheSize())
        return d->currentCacheSize;

    if (cacheDirectory().isEmpty()) {
        qWarning("QNetworkDiskCache::expire() The cache directory is not set");
        return 0;
    }

    // close file handle to prevent "in use" error when QFile::remove() is called
    d->lastItem.reset();

    struct CacheItem
    {
        std::chrono::milliseconds msecs;
        QString path;
        qint64 size = 0;
    };
    std::vector<CacheItem> cacheItems;
    qint64 totalSize = 0;
    using F = QDirListing::IteratorFlag;
    for (const auto &dirEntry : QDirListing(cacheDirectory(), F::FilesOnly | F::Recursive)) {
        if (!dirEntry.fileName().endsWith(CACHE_POSTFIX))
            continue;

        const QFileInfo &info = dirEntry.fileInfo();
        QDateTime fileTime = info.birthTime(QTimeZone::UTC);
        if (!fileTime.isValid())
            fileTime = info.metadataChangeTime(QTimeZone::UTC);
        const std::chrono::milliseconds msecs{fileTime.toMSecsSinceEpoch()};
        const qint64 size = info.size();
        cacheItems.push_back(CacheItem{msecs, info.filePath(), size});
        totalSize += size;
    }

    const qint64 goal = (maximumCacheSize() * 9) / 10;
    if (totalSize < goal)
        return totalSize; // Nothing to do

    auto byFileTime = [&](const auto &a, const auto &b) { return a.msecs < b.msecs; };
    std::sort(cacheItems.begin(), cacheItems.end(), byFileTime);

    [[maybe_unused]] int removedFiles = 0; // used under QNETWORKDISKCACHE_DEBUG
    for (const CacheItem &cached : cacheItems) {
        QFile::remove(cached.path);
        ++removedFiles;
        totalSize -= cached.size;
        if (totalSize < goal)
            break;
    }
#if defined(QNETWORKDISKCACHE_DEBUG)
    if (removedFiles > 0) {
        qDebug() << "QNetworkDiskCache::expire()"
                << "Removed:" << removedFiles
                << "Kept:" << cacheItems.count() - removedFiles;
    }
#endif
    return totalSize;
}

/*!
    \reimp
*/
void QNetworkDiskCache::clear()
{
#if defined(QNETWORKDISKCACHE_DEBUG)
    qDebug("QNetworkDiskCache::clear()");
#endif
    Q_D(QNetworkDiskCache);
    qint64 size = d->maximumCacheSize;
    d->maximumCacheSize = 0;
    d->currentCacheSize = expire();
    d->maximumCacheSize = size;
}

/*!
    Given a URL, generates a unique enough filename (and subdirectory)
 */
QString QNetworkDiskCachePrivate::uniqueFileName(const QUrl &url)
{
    QUrl cleanUrl = url;
    cleanUrl.setPassword(QString());
    cleanUrl.setFragment(QString());

    const QByteArray hash = QCryptographicHash::hash(cleanUrl.toEncoded(), QCryptographicHash::Sha1);
    // convert sha1 to base36 form and return first 8 bytes for use as string
    const QByteArray id = QByteArray::number(*(qlonglong*)hash.data(), 36).left(8);
    // generates <one-char subdir>/<8-char filename.d>
    uint code = (uint)id.at(id.size()-1) % 16;
    QString pathFragment = QString::number(code, 16) + u'/' + QLatin1StringView(id) + CACHE_POSTFIX;

    return pathFragment;
}

/*!
    Generates fully qualified path of cached resource from a URL.
 */
QString QNetworkDiskCachePrivate::cacheFileName(const QUrl &url) const
{
    if (!url.isValid())
        return QString();

    QString fullpath = dataDirectory + uniqueFileName(url);
    return  fullpath;
}

/*!
    We compress small text and JavaScript files.
 */
bool QCacheItem::canCompress() const
{
    const auto h = metaData.headers();

    const auto sizeValue = h.value(QHttpHeaders::WellKnownHeader::ContentLength);
    if (sizeValue.empty())
        return false;

    qint64 size = sizeValue.toLongLong();
    if (size > MAX_COMPRESSION_SIZE)
        return false;

    const auto type = h.value(QHttpHeaders::WellKnownHeader::ContentType);
    if (!type.empty())
        return false;

    if (!type.startsWith("text/")
        && !(type.startsWith("application/")
            && (type.endsWith("javascript") || type.endsWith("ecmascript")))) {
        return false;
    }

    return true;
}

enum
{
    CacheMagic = 0xe8,
    CurrentCacheVersion = CACHE_VERSION
};

void QCacheItem::writeHeader(QFileDevice *device) const
{
    QDataStream out(device);

    out << qint32(CacheMagic);
    out << qint32(CurrentCacheVersion);
    out << static_cast<qint32>(out.version());
    out << metaData;
    bool compressed = canCompress();
    out << compressed;
}

void QCacheItem::writeCompressedData(QFileDevice *device) const
{
    QDataStream out(device);

    out << qCompress(data.data());
}

/*!
    Returns \c false if the file is a cache file,
    but is an older version and should be removed otherwise true.
 */
bool QCacheItem::read(QFileDevice *device, bool readData)
{
    reset();

    QDataStream in(device);

    qint32 marker;
    qint32 v;
    in >> marker;
    in >> v;
    if (marker != CacheMagic)
        return true;

    // If the cache magic is correct, but the version is not we should remove it
    if (v != CurrentCacheVersion)
        return false;

    qint32 streamVersion;
    in >> streamVersion;
    // Default stream version is also the highest we can handle
    if (streamVersion > in.version())
        return false;
    in.setVersion(streamVersion);

    bool compressed;
    QByteArray dataBA;
    in >> metaData;
    in >> compressed;
    if (readData && compressed) {
        in >> dataBA;
        data.setData(qUncompress(dataBA));
        data.open(QBuffer::ReadOnly);
    }

    // quick and dirty check if metadata's URL field and the file's name are in synch
    QString expectedFilename = QNetworkDiskCachePrivate::uniqueFileName(metaData.url());
    if (!device->fileName().endsWith(expectedFilename))
        return false;

    return metaData.isValid() && !metaData.headers().isEmpty();
}

QT_END_NAMESPACE

#include "moc_qnetworkdiskcache.cpp"
