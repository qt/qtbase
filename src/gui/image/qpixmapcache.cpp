// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpixmapcache.h"
#include "qobject.h"
#include "qdebug.h"
#include "qpixmapcache_p.h"
#include "qthread.h"
#include "qcoreapplication.h"

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

/*!
    \class QPixmapCache
    \inmodule QtGui

    \brief The QPixmapCache class provides an application-wide cache for pixmaps.

    This class is a tool for optimized drawing with QPixmap. You can
    use it to store temporary pixmaps that are expensive to generate
    without using more storage space than cacheLimit(). Use insert()
    to insert pixmaps, find() to find them, and clear() to empty the
    cache.

    QPixmapCache contains no member data, only static functions to
    access the global pixmap cache. It creates an internal QCache
    object for caching the pixmaps.

    The cache associates a pixmap with a user-provided string as a key,
    or with a QPixmapCache::Key that the cache generates.
    Using QPixmapCache::Key for keys is faster than using strings. The string API is
    very convenient for complex keys but the QPixmapCache::Key API will be very
    efficient and convenient for a one-to-one object-to-pixmap mapping - in
    this case, you can store the keys as members of an object.

    If two pixmaps are inserted into the cache using equal keys then the
    last pixmap will replace the first pixmap in the cache. This follows the
    behavior of the QHash and QCache classes.

    The cache becomes full when the total size of all pixmaps in the
    cache exceeds cacheLimit(). The initial cache limit is 10240 KB (10 MB);
    you can change this by calling setCacheLimit() with the required value.
    A pixmap takes roughly (\e{width} * \e{height} * \e{depth})/8 bytes of
    memory.

    The \e{Qt Quarterly} article
    \l{http://doc.qt.io/archives/qq/qq12-qpixmapcache.html}{Optimizing
    with QPixmapCache} explains how to use QPixmapCache to speed up
    applications by caching the results of painting.

    \note QPixmapCache is only usable from the application's main thread.
    Access from other threads will be ignored and return failure.

    \sa QCache, QPixmap
*/

static const int cache_limit_default = 10240; // 10 MB cache limit

static inline qsizetype cost(const QPixmap &pixmap)
{
    // make sure to do a 64bit calculation; qsizetype might be smaller
    const qint64 costKb = static_cast<qint64>(pixmap.width())
                        * pixmap.height() * pixmap.depth() / (8 * 1024);
    const qint64 costMax = std::numeric_limits<qsizetype>::max();
    // a small pixmap should have at least a cost of 1(kb)
    return static_cast<qsizetype>(qBound(1LL, costKb, costMax));
}

static inline bool qt_pixmapcache_thread_test()
{
    if (Q_LIKELY(QCoreApplication::instance() && QThread::currentThread() == QCoreApplication::instance()->thread()))
        return true;

    return false;
}

/*!
    \class QPixmapCache::Key
    \brief The QPixmapCache::Key class can be used for efficient access
    to the QPixmapCache.
    \inmodule QtGui
    \since 4.6

    Use QPixmapCache::insert() to receive an instance of Key generated
    by the pixmap cache. You can store the key in your own objects for
    a very efficient one-to-one object-to-pixmap mapping.
*/

/*!
    Constructs an empty Key object.
*/
QPixmapCache::Key::Key() : d(nullptr)
{
}

/*!
   \internal
    Constructs a copy of \a other.
*/
QPixmapCache::Key::Key(const Key &other)
{
    if (other.d)
        ++(other.d->ref);
    d = other.d;
}

/*!
    Destroys the key.
*/
QPixmapCache::Key::~Key()
{
    if (d && --(d->ref) == 0)
        delete d;
}

/*!
    \internal

    Returns \c true if this key is the same as the given \a key; otherwise returns
    false.
*/
bool QPixmapCache::Key::operator ==(const Key &key) const
{
    return (d == key.d);
}

/*!
    \fn bool QPixmapCache::Key::operator !=(const Key &key) const
    \internal
*/

/*!
    \fn QPixmapCache::Key::Key(Key &&)
    \internal
    \since 5.6
*/

/*!
    \fn QPixmapCache::Key &QPixmapCache::Key::operator=(Key &&)
    \internal
    \since 5.6
*/

/*!
    \fn void QPixmapCache::Key::swap(Key &)
    \internal
    \since 5.6
*/

/*!
    Returns \c true if there is a cached pixmap associated with this key.
    Otherwise, if pixmap was flushed, the key is no longer valid.
    \since 5.7
*/
bool QPixmapCache::Key::isValid() const noexcept
{
    return d && d->isValid;
}

/*!
    \internal
*/
QPixmapCache::Key &QPixmapCache::Key::operator =(const Key &other)
{
    if (d != other.d) {
        if (other.d)
            ++(other.d->ref);
        if (d && --(d->ref) == 0)
            delete d;
        d = other.d;
    }
    return *this;
}

class QPMCache : public QObject, public QCache<QPixmapCache::Key, QPixmapCacheEntry>
{
    Q_OBJECT
public:
    QPMCache();
    ~QPMCache();

    void timerEvent(QTimerEvent *) override;
    bool insert(const QString& key, const QPixmap &pixmap, int cost);
    QPixmapCache::Key insert(const QPixmap &pixmap, int cost);
    bool remove(const QString &key);
    bool remove(const QPixmapCache::Key &key);

    void resizeKeyArray(int size);
    QPixmapCache::Key createKey();
    void releaseKey(const QPixmapCache::Key &key);
    void clear();

    QPixmap *object(const QString &key) const;
    QPixmap *object(const QPixmapCache::Key &key) const;

    static inline QPixmapCache::KeyData *get(const QPixmapCache::Key &key)
    {return key.d;}

    static QPixmapCache::KeyData* getKeyData(QPixmapCache::Key *key);

    bool flushDetachedPixmaps(bool nt);

private:
    static constexpr auto soon_time = 10s;
    static constexpr auto flush_time = 30s;
    int *keyArray;
    int theid;
    int ps;
    int keyArraySize;
    int freeKey;
    QHash<QString, QPixmapCache::Key> cacheKeys;
    bool t;
};

QT_BEGIN_INCLUDE_NAMESPACE
#include "qpixmapcache.moc"
QT_END_INCLUDE_NAMESPACE

/*!
    size_t QPixmapCache::qHash(const Key &key, size_t seed = 0);
    \since 6.6

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
size_t QPixmapCache::Key::hash(size_t seed) const noexcept
{
    return qHash(this->d ? this->d->key : 0, seed);
}

QPMCache::QPMCache()
    : QObject(nullptr),
      QCache<QPixmapCache::Key, QPixmapCacheEntry>(cache_limit_default),
      keyArray(nullptr), theid(0), ps(0), keyArraySize(0), freeKey(0), t(false)
{
}
QPMCache::~QPMCache()
{
    clear();
    free(keyArray);
}

/*
  This is supposed to cut the cache size down by about 25% in a
  minute once the application becomes idle, to let any inserted pixmap
  remain in the cache for some time before it becomes a candidate for
  cleaning-up, and to not cut down the size of the cache while the
  cache is in active use.

  When the last detached pixmap has been deleted from the cache, kill the
  timer so Qt won't keep the CPU from going into sleep mode. Currently
  the timer is not restarted when the pixmap becomes unused, but it does
  restart once something else is added (i.e. the cache space is actually needed).

  Returns \c true if any were removed.
*/
bool QPMCache::flushDetachedPixmaps(bool nt)
{
    auto mc = maxCost();
    const qsizetype currentTotal = totalCost();
    const qsizetype oldSize = size();
    if (currentTotal)
        setMaxCost(nt ? currentTotal * 3 / 4 : currentTotal - 1);
    setMaxCost(mc);
    ps = totalCost();
    return size() != oldSize;
}

void QPMCache::timerEvent(QTimerEvent *)
{
    bool nt = totalCost() == ps;
    if (!flushDetachedPixmaps(nt)) {
        killTimer(theid);
        theid = 0;
    } else if (nt != t) {
        killTimer(theid);
        theid = startTimer(nt ? soon_time : flush_time);
        t = nt;
    }
}


QPixmap *QPMCache::object(const QString &key) const
{
    if (const auto it = cacheKeys.find(key); it != cacheKeys.cend())
        return object(it.value());
    return nullptr;
}

QPixmap *QPMCache::object(const QPixmapCache::Key &key) const
{
    Q_ASSERT(key.isValid());
    QPixmap *ptr = QCache<QPixmapCache::Key, QPixmapCacheEntry>::object(key);
    //We didn't find the pixmap in the cache, the key is not valid anymore
    if (!ptr)
        const_cast<QPMCache *>(this)->releaseKey(key);
    return ptr;
}

bool QPMCache::insert(const QString& key, const QPixmap &pixmap, int cost)
{
    //If for the same key we add already a pixmap we should delete it
    remove(key);

    // this will create a new key; the old one has been removed
    auto k = insert(pixmap, cost);
    if (k.isValid()) {
        k.d->stringKey = key;
        cacheKeys[key] = std::move(k);
        return true;
    }
    return false;
}

QPixmapCache::Key QPMCache::insert(const QPixmap &pixmap, int cost)
{
    QPixmapCache::Key cacheKey = createKey(); // invalidated by ~QPixmapCacheEntry on failed insert
    bool success = QCache<QPixmapCache::Key, QPixmapCacheEntry>::insert(cacheKey, new QPixmapCacheEntry(cacheKey, pixmap), cost);
    Q_ASSERT(success || !cacheKey.isValid());
    if (success) {
        if (!theid) {
            theid = startTimer(flush_time);
            t = false;
        }
    }
    return cacheKey;
}

bool QPMCache::remove(const QString &key)
{
    const auto cacheKey = cacheKeys.take(key);
    return cacheKey.isValid() && remove(cacheKey);
}

bool QPMCache::remove(const QPixmapCache::Key &key)
{
    return QCache<QPixmapCache::Key, QPixmapCacheEntry>::remove(key);
}

void QPMCache::resizeKeyArray(int size)
{
    if (size <= keyArraySize || size == 0)
        return;
    keyArray = q_check_ptr(static_cast<int *>(realloc(keyArray,
                    size * sizeof(int))));
    for (int i = keyArraySize; i != size; ++i)
        keyArray[i] = i + 1;
    keyArraySize = size;
}

QPixmapCache::Key QPMCache::createKey()
{
    if (freeKey == keyArraySize)
        resizeKeyArray(keyArraySize ? keyArraySize << 1 : 2);
    int id = freeKey;
    freeKey = keyArray[id];
    QPixmapCache::Key key;
    QPixmapCache::KeyData *d = QPMCache::getKeyData(&key);
    d->key = ++id;
    return key;
}

void QPMCache::releaseKey(const QPixmapCache::Key &key)
{
    QPixmapCache::KeyData *keyData = key.d;
    if (!keyData)
        return;
    if (!keyData->stringKey.isNull())
        cacheKeys.remove(keyData->stringKey);
    if (keyData->key > keyArraySize || keyData->key <= 0)
        return;
    keyData->key--;
    keyArray[keyData->key] = freeKey;
    freeKey = keyData->key;
    keyData->isValid = false;
    keyData->key = 0;
}

void QPMCache::clear()
{
    free(keyArray);
    keyArray = nullptr;
    freeKey = 0;
    keyArraySize = 0;
    //Mark all keys as invalid
    const QList<QPixmapCache::Key> keys = QCache<QPixmapCache::Key, QPixmapCacheEntry>::keys();
    for (const auto &key : keys) {
        if (key.d)
            key.d->isValid = false;
    }
    QCache<QPixmapCache::Key, QPixmapCacheEntry>::clear();
    // Nothing left to flush; stop the timer
    if (theid) {
        killTimer(theid);
        theid = 0;
    }
}

QPixmapCache::KeyData* QPMCache::getKeyData(QPixmapCache::Key *key)
{
    if (!key->d)
        key->d = new QPixmapCache::KeyData;
    return key->d;
}

Q_GLOBAL_STATIC(QPMCache, pm_cache)

int Q_AUTOTEST_EXPORT q_QPixmapCache_keyHashSize()
{
    return pm_cache()->size();
}

QPixmapCacheEntry::~QPixmapCacheEntry()
{
    pm_cache()->releaseKey(key);
}

/*!
    Looks for a cached pixmap associated with the given \a key in the cache.
    If the pixmap is found, the function sets \a pixmap to that pixmap and
    returns \c true; otherwise it leaves \a pixmap alone and returns \c false.

    \since 4.6

    Example:
    \snippet code/src_gui_image_qpixmapcache.cpp 1
*/

bool QPixmapCache::find(const QString &key, QPixmap *pixmap)
{
    if (key.isEmpty() || !qt_pixmapcache_thread_test())
        return false;
    QPixmap *ptr = pm_cache()->object(key);
    if (ptr && pixmap)
        *pixmap = *ptr;
    return ptr != nullptr;
}

/*!
    Looks for a cached pixmap associated with the given \a key in the cache.
    If the pixmap is found, the function sets \a pixmap to that pixmap and
    returns \c true; otherwise it leaves \a pixmap alone and returns \c false. If
    the pixmap is not found, it means that the \a key is no longer valid,
    so it will be released for the next insertion.

    \since 4.6
*/
bool QPixmapCache::find(const Key &key, QPixmap *pixmap)
{
    if (!qt_pixmapcache_thread_test())
        return false;
    //The key is not valid anymore, a flush happened before probably
    if (!key.d || !key.d->isValid)
        return false;
    QPixmap *ptr = pm_cache()->object(key);
    if (ptr && pixmap)
        *pixmap = *ptr;
    return ptr != nullptr;
}

/*!
    Inserts a copy of the pixmap \a pixmap associated with the \a key into
    the cache.

    All pixmaps inserted by the Qt library have a key starting with
    "$qt", so your own pixmap keys should never begin "$qt".

    When a pixmap is inserted and the cache is about to exceed its
    limit, it removes pixmaps until there is enough room for the
    pixmap to be inserted.

    The oldest pixmaps (least recently accessed in the cache) are
    deleted when more space is needed.

    The function returns \c true if the object was inserted into the
    cache; otherwise it returns \c false.

    \sa setCacheLimit()
*/

bool QPixmapCache::insert(const QString &key, const QPixmap &pixmap)
{
    if (key.isEmpty() || !qt_pixmapcache_thread_test())
        return false;
    return pm_cache()->insert(key, pixmap, cost(pixmap));
}

/*!
    Inserts a copy of the given \a pixmap into the cache and returns a key
    that can be used to retrieve it.

    When a pixmap is inserted and the cache is about to exceed its
    limit, it removes pixmaps until there is enough room for the
    pixmap to be inserted.

    The oldest pixmaps (least recently accessed in the cache) are
    deleted when more space is needed.

    \sa setCacheLimit(), replace()

    \since 4.6
*/
QPixmapCache::Key QPixmapCache::insert(const QPixmap &pixmap)
{
    if (!qt_pixmapcache_thread_test())
        return QPixmapCache::Key();
    return pm_cache()->insert(pixmap, cost(pixmap));
}

#if QT_DEPRECATED_SINCE(6, 6)
/*!
    \fn bool QPixmapCache::replace(const Key &key, const QPixmap &pixmap)

    \deprecated [6.6] Use \c{remove(key); key = insert(pixmap);} instead.

    Replaces the pixmap associated with the given \a key with the \a pixmap
    specified. Returns \c true if the \a pixmap has been correctly inserted into
    the cache; otherwise returns \c false.

    The passed \a key is updated to reference \a pixmap now. Other copies of \a
    key, if any, still refer to the old pixmap, which is, however, removed from
    the cache by this function.

    \sa setCacheLimit(), insert()

    \since 4.6
*/
#endif // QT_DEPRECATED_SINCE(6, 6)

/*!
    Returns the cache limit (in kilobytes).

    The default cache limit is 10240 KB.

    \sa setCacheLimit()
*/

int QPixmapCache::cacheLimit()
{
    if (!qt_pixmapcache_thread_test())
        return 0;
    return pm_cache()->maxCost();
}

/*!
    Sets the cache limit to \a n kilobytes.

    The default setting is 10240 KB.

    \sa cacheLimit()
*/

void QPixmapCache::setCacheLimit(int n)
{
    if (!qt_pixmapcache_thread_test())
        return;
    pm_cache()->setMaxCost(n);
}

/*!
  Removes the pixmap associated with \a key from the cache.
*/
void QPixmapCache::remove(const QString &key)
{
    if (key.isEmpty() || !qt_pixmapcache_thread_test())
        return;
    pm_cache()->remove(key);
}

/*!
  Removes the pixmap associated with \a key from the cache and releases
  the key for a future insertion.

  \since 4.6
*/
void QPixmapCache::remove(const Key &key)
{
    if (!qt_pixmapcache_thread_test())
        return;
    //The key is not valid anymore, a flush happened before probably
    if (!key.d || !key.d->isValid)
        return;
    pm_cache()->remove(key);
}

/*!
    Removes all pixmaps from the cache.
*/

void QPixmapCache::clear()
{
    if (!QCoreApplication::closingDown() && !qt_pixmapcache_thread_test())
        return;
    QT_TRY {
        if (pm_cache.exists())
            pm_cache->clear();
    } QT_CATCH(const std::bad_alloc &) {
        // if we ran out of memory during pm_cache(), it's no leak,
        // so just ignore it.
    }
}

Q_AUTOTEST_EXPORT void qt_qpixmapcache_flush_detached_pixmaps() // for tst_qpixmapcache
{
    if (!qt_pixmapcache_thread_test())
        return;
    pm_cache()->flushDetachedPixmaps(true);
}

Q_AUTOTEST_EXPORT int qt_qpixmapcache_qpixmapcache_total_used() // for tst_qpixmapcache
{
    if (!qt_pixmapcache_thread_test())
        return 0;
    return (pm_cache()->totalCost()+1023) / 1024;
}

/*!
   \fn QPixmapCache::KeyData::KeyData()

   \internal
*/
/*!
   \fn QPixmapCache::KeyData::KeyData(const KeyData &other)
   \internal
*/
/*!
   \fn QPixmapCache::KeyData::~KeyData()

   \internal
*/
QT_END_NAMESPACE
