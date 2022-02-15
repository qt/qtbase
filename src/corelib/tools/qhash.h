/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#ifndef QHASH_H
#define QHASH_H

#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qhashfunctions.h>
#include <QtCore/qiterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qmath.h>
#include <QtCore/qrefcount.h>

#include <initializer_list>
#include <functional> // for std::hash

class tst_QHash; // for befriending

QT_BEGIN_NAMESPACE

struct QHashDummyValue
{
    bool operator==(const QHashDummyValue &) const noexcept { return true; }
};

namespace QHashPrivate {

template <typename T, typename = void>
constexpr inline bool HasQHashOverload = false;

template <typename T>
constexpr inline bool HasQHashOverload<T, std::enable_if_t<
    std::is_convertible_v<decltype(qHash(std::declval<const T &>(), std::declval<size_t>())), size_t>
>> = true;

template <typename T, typename = void>
constexpr inline bool HasStdHashSpecializationWithSeed = false;

template <typename T>
constexpr inline bool HasStdHashSpecializationWithSeed<T, std::enable_if_t<
    std::is_convertible_v<decltype(std::hash<T>()(std::declval<const T &>(), std::declval<size_t>())), size_t>
>> = true;

template <typename T, typename = void>
constexpr inline bool HasStdHashSpecializationWithoutSeed = false;

template <typename T>
constexpr inline bool HasStdHashSpecializationWithoutSeed<T, std::enable_if_t<
    std::is_convertible_v<decltype(std::hash<T>()(std::declval<const T &>())), size_t>
>> = true;

template <typename T>
size_t calculateHash(const T &t, size_t seed = 0)
{
    if constexpr (HasQHashOverload<T>) {
        return qHash(t, seed);
    } else if constexpr (HasStdHashSpecializationWithSeed<T>) {
        return std::hash<T>()(t, seed);
    } else if constexpr (HasStdHashSpecializationWithoutSeed<T>) {
        Q_UNUSED(seed);
        return std::hash<T>()(t);
    } else {
        static_assert(sizeof(T) == 0, "The key type must have a qHash overload or a std::hash specialization");
        return 0;
    }
}

template <typename Key, typename T>
struct Node
{
    using KeyType = Key;
    using ValueType = T;

    Key key;
    T value;
    template<typename ...Args>
    static void createInPlace(Node *n, Key &&k, Args &&... args)
    { new (n) Node{ std::move(k), T(std::forward<Args>(args)...) }; }
    template<typename ...Args>
    static void createInPlace(Node *n, const Key &k, Args &&... args)
    { new (n) Node{ Key(k), T(std::forward<Args>(args)...) }; }
    template<typename ...Args>
    void emplaceValue(Args &&... args)
    {
        value = T(std::forward<Args>(args)...);
    }
    T &&takeValue() noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        return std::move(value);
    }
    bool valuesEqual(const Node *other) const { return value == other->value; }
};

template <typename Key>
struct Node<Key, QHashDummyValue> {
    using KeyType = Key;
    using ValueType = QHashDummyValue;

    Key key;
    template<typename ...Args>
    static void createInPlace(Node *n, Key &&k, Args &&...)
    { new (n) Node{ std::move(k) }; }
    template<typename ...Args>
    static void createInPlace(Node *n, const Key &k, Args &&...)
    { new (n) Node{ k }; }
    template<typename ...Args>
    void emplaceValue(Args &&...)
    {
    }
    ValueType takeValue() { return QHashDummyValue(); }
    bool valuesEqual(const Node *) const { return true; }
};

template <typename T>
struct MultiNodeChain
{
    T value;
    MultiNodeChain *next = nullptr;
    ~MultiNodeChain()
    {
    }
    qsizetype free() noexcept(std::is_nothrow_destructible_v<T>)
    {
        qsizetype nEntries = 0;
        MultiNodeChain *e = this;
        while (e) {
            MultiNodeChain *n = e->next;
            ++nEntries;
            delete e;
            e = n;
        }
        return  nEntries;
    }
    bool contains(const T &val) const noexcept
    {
        const MultiNodeChain *e = this;
        while (e) {
            if (e->value == val)
                return true;
            e = e->next;
        }
        return false;
    }
};

template <typename Key, typename T>
struct MultiNode
{
    using KeyType = Key;
    using ValueType = T;
    using Chain = MultiNodeChain<T>;

    Key key;
    Chain *value;

    template<typename ...Args>
    static void createInPlace(MultiNode *n, Key &&k, Args &&... args)
    { new (n) MultiNode(std::move(k), new Chain{ T(std::forward<Args>(args)...), nullptr }); }
    template<typename ...Args>
    static void createInPlace(MultiNode *n, const Key &k, Args &&... args)
    { new (n) MultiNode(k, new Chain{ T(std::forward<Args>(args)...), nullptr }); }

    MultiNode(const Key &k, Chain *c)
        : key(k),
          value(c)
    {}
    MultiNode(Key &&k, Chain *c) noexcept(std::is_nothrow_move_assignable_v<Key>)
        : key(std::move(k)),
          value(c)
    {}

    MultiNode(MultiNode &&other)
        : key(other.key),
          value(qExchange(other.value, nullptr))
    {
    }

    MultiNode(const MultiNode &other)
        : key(other.key)
    {
        Chain *c = other.value;
        Chain **e = &value;
        while (c) {
            Chain *chain = new Chain{ c->value, nullptr };
            *e = chain;
            e = &chain->next;
            c = c->next;
        }
    }
    ~MultiNode()
    {
        if (value)
            value->free();
    }
    static qsizetype freeChain(MultiNode *n) noexcept(std::is_nothrow_destructible_v<T>)
    {
        qsizetype size = n->value->free();
        n->value = nullptr;
        return size;
    }
    template<typename ...Args>
    void insertMulti(Args &&... args)
    {
        Chain *e = new Chain{ T(std::forward<Args>(args)...), nullptr };
        e->next = qExchange(value, e);
    }
    template<typename ...Args>
    void emplaceValue(Args &&... args)
    {
        value->value = T(std::forward<Args>(args)...);
    }
};

template<typename  Node>
constexpr bool isRelocatable()
{
    return QTypeInfo<typename Node::KeyType>::isRelocatable && QTypeInfo<typename Node::ValueType>::isRelocatable;
}

// Regular hash tables consist of a list of buckets that can store Nodes. But simply allocating one large array of buckets
// would waste a lot of memory. To avoid this, we split the vector of buckets up into a vector of Spans. Each Span represents
// NEntries buckets. To quickly find the correct Span that holds a bucket, NEntries must be a power of two.
//
// Inside each Span, there is an offset array that represents the actual buckets. offsets contains either an index into the
// actual storage space for the Nodes (the 'entries' member) or 0xff (UnusedEntry) to flag that the bucket is empty.
// As we have only 128 entries per Span, the offset array can be represented using an unsigned char. This trick makes the hash
// table have a very small memory overhead compared to many other implementations.
template<typename Node>
struct Span {
    enum {
        NEntries = 128,
        LocalBucketMask = (NEntries - 1),
        UnusedEntry = 0xff
    };
    static_assert ((NEntries & LocalBucketMask) == 0, "EntriesPerSpan must be a power of two.");

    // Entry is a slot available for storing a Node. The Span holds a pointer to
    // an array of Entries. Upon construction of the array, those entries are
    // unused, and nextFree() is being used to set up a singly linked list
    // of free entries.
    // When a node gets inserted, the first free entry is being picked, removed
    // from the singly linked list and the Node gets constructed in place.
    struct Entry {
        typename std::aligned_storage<sizeof(Node), alignof(Node)>::type storage;

        unsigned char &nextFree() { return *reinterpret_cast<unsigned char *>(&storage); }
        Node &node() { return *reinterpret_cast<Node *>(&storage); }
    };

    unsigned char offsets[NEntries];
    Entry *entries = nullptr;
    unsigned char allocated = 0;
    unsigned char nextFree = 0;
    Span() noexcept
    {
        memset(offsets, UnusedEntry, sizeof(offsets));
    }
    ~Span()
    {
        freeData();
    }
    void freeData() noexcept(std::is_nothrow_destructible<Node>::value)
    {
        if (entries) {
            if constexpr (!std::is_trivially_destructible<Node>::value) {
                for (auto o : offsets) {
                    if (o != UnusedEntry)
                        entries[o].node().~Node();
                }
            }
            delete[] entries;
            entries = nullptr;
        }
    }
    Node *insert(size_t i)
    {
        Q_ASSERT(i < NEntries);
        Q_ASSERT(offsets[i] == UnusedEntry);
        if (nextFree == allocated)
            addStorage();
        unsigned char entry = nextFree;
        Q_ASSERT(entry < allocated);
        nextFree = entries[entry].nextFree();
        offsets[i] = entry;
        return &entries[entry].node();
    }
    void erase(size_t bucket) noexcept(std::is_nothrow_destructible<Node>::value)
    {
        Q_ASSERT(bucket < NEntries);
        Q_ASSERT(offsets[bucket] != UnusedEntry);

        unsigned char entry = offsets[bucket];
        offsets[bucket] = UnusedEntry;

        entries[entry].node().~Node();
        entries[entry].nextFree() = nextFree;
        nextFree = entry;
    }
    size_t offset(size_t i) const noexcept
    {
        return offsets[i];
    }
    bool hasNode(size_t i) const noexcept
    {
        return (offsets[i] != UnusedEntry);
    }
    Node &at(size_t i) noexcept
    {
        Q_ASSERT(i < NEntries);
        Q_ASSERT(offsets[i] != UnusedEntry);

        return entries[offsets[i]].node();
    }
    const Node &at(size_t i) const noexcept
    {
        Q_ASSERT(i < NEntries);
        Q_ASSERT(offsets[i] != UnusedEntry);

        return entries[offsets[i]].node();
    }
    Node &atOffset(size_t o) noexcept
    {
        Q_ASSERT(o < allocated);

        return entries[o].node();
    }
    const Node &atOffset(size_t o) const noexcept
    {
        Q_ASSERT(o < allocated);

        return entries[o].node();
    }
    void moveLocal(size_t from, size_t to) noexcept
    {
        Q_ASSERT(offsets[from] != UnusedEntry);
        Q_ASSERT(offsets[to] == UnusedEntry);
        offsets[to] = offsets[from];
        offsets[from] = UnusedEntry;
    }
    void moveFromSpan(Span &fromSpan, size_t fromIndex, size_t to) noexcept(std::is_nothrow_move_constructible_v<Node>)
    {
        Q_ASSERT(to < NEntries);
        Q_ASSERT(offsets[to] == UnusedEntry);
        Q_ASSERT(fromIndex < NEntries);
        Q_ASSERT(fromSpan.offsets[fromIndex] != UnusedEntry);
        if (nextFree == allocated)
            addStorage();
        Q_ASSERT(nextFree < allocated);
        offsets[to] = nextFree;
        Entry &toEntry = entries[nextFree];
        nextFree = toEntry.nextFree();

        size_t fromOffset = fromSpan.offsets[fromIndex];
        fromSpan.offsets[fromIndex] = UnusedEntry;
        Entry &fromEntry = fromSpan.entries[fromOffset];

        if constexpr (isRelocatable<Node>()) {
            memcpy(&toEntry, &fromEntry, sizeof(Entry));
        } else {
            new (&toEntry.node()) Node(std::move(fromEntry.node()));
            fromEntry.node().~Node();
        }
        fromEntry.nextFree() = fromSpan.nextFree;
        fromSpan.nextFree = static_cast<unsigned char>(fromOffset);
    }

    void addStorage()
    {
        Q_ASSERT(allocated < NEntries);
        Q_ASSERT(nextFree == allocated);
        // the hash table should always be between 25 and 50% full
        // this implies that we on average have between 32 and 64 entries
        // in here. The likelihood of having below 16 entries is very small,
        // so start with that and increment by 16 each time we need to add
        // some more space
        const size_t increment = NEntries / 8;
        size_t alloc = allocated + increment;
        Entry *newEntries = new Entry[alloc];
        // we only add storage if the previous storage was fully filled, so
        // simply copy the old data over
        if constexpr (isRelocatable<Node>()) {
            if (allocated)
                memcpy(newEntries, entries, allocated * sizeof(Entry));
        } else {
            for (size_t i = 0; i < allocated; ++i) {
                new (&newEntries[i].node()) Node(std::move(entries[i].node()));
                entries[i].node().~Node();
            }
        }
        for (size_t i = allocated; i < allocated + increment; ++i) {
            newEntries[i].nextFree() = uchar(i + 1);
        }
        delete[] entries;
        entries = newEntries;
        allocated = uchar(alloc);
    }
};

// QHash uses a power of two growth policy.
namespace GrowthPolicy {
inline constexpr size_t maxNumBuckets() noexcept
{
    // ensure the size of a Span does not depend on the template parameters
    using Node1 = Node<int, int>;
    using Node2 = Node<char, void *>;
    using Node3 = Node<qsizetype, QHashDummyValue>;
    static_assert(sizeof(Span<Node1>) == sizeof(Span<Node2>));
    static_assert(sizeof(Span<Node1>) == sizeof(Span<Node3>));
    static_assert(int(Span<Node1>::NEntries) == int(Span<Node2>::NEntries));
    static_assert(int(Span<Node1>::NEntries) == int(Span<Node3>::NEntries));

    // Maximum is 2^31-1 or 2^63-1 bytes (limited by qsizetype and ptrdiff_t)
    size_t max = (std::numeric_limits<ptrdiff_t>::max)();
    return max / sizeof(Span<Node1>) * Span<Node1>::NEntries;
}
inline constexpr size_t bucketsForCapacity(size_t requestedCapacity) noexcept
{
    if (requestedCapacity <= 8)
        return 16;
    if (requestedCapacity >= maxNumBuckets())
        return maxNumBuckets();
    return qNextPowerOfTwo(QIntegerForSize<sizeof(size_t)>::Unsigned(2 * requestedCapacity - 1));
}
inline constexpr size_t bucketForHash(size_t nBuckets, size_t hash) noexcept
{
    return hash & (nBuckets - 1);
}
} // namespace GrowthPolicy

template <typename Node>
struct iterator;

template <typename Node>
struct Data
{
    using Key = typename Node::KeyType;
    using T = typename Node::ValueType;
    using Span = QHashPrivate::Span<Node>;
    using iterator = QHashPrivate::iterator<Node>;

    QtPrivate::RefCount ref = {{1}};
    size_t size = 0;
    size_t numBuckets = 0;
    size_t seed = 0;


    Span *spans = nullptr;

    Data(size_t reserve = 0)
    {
        numBuckets = GrowthPolicy::bucketsForCapacity(reserve);
        size_t nSpans = (numBuckets + Span::LocalBucketMask) / Span::NEntries;
        spans = new Span[nSpans];
        seed = QHashSeed::globalSeed();
    }
    Data(const Data &other, size_t reserved = 0)
        : size(other.size),
          numBuckets(other.numBuckets),
          seed(other.seed)
    {
        if (reserved)
            numBuckets = GrowthPolicy::bucketsForCapacity(qMax(size, reserved));
        bool resized = numBuckets != other.numBuckets;
        size_t nSpans = (numBuckets + Span::LocalBucketMask) / Span::NEntries;
        spans = new Span[nSpans];
        size_t otherNSpans = (other.numBuckets + Span::LocalBucketMask) / Span::NEntries;

        for (size_t s = 0; s < otherNSpans; ++s) {
            const Span &span = other.spans[s];
            for (size_t index = 0; index < Span::NEntries; ++index) {
                if (!span.hasNode(index))
                    continue;
                const Node &n = span.at(index);
                iterator it = resized ? find(n.key) : iterator{ this, s*Span::NEntries + index };
                Q_ASSERT(it.isUnused());
                Node *newNode = spans[it.span()].insert(it.index());
                new (newNode) Node(n);
            }
        }
    }

    static Data *detached(Data *d, size_t size = 0)
    {
        if (!d)
            return new Data(size);
        Data *dd = new Data(*d, size);
        if (!d->ref.deref())
            delete d;
        return dd;
    }

    void clear()
    {
        delete[] spans;
        spans = nullptr;
        size = 0;
        numBuckets = 0;
    }

    iterator detachedIterator(iterator other) const noexcept
    {
        return iterator{this, other.bucket};
    }

    iterator begin() const noexcept
    {
        iterator it{ this, 0 };
        if (it.isUnused())
            ++it;
        return it;
    }

    constexpr iterator end() const noexcept
    {
        return iterator();
    }

    void rehash(size_t sizeHint = 0)
    {
        if (sizeHint == 0)
            sizeHint = size;
        size_t newBucketCount = GrowthPolicy::bucketsForCapacity(sizeHint);

        Span *oldSpans = spans;
        size_t oldBucketCount = numBuckets;
        size_t nSpans = (newBucketCount + Span::LocalBucketMask) / Span::NEntries;
        spans = new Span[nSpans];
        numBuckets = newBucketCount;
        size_t oldNSpans = (oldBucketCount + Span::LocalBucketMask) / Span::NEntries;

        for (size_t s = 0; s < oldNSpans; ++s) {
            Span &span = oldSpans[s];
            for (size_t index = 0; index < Span::NEntries; ++index) {
                if (!span.hasNode(index))
                    continue;
                Node &n = span.at(index);
                iterator it = find(n.key);
                Q_ASSERT(it.isUnused());
                Node *newNode = spans[it.span()].insert(it.index());
                new (newNode) Node(std::move(n));
            }
            span.freeData();
        }
        delete[] oldSpans;
    }

    size_t nextBucket(size_t bucket) const noexcept
    {
        ++bucket;
        if (bucket == numBuckets)
            bucket = 0;
        return bucket;
    }

    float loadFactor() const noexcept
    {
        return float(size)/numBuckets;
    }
    bool shouldGrow() const noexcept
    {
        return size >= (numBuckets >> 1);
    }

    iterator find(const Key &key) const noexcept
    {
        Q_ASSERT(numBuckets > 0);
        size_t hash = QHashPrivate::calculateHash(key, seed);
        size_t bucket = GrowthPolicy::bucketForHash(numBuckets, hash);
        // loop over the buckets until we find the entry we search for
        // or an empty slot, in which case we know the entry doesn't exist
        while (true) {
            // Split the bucket into the indexex of span array, and the local
            // offset inside the span
            size_t span = bucket / Span::NEntries;
            size_t index = bucket & Span::LocalBucketMask;
            Span &s = spans[span];
            size_t offset = s.offset(index);
            if (offset == Span::UnusedEntry) {
                return iterator{ this, bucket };
            } else {
                Node &n = s.atOffset(offset);
                if (qHashEquals(n.key, key))
                    return iterator{ this, bucket };
            }
            bucket = nextBucket(bucket);
        }
    }

    Node *findNode(const Key &key) const noexcept
    {
        if (!size)
            return nullptr;
        iterator it = find(key);
        if (it.isUnused())
            return nullptr;
        return it.node();
    }

    struct InsertionResult
    {
        iterator it;
        bool initialized;
    };

    InsertionResult findOrInsert(const Key &key) noexcept
    {
        iterator it;
        if (numBuckets > 0) {
            it = find(key);
            if (!it.isUnused())
                return { it, true };
        }
        if (shouldGrow()) {
            rehash(size + 1);
            it = find(key); // need to get a new iterator after rehashing
        }
        Q_ASSERT(it.d);
        Q_ASSERT(it.isUnused());
        spans[it.span()].insert(it.index());
        ++size;
        return { it, false };
    }

    iterator erase(iterator it) noexcept(std::is_nothrow_destructible<Node>::value)
    {
        size_t bucket = it.bucket;
        size_t span = bucket / Span::NEntries;
        size_t index = bucket & Span::LocalBucketMask;
        Q_ASSERT(spans[span].hasNode(index));
        spans[span].erase(index);
        --size;

        // re-insert the following entries to avoid holes
        size_t hole = bucket;
        size_t next = bucket;
        while (true) {
            next = nextBucket(next);
            size_t nextSpan = next / Span::NEntries;
            size_t nextIndex = next & Span::LocalBucketMask;
            if (!spans[nextSpan].hasNode(nextIndex))
                break;
            size_t hash = QHashPrivate::calculateHash(spans[nextSpan].at(nextIndex).key, seed);
            size_t newBucket = GrowthPolicy::bucketForHash(numBuckets, hash);
            while (true) {
                if (newBucket == next) {
                    // nothing to do, item is at the right plae
                    break;
                } else if (newBucket == hole) {
                    // move into hole
                    size_t holeSpan = hole / Span::NEntries;
                    size_t holeIndex = hole & Span::LocalBucketMask;
                    if (nextSpan == holeSpan) {
                        spans[holeSpan].moveLocal(nextIndex, holeIndex);
                    } else {
                        // move between spans, more expensive
                        spans[holeSpan].moveFromSpan(spans[nextSpan], nextIndex, holeIndex);
                    }
                    hole = next;
                    break;
                }
                newBucket = nextBucket(newBucket);
            }
        }

        // return correct position of the next element
        if (bucket == numBuckets - 1 || !spans[span].hasNode(index))
            ++it;
        return it;
    }

    ~Data()
    {
        delete [] spans;
    }
};

template <typename Node>
struct iterator {
    using Span = QHashPrivate::Span<Node>;

    const Data<Node> *d = nullptr;
    size_t bucket = 0;

    size_t span() const noexcept { return bucket / Span::NEntries; }
    size_t index() const noexcept { return bucket & Span::LocalBucketMask; }
    inline bool isUnused() const noexcept { return !d->spans[span()].hasNode(index()); }

    inline Node *node() const noexcept
    {
        Q_ASSERT(!isUnused());
        return &d->spans[span()].at(index());
    }
    bool atEnd() const noexcept { return !d; }

    iterator operator++() noexcept
    {
        while (true) {
            ++bucket;
            if (bucket == d->numBuckets) {
                d = nullptr;
                bucket = 0;
                break;
            }
            if (!isUnused())
                break;
        }
        return *this;
    }
    bool operator==(iterator other) const noexcept
    { return d == other.d && bucket == other.bucket; }
    bool operator!=(iterator other) const noexcept
    { return !(*this == other); }
};



} // namespace QHashPrivate

template <typename Key, typename T>
class QHash
{
    using Node = QHashPrivate::Node<Key, T>;
    using Data = QHashPrivate::Data<Node>;
    friend class QSet<Key>;
    friend class QMultiHash<Key, T>;
    friend tst_QHash;

    Data *d = nullptr;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = T;
    using size_type = qsizetype;
    using difference_type = qsizetype;
    using reference = T &;
    using const_reference = const T &;

    inline QHash() noexcept = default;
    inline QHash(std::initializer_list<std::pair<Key,T> > list)
        : d(new Data(list.size()))
    {
        for (typename std::initializer_list<std::pair<Key,T> >::const_iterator it = list.begin(); it != list.end(); ++it)
            insert(it->first, it->second);
    }
    QHash(const QHash &other) noexcept
        : d(other.d)
    {
        if (d)
            d->ref.ref();
    }
    ~QHash()
    {
        static_assert(std::is_nothrow_destructible_v<Key>, "Types with throwing destructors are not supported in Qt containers.");
        static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

        if (d && !d->ref.deref())
            delete d;
    }

    QHash &operator=(const QHash &other) noexcept(std::is_nothrow_destructible<Node>::value)
    {
        if (d != other.d) {
            Data *o = other.d;
            if (o)
                o->ref.ref();
            if (d && !d->ref.deref())
                delete d;
            d = o;
        }
        return *this;
    }

    QHash(QHash &&other) noexcept
        : d(std::exchange(other.d, nullptr))
    {
    }
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QHash)
#ifdef Q_QDOC
    template <typename InputIterator>
    QHash(InputIterator f, InputIterator l);
#else
    template <typename InputIterator, QtPrivate::IfAssociativeIteratorHasKeyAndValue<InputIterator> = true>
    QHash(InputIterator f, InputIterator l)
        : QHash()
    {
        QtPrivate::reserveIfForwardIterator(this, f, l);
        for (; f != l; ++f)
            insert(f.key(), f.value());
    }

    template <typename InputIterator, QtPrivate::IfAssociativeIteratorHasFirstAndSecond<InputIterator> = true>
    QHash(InputIterator f, InputIterator l)
        : QHash()
    {
        QtPrivate::reserveIfForwardIterator(this, f, l);
        for (; f != l; ++f)
            insert(f->first, f->second);
    }
#endif
    void swap(QHash &other) noexcept { qt_ptr_swap(d, other.d); }

#ifndef Q_CLANG_QDOC
    template <typename AKey = Key, typename AT = T>
    QTypeTraits::compare_eq_result_container<QHash, AKey, AT> operator==(const QHash &other) const noexcept
    {
        if (d == other.d)
            return true;
        if (size() != other.size())
            return false;

        for (const_iterator it = other.begin(); it != other.end(); ++it) {
            const_iterator i = find(it.key());
            if (i == end() || !i.i.node()->valuesEqual(it.i.node()))
                return false;
        }
        // all values must be the same as size is the same
        return true;
    }
    template <typename AKey = Key, typename AT = T>
    QTypeTraits::compare_eq_result_container<QHash, AKey, AT> operator!=(const QHash &other) const noexcept
    { return !(*this == other); }
#else
    bool operator==(const QHash &other) const;
    bool operator!=(const QHash &other) const;
#endif // Q_CLANG_QDOC

    inline qsizetype size() const noexcept { return d ? qsizetype(d->size) : 0; }
    inline bool isEmpty() const noexcept { return !d || d->size == 0; }

    inline qsizetype capacity() const noexcept { return d ? qsizetype(d->numBuckets >> 1) : 0; }
    void reserve(qsizetype size)
    {
        if (isDetached())
            d->rehash(size);
        else
            d = Data::detached(d, size_t(size));
    }
    inline void squeeze()
    {
        if (capacity())
            reserve(0);
    }

    inline void detach() { if (!d || d->ref.isShared()) d = Data::detached(d); }
    inline bool isDetached() const noexcept { return d && !d->ref.isShared(); }
    bool isSharedWith(const QHash &other) const noexcept { return d == other.d; }

    void clear() noexcept(std::is_nothrow_destructible<Node>::value)
    {
        if (d && !d->ref.deref())
            delete d;
        d = nullptr;
    }

    bool remove(const Key &key)
    {
        if (isEmpty()) // prevents detaching shared null
            return false;
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            return false;
        d->erase(it);
        return true;
    }
    template <typename Predicate>
    qsizetype removeIf(Predicate pred)
    {
        return QtPrivate::associative_erase_if(*this, pred);
    }
    T take(const Key &key)
    {
        if (isEmpty()) // prevents detaching shared null
            return T();
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            return T();
        T value = it.node()->takeValue();
        d->erase(it);
        return value;
    }

    bool contains(const Key &key) const noexcept
    {
        if (!d)
            return false;
        return d->findNode(key) != nullptr;
    }
    qsizetype count(const Key &key) const noexcept
    {
        return contains(key) ? 1 : 0;
    }

private:
    const Key *keyImpl(const T &value) const noexcept
    {
        if (d) {
            const_iterator i = begin();
            while (i != end()) {
                if (i.value() == value)
                    return &i.key();
                ++i;
            }
        }

        return nullptr;
    }

public:
    Key key(const T &value) const noexcept
    {
        if (auto *k = keyImpl(value))
            return *k;
        else
            return Key();
    }
    Key key(const T &value, const Key &defaultKey) const noexcept
    {
        if (auto *k = keyImpl(value))
            return *k;
        else
            return defaultKey;
    }

private:
    T *valueImpl(const Key &key) const noexcept
    {
        if (d) {
            Node *n = d->findNode(key);
            if (n)
                return &n->value;
        }
        return nullptr;
    }
public:
    T value(const Key &key) const noexcept
    {
        if (T *v = valueImpl(key))
            return *v;
        else
            return T();
    }

    T value(const Key &key, const T &defaultValue) const noexcept
    {
        if (T *v = valueImpl(key))
            return *v;
        else
            return defaultValue;
    }

    T &operator[](const Key &key)
    {
        const auto copy = isDetached() ? QHash() : *this; // keep 'key' alive across the detach
        detach();
        auto result = d->findOrInsert(key);
        Q_ASSERT(!result.it.atEnd());
        if (!result.initialized)
            Node::createInPlace(result.it.node(), key, T());
        return result.it.node()->value;
    }

    const T operator[](const Key &key) const noexcept
    {
        return value(key);
    }

    QList<Key> keys() const { return QList<Key>(keyBegin(), keyEnd()); }
    QList<Key> keys(const T &value) const
    {
        QList<Key> res;
        const_iterator i = begin();
        while (i != end()) {
            if (i.value() == value)
                res.append(i.key());
            ++i;
        }
        return res;
    }
    QList<T> values() const { return QList<T>(begin(), end()); }

    class const_iterator;

    class iterator
    {
        using piter = typename QHashPrivate::iterator<Node>;
        friend class const_iterator;
        friend class QHash<Key, T>;
        friend class QSet<Key>;
        piter i;
        explicit inline iterator(piter it) noexcept : i(it) { }

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        constexpr iterator() noexcept = default;

        inline const Key &key() const noexcept { return i.node()->key; }
        inline T &value() const noexcept { return i.node()->value; }
        inline T &operator*() const noexcept { return i.node()->value; }
        inline T *operator->() const noexcept { return &i.node()->value; }
        inline bool operator==(const iterator &o) const noexcept { return i == o.i; }
        inline bool operator!=(const iterator &o) const noexcept { return i != o.i; }

        inline iterator &operator++() noexcept
        {
            ++i;
            return *this;
        }
        inline iterator operator++(int) noexcept
        {
            iterator r = *this;
            ++i;
            return r;
        }

        inline bool operator==(const const_iterator &o) const noexcept { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const noexcept { return i != o.i; }
    };
    friend class iterator;

    class const_iterator
    {
        using piter = typename QHashPrivate::iterator<Node>;
        friend class iterator;
        friend class QHash<Key, T>;
        friend class QSet<Key>;
        piter i;
        explicit inline const_iterator(piter it) : i(it) { }

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        constexpr const_iterator() noexcept = default;
        inline const_iterator(const iterator &o) noexcept : i(o.i) { }

        inline const Key &key() const noexcept { return i.node()->key; }
        inline const T &value() const noexcept { return i.node()->value; }
        inline const T &operator*() const noexcept { return i.node()->value; }
        inline const T *operator->() const noexcept { return &i.node()->value; }
        inline bool operator==(const const_iterator &o) const noexcept { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const noexcept { return i != o.i; }

        inline const_iterator &operator++() noexcept
        {
            ++i;
            return *this;
        }
        inline const_iterator operator++(int) noexcept
        {
            const_iterator r = *this;
            ++i;
            return r;
        }
    };
    friend class const_iterator;

    class key_iterator
    {
        const_iterator i;

    public:
        typedef typename const_iterator::iterator_category iterator_category;
        typedef qptrdiff difference_type;
        typedef Key value_type;
        typedef const Key *pointer;
        typedef const Key &reference;

        key_iterator() noexcept = default;
        explicit key_iterator(const_iterator o) noexcept : i(o) { }

        const Key &operator*() const noexcept { return i.key(); }
        const Key *operator->() const noexcept { return &i.key(); }
        bool operator==(key_iterator o) const noexcept { return i == o.i; }
        bool operator!=(key_iterator o) const noexcept { return i != o.i; }

        inline key_iterator &operator++() noexcept { ++i; return *this; }
        inline key_iterator operator++(int) noexcept { return key_iterator(i++);}
        const_iterator base() const noexcept { return i; }
    };

    typedef QKeyValueIterator<const Key&, const T&, const_iterator> const_key_value_iterator;
    typedef QKeyValueIterator<const Key&, T&, iterator> key_value_iterator;

    // STL style
    inline iterator begin() { detach(); return iterator(d->begin()); }
    inline const_iterator begin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline const_iterator cbegin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline const_iterator constBegin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline iterator end() noexcept { return iterator(); }
    inline const_iterator end() const noexcept { return const_iterator(); }
    inline const_iterator cend() const noexcept { return const_iterator(); }
    inline const_iterator constEnd() const noexcept { return const_iterator(); }
    inline key_iterator keyBegin() const noexcept { return key_iterator(begin()); }
    inline key_iterator keyEnd() const noexcept { return key_iterator(end()); }
    inline key_value_iterator keyValueBegin() { return key_value_iterator(begin()); }
    inline key_value_iterator keyValueEnd() { return key_value_iterator(end()); }
    inline const_key_value_iterator keyValueBegin() const noexcept { return const_key_value_iterator(begin()); }
    inline const_key_value_iterator constKeyValueBegin() const noexcept { return const_key_value_iterator(begin()); }
    inline const_key_value_iterator keyValueEnd() const noexcept { return const_key_value_iterator(end()); }
    inline const_key_value_iterator constKeyValueEnd() const noexcept { return const_key_value_iterator(end()); }

    iterator erase(const_iterator it)
    {
        Q_ASSERT(it != constEnd());
        detach();
        // ensure a valid iterator across the detach:
        iterator i = iterator{d->detachedIterator(it.i)};

        i.i = d->erase(i.i);
        return i;
    }

    QPair<iterator, iterator> equal_range(const Key &key)
    {
        auto first = find(key);
        auto second = first;
        if (second != iterator())
            ++second;
        return qMakePair(first, second);
    }

    QPair<const_iterator, const_iterator> equal_range(const Key &key) const noexcept
    {
        auto first = find(key);
        auto second = first;
        if (second != iterator())
            ++second;
        return qMakePair(first, second);
    }

    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline qsizetype count() const noexcept { return d ? qsizetype(d->size) : 0; }
    iterator find(const Key &key)
    {
        if (isEmpty()) // prevents detaching shared null
            return end();
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);
        if (it.isUnused())
            it = d->end();
        return iterator(it);
    }
    const_iterator find(const Key &key) const noexcept
    {
        if (isEmpty())
            return end();
        auto it = d->find(key);
        if (it.isUnused())
            it = d->end();
        return const_iterator(it);
    }
    const_iterator constFind(const Key &key) const noexcept
    {
        return find(key);
    }
    iterator insert(const Key &key, const T &value)
    {
        return emplace(key, value);
    }

    void insert(const QHash &hash)
    {
        if (d == hash.d || !hash.d)
            return;
        if (!d) {
            *this = hash;
            return;
        }

        detach();

        for (auto it = hash.begin(); it != hash.end(); ++it)
            emplace(it.key(), it.value());
    }

    template <typename ...Args>
    iterator emplace(const Key &key, Args &&... args)
    {
        Key copy = key; // Needs to be explicit for MSVC 2019
        return emplace(std::move(copy), std::forward<Args>(args)...);
    }

    template <typename ...Args>
    iterator emplace(Key &&key, Args &&... args)
    {
        if (isDetached()) {
            if (d->shouldGrow()) // Construct the value now so that no dangling references are used
                return emplace_helper(std::move(key), T(std::forward<Args>(args)...));
            return emplace_helper(std::move(key), std::forward<Args>(args)...);
        }
        // else: we must detach
        const auto copy = *this; // keep 'args' alive across the detach/growth
        detach();
        return emplace_helper(std::move(key), std::forward<Args>(args)...);
    }

    float load_factor() const noexcept { return d ? d->loadFactor() : 0; }
    static float max_load_factor() noexcept { return 0.5; }
    size_t bucket_count() const noexcept { return d ? d->numBuckets : 0; }
    static size_t max_bucket_count() noexcept { return QHashPrivate::GrowthPolicy::maxNumBuckets(); }

    inline bool empty() const noexcept { return isEmpty(); }

private:
    template <typename ...Args>
    iterator emplace_helper(Key &&key, Args &&... args)
    {
        auto result = d->findOrInsert(key);
        if (!result.initialized)
            Node::createInPlace(result.it.node(), std::move(key), std::forward<Args>(args)...);
        else
            result.it.node()->emplaceValue(std::forward<Args>(args)...);
        return iterator(result.it);
    }
};



template <typename Key, typename T>
class QMultiHash
{
    using Node = QHashPrivate::MultiNode<Key, T>;
    using Data = QHashPrivate::Data<Node>;
    using Chain = QHashPrivate::MultiNodeChain<T>;

    Data *d = nullptr;
    qsizetype m_size = 0;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = T;
    using size_type = qsizetype;
    using difference_type = qsizetype;
    using reference = T &;
    using const_reference = const T &;

    QMultiHash() noexcept = default;
    inline QMultiHash(std::initializer_list<std::pair<Key,T> > list)
        : d(new Data(list.size()))
    {
        for (typename std::initializer_list<std::pair<Key,T> >::const_iterator it = list.begin(); it != list.end(); ++it)
            insert(it->first, it->second);
    }
#ifdef Q_QDOC
    template <typename InputIterator>
    QMultiHash(InputIterator f, InputIterator l);
#else
    template <typename InputIterator, QtPrivate::IfAssociativeIteratorHasKeyAndValue<InputIterator> = true>
    QMultiHash(InputIterator f, InputIterator l)
    {
        QtPrivate::reserveIfForwardIterator(this, f, l);
        for (; f != l; ++f)
            insert(f.key(), f.value());
    }

    template <typename InputIterator, QtPrivate::IfAssociativeIteratorHasFirstAndSecond<InputIterator> = true>
    QMultiHash(InputIterator f, InputIterator l)
    {
        QtPrivate::reserveIfForwardIterator(this, f, l);
        for (; f != l; ++f)
            insert(f->first, f->second);
    }
#endif
    QMultiHash(const QMultiHash &other) noexcept
        : d(other.d), m_size(other.m_size)
    {
        if (d)
            d->ref.ref();
    }
    ~QMultiHash()
    {
        static_assert(std::is_nothrow_destructible_v<Key>, "Types with throwing destructors are not supported in Qt containers.");
        static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

        if (d && !d->ref.deref())
            delete d;
    }

    QMultiHash &operator=(const QMultiHash &other) noexcept(std::is_nothrow_destructible<Node>::value)
    {
        if (d != other.d) {
            Data *o = other.d;
            if (o)
                o->ref.ref();
            if (d && !d->ref.deref())
                delete d;
            d = o;
            m_size = other.m_size;
        }
        return *this;
    }
    QMultiHash(QMultiHash &&other) noexcept
        : d(qExchange(other.d, nullptr)),
          m_size(qExchange(other.m_size, 0))
    {
    }
    QMultiHash &operator=(QMultiHash &&other) noexcept(std::is_nothrow_destructible<Node>::value)
    {
        QMultiHash moved(std::move(other));
        swap(moved);
        return *this;
    }

    explicit QMultiHash(const QHash<Key, T> &other)
        : QMultiHash(other.begin(), other.end())
    {}

    explicit QMultiHash(QHash<Key, T> &&other)
    {
        unite(std::move(other));
    }

    void swap(QMultiHash &other) noexcept
    {
        qt_ptr_swap(d, other.d);
        std::swap(m_size, other.m_size);
    }

#ifndef Q_CLANG_QDOC
    template <typename AKey = Key, typename AT = T>
    QTypeTraits::compare_eq_result_container<QMultiHash, AKey, AT> operator==(const QMultiHash &other) const noexcept
    {
        if (d == other.d)
            return true;
        if (m_size != other.m_size)
            return false;
        if (m_size == 0)
            return true;
        // equal size, and both non-zero size => d pointers allocated for both
        Q_ASSERT(d);
        Q_ASSERT(other.d);
        if (d->size != other.d->size)
            return false;
        for (auto it = other.d->begin(); it != other.d->end(); ++it) {
            auto *n = d->findNode(it.node()->key);
            if (!n)
                return false;
            Chain *e = it.node()->value;
            while (e) {
                Chain *oe = n->value;
                while (oe) {
                    if (oe->value == e->value)
                        break;
                    oe = oe->next;
                }
                if (!oe)
                    return false;
                e = e->next;
            }
        }
        // all values must be the same as size is the same
        return true;
    }
    template <typename AKey = Key, typename AT = T>
    QTypeTraits::compare_eq_result_container<QMultiHash, AKey, AT> operator!=(const QMultiHash &other) const noexcept
    { return !(*this == other); }
#else
    bool operator==(const QMultiHash &other) const;
    bool operator!=(const QMultiHash &other) const;
#endif // Q_CLANG_QDOC

    inline qsizetype size() const noexcept { return m_size; }

    inline bool isEmpty() const noexcept { return !m_size; }

    inline qsizetype capacity() const noexcept { return d ? qsizetype(d->numBuckets >> 1) : 0; }
    void reserve(qsizetype size)
    {
        if (isDetached())
            d->rehash(size);
        else
            d = Data::detached(d, size_t(size));
    }
    inline void squeeze() { reserve(0); }

    inline void detach() { if (!d || d->ref.isShared()) d = Data::detached(d); }
    inline bool isDetached() const noexcept { return d && !d->ref.isShared(); }
    bool isSharedWith(const QMultiHash &other) const noexcept { return d == other.d; }

    void clear() noexcept(std::is_nothrow_destructible<Node>::value)
    {
        if (d && !d->ref.deref())
            delete d;
        d = nullptr;
        m_size = 0;
    }

    qsizetype remove(const Key &key)
    {
        if (isEmpty()) // prevents detaching shared null
            return 0;
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            return 0;
        qsizetype n = Node::freeChain(it.node());
        m_size -= n;
        Q_ASSERT(m_size >= 0);
        d->erase(it);
        return n;
    }
    template <typename Predicate>
    qsizetype removeIf(Predicate pred)
    {
        return QtPrivate::associative_erase_if(*this, pred);
    }
    T take(const Key &key)
    {
        if (isEmpty()) // prevents detaching shared null
            return T();
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            return T();
        Chain *e = it.node()->value;
        Q_ASSERT(e);
        T t = std::move(e->value);
        if (e->next) {
            it.node()->value = e->next;
            delete e;
        } else {
            // erase() deletes the values.
            d->erase(it);
        }
        --m_size;
        Q_ASSERT(m_size >= 0);
        return t;
    }

    bool contains(const Key &key) const noexcept
    {
        if (!d)
            return false;
        return d->findNode(key) != nullptr;
    }

private:
    const Key *keyImpl(const T &value) const noexcept
    {
        if (d) {
            auto i = d->begin();
            while (i != d->end()) {
                Chain *e = i.node()->value;
                if (e->contains(value))
                    return &i.node()->key;
                ++i;
            }
        }

        return nullptr;
    }
public:
    Key key(const T &value) const noexcept
    {
        if (auto *k = keyImpl(value))
            return *k;
        else
            return Key();
    }
    Key key(const T &value, const Key &defaultKey) const noexcept
    {
        if (auto *k = keyImpl(value))
            return *k;
        else
            return defaultKey;
    }

private:
    T *valueImpl(const Key &key) const noexcept
    {
        if (d) {
            Node *n = d->findNode(key);
            if (n) {
                Q_ASSERT(n->value);
                return &n->value->value;
            }
        }
        return nullptr;
    }
public:
    T value(const Key &key) const noexcept
    {
        if (auto *v = valueImpl(key))
            return *v;
        else
            return T();
    }
    T value(const Key &key, const T &defaultValue) const noexcept
    {
        if (auto *v = valueImpl(key))
            return *v;
        else
            return defaultValue;
    }

    T &operator[](const Key &key)
    {
        const auto copy = isDetached() ? QMultiHash() : *this; // keep 'key' alive across the detach
        detach();
        auto result = d->findOrInsert(key);
        Q_ASSERT(!result.it.atEnd());
        if (!result.initialized)
            Node::createInPlace(result.it.node(), key, T());
        return result.it.node()->value->value;
    }

    const T operator[](const Key &key) const noexcept
    {
        return value(key);
    }

    QList<Key> uniqueKeys() const
    {
        QList<Key> res;
        if (d) {
            auto i = d->begin();
            while (i != d->end()) {
                res.append(i.node()->key);
                ++i;
            }
        }
        return res;
    }

    QList<Key> keys() const { return QList<Key>(keyBegin(), keyEnd()); }
    QList<Key> keys(const T &value) const
    {
        QList<Key> res;
        const_iterator i = begin();
        while (i != end()) {
            if (i.value() == value)
                res.append(i.key());
            ++i;
        }
        return res;
    }
    QList<T> values() const { return QList<T>(begin(), end()); }
    QList<T> values(const Key &key) const
    {
        QList<T> values;
        if (d) {
            Node *n = d->findNode(key);
            if (n) {
                Chain *e = n->value;
                while (e) {
                    values.append(e->value);
                    e = e->next;
                }
            }
        }
        return values;
    }

    class const_iterator;

    class iterator
    {
        using piter = typename QHashPrivate::iterator<Node>;
        friend class const_iterator;
        friend class QMultiHash<Key, T>;
        piter i;
        Chain **e = nullptr;
        explicit inline iterator(piter it, Chain **entry = nullptr) noexcept : i(it), e(entry)
        {
            if (!it.atEnd() && !e) {
                e = &it.node()->value;
                Q_ASSERT(e && *e);
            }
        }

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        constexpr iterator() noexcept = default;

        inline const Key &key() const noexcept { return i.node()->key; }
        inline T &value() const noexcept { return (*e)->value; }
        inline T &operator*() const noexcept { return (*e)->value; }
        inline T *operator->() const noexcept { return &(*e)->value; }
        inline bool operator==(const iterator &o) const noexcept { return e == o.e; }
        inline bool operator!=(const iterator &o) const noexcept { return e != o.e; }

        inline iterator &operator++() noexcept {
            Q_ASSERT(e && *e);
            e = &(*e)->next;
            Q_ASSERT(e);
            if (!*e) {
                ++i;
                e = i.atEnd() ? nullptr : &i.node()->value;
            }
            return *this;
        }
        inline iterator operator++(int) noexcept {
            iterator r = *this;
            ++(*this);
            return r;
        }

        inline bool operator==(const const_iterator &o) const noexcept { return e == o.e; }
        inline bool operator!=(const const_iterator &o) const noexcept { return e != o.e; }
    };
    friend class iterator;

    class const_iterator
    {
        using piter = typename QHashPrivate::iterator<Node>;
        friend class iterator;
        friend class QMultiHash<Key, T>;
        piter i;
        Chain **e = nullptr;
        explicit inline const_iterator(piter it, Chain **entry = nullptr) noexcept : i(it), e(entry)
        {
            if (!it.atEnd() && !e) {
                e = &it.node()->value;
                Q_ASSERT(e && *e);
            }
        }

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        constexpr const_iterator() noexcept = default;
        inline const_iterator(const iterator &o) noexcept : i(o.i), e(o.e) { }

        inline const Key &key() const noexcept { return i.node()->key; }
        inline T &value() const noexcept { return (*e)->value; }
        inline T &operator*() const noexcept { return (*e)->value; }
        inline T *operator->() const noexcept { return &(*e)->value; }
        inline bool operator==(const const_iterator &o) const noexcept { return e == o.e; }
        inline bool operator!=(const const_iterator &o) const noexcept { return e != o.e; }

        inline const_iterator &operator++() noexcept {
            Q_ASSERT(e && *e);
            e = &(*e)->next;
            Q_ASSERT(e);
            if (!*e) {
                ++i;
                e = i.atEnd() ? nullptr : &i.node()->value;
            }
            return *this;
        }
        inline const_iterator operator++(int) noexcept
        {
            const_iterator r = *this;
            ++(*this);
            return r;
        }
    };
    friend class const_iterator;

    class key_iterator
    {
        const_iterator i;

    public:
        typedef typename const_iterator::iterator_category iterator_category;
        typedef qptrdiff difference_type;
        typedef Key value_type;
        typedef const Key *pointer;
        typedef const Key &reference;

        key_iterator() noexcept = default;
        explicit key_iterator(const_iterator o) noexcept : i(o) { }

        const Key &operator*() const noexcept { return i.key(); }
        const Key *operator->() const noexcept { return &i.key(); }
        bool operator==(key_iterator o) const noexcept { return i == o.i; }
        bool operator!=(key_iterator o) const noexcept { return i != o.i; }

        inline key_iterator &operator++() noexcept { ++i; return *this; }
        inline key_iterator operator++(int) noexcept { return key_iterator(i++);}
        const_iterator base() const noexcept { return i; }
    };

    typedef QKeyValueIterator<const Key&, const T&, const_iterator> const_key_value_iterator;
    typedef QKeyValueIterator<const Key&, T&, iterator> key_value_iterator;

    // STL style
    inline iterator begin() { detach(); return iterator(d->begin()); }
    inline const_iterator begin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline const_iterator cbegin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline const_iterator constBegin() const noexcept { return d ? const_iterator(d->begin()): const_iterator(); }
    inline iterator end() noexcept { return iterator(); }
    inline const_iterator end() const noexcept { return const_iterator(); }
    inline const_iterator cend() const noexcept { return const_iterator(); }
    inline const_iterator constEnd() const noexcept { return const_iterator(); }
    inline key_iterator keyBegin() const noexcept { return key_iterator(begin()); }
    inline key_iterator keyEnd() const noexcept { return key_iterator(end()); }
    inline key_value_iterator keyValueBegin() noexcept { return key_value_iterator(begin()); }
    inline key_value_iterator keyValueEnd() noexcept { return key_value_iterator(end()); }
    inline const_key_value_iterator keyValueBegin() const noexcept { return const_key_value_iterator(begin()); }
    inline const_key_value_iterator constKeyValueBegin() const noexcept { return const_key_value_iterator(begin()); }
    inline const_key_value_iterator keyValueEnd() const noexcept { return const_key_value_iterator(end()); }
    inline const_key_value_iterator constKeyValueEnd() const noexcept { return const_key_value_iterator(end()); }

    iterator detach(const_iterator it)
    {
        auto i = it.i;
        Chain **e = it.e;
        if (d->ref.isShared()) {
            // need to store iterator position before detaching
            qsizetype n = 0;
            Chain *entry = i.node()->value;
            while (entry != *it.e) {
                ++n;
                entry = entry->next;
            }
            Q_ASSERT(entry);
            detach_helper();

            i = d->detachedIterator(i);
            e = &i.node()->value;
            while (n) {
                e = &(*e)->next;
                --n;
            }
            Q_ASSERT(e && *e);
        }
        return iterator(i, e);
    }

    iterator erase(const_iterator it)
    {
        Q_ASSERT(d);
        iterator iter = detach(it);
        iterator i = iter;
        Chain *e = *i.e;
        Chain *next = e->next;
        *i.e = next;
        delete e;
        if (!next) {
            if (i.e == &i.i.node()->value) {
                // last remaining entry, erase
                i = iterator(d->erase(i.i));
            } else {
                i = iterator(++iter.i);
            }
        }
        --m_size;
        Q_ASSERT(m_size >= 0);
        return i;
    }

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline qsizetype count() const noexcept { return size(); }
    iterator find(const Key &key)
    {
        if (isEmpty())
            return end();
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            it = d->end();
        return iterator(it);
    }
    const_iterator find(const Key &key) const noexcept
    {
        return constFind(key);
    }
    const_iterator constFind(const Key &key) const noexcept
    {
        if (isEmpty())
            return end();
        auto it = d->find(key);
        if (it.isUnused())
            it = d->end();
        return const_iterator(it);
    }
    iterator insert(const Key &key, const T &value)
    {
        return emplace(key, value);
    }

    template <typename ...Args>
    iterator emplace(const Key &key, Args &&... args)
    {
        return emplace(Key(key), std::forward<Args>(args)...);
    }

    template <typename ...Args>
    iterator emplace(Key &&key, Args &&... args)
    {
        if (isDetached()) {
            if (d->shouldGrow()) // Construct the value now so that no dangling references are used
                return emplace_helper(std::move(key), T(std::forward<Args>(args)...));
            return emplace_helper(std::move(key), std::forward<Args>(args)...);
        }
        // else: we must detach
        const auto copy = *this; // keep 'args' alive across the detach/growth
        detach();
        return emplace_helper(std::move(key), std::forward<Args>(args)...);
    }


    float load_factor() const noexcept { return d ? d->loadFactor() : 0; }
    static float max_load_factor() noexcept { return 0.5; }
    size_t bucket_count() const noexcept { return d ? d->numBuckets : 0; }
    static size_t max_bucket_count() noexcept { return QHashPrivate::GrowthPolicy::maxNumBuckets(); }

    inline bool empty() const noexcept { return isEmpty(); }

    inline iterator replace(const Key &key, const T &value)
    {
        return emplaceReplace(key, value);
    }

    template <typename ...Args>
    iterator emplaceReplace(const Key &key, Args &&... args)
    {
        return emplaceReplace(Key(key), std::forward<Args>(args)...);
    }

    template <typename ...Args>
    iterator emplaceReplace(Key &&key, Args &&... args)
    {
        if (isDetached()) {
            if (d->shouldGrow()) // Construct the value now so that no dangling references are used
                return emplaceReplace_helper(std::move(key), T(std::forward<Args>(args)...));
            return emplaceReplace_helper(std::move(key), std::forward<Args>(args)...);
        }
        // else: we must detach
        const auto copy = *this; // keep 'args' alive across the detach/growth
        detach();
        return emplaceReplace_helper(std::move(key), std::forward<Args>(args)...);
    }

    inline QMultiHash &operator+=(const QMultiHash &other)
    { this->unite(other); return *this; }
    inline QMultiHash operator+(const QMultiHash &other) const
    { QMultiHash result = *this; result += other; return result; }

    bool contains(const Key &key, const T &value) const noexcept
    {
        if (isEmpty())
            return false;
        auto n = d->findNode(key);
        if (n == nullptr)
            return false;
        return n->value->contains(value);
    }

    qsizetype remove(const Key &key, const T &value)
    {
        if (isEmpty()) // prevents detaching shared null
            return 0;
        auto it = d->find(key);
        detach();
        it = d->detachedIterator(it);

        if (it.isUnused())
            return 0;
        qsizetype n = 0;
        Chain **e = &it.node()->value;
        while (*e) {
            Chain *entry = *e;
            if (entry->value == value) {
                *e = entry->next;
                delete entry;
                ++n;
            } else {
                e = &entry->next;
            }
        }
        if (!it.node()->value)
            d->erase(it);
        m_size -= n;
        Q_ASSERT(m_size >= 0);
        return n;
    }

    qsizetype count(const Key &key) const noexcept
    {
        if (!d)
            return 0;
        auto it = d->find(key);
        if (it.isUnused())
            return 0;
        qsizetype n = 0;
        Chain *e = it.node()->value;
        while (e) {
            ++n;
            e = e->next;
        }

        return n;
    }

    qsizetype count(const Key &key, const T &value) const noexcept
    {
        if (!d)
            return 0;
        auto it = d->find(key);
        if (it.isUnused())
            return 0;
        qsizetype n = 0;
        Chain *e = it.node()->value;
        while (e) {
            if (e->value == value)
                ++n;
            e = e->next;
        }

        return n;
    }

    iterator find(const Key &key, const T &value)
    {
        if (isEmpty())
            return end();
        const auto copy = isDetached() ? QMultiHash() : *this; // keep 'key'/'value' alive across the detach
        detach();
        auto it = constFind(key, value);
        return iterator(it.i, it.e);
    }
    const_iterator find(const Key &key, const T &value) const noexcept
    {
        return constFind(key, value);
    }
    const_iterator constFind(const Key &key, const T &value) const noexcept
    {
        const_iterator i(constFind(key));
        const_iterator end(constEnd());
        while (i != end && i.key() == key) {
            if (i.value() == value)
                return i;
            ++i;
        }
        return end;
    }

    QMultiHash &unite(const QMultiHash &other)
    {
        if (isEmpty()) {
            *this = other;
        } else if (other.isEmpty()) {
            ;
        } else {
            QMultiHash copy(other);
            detach();
            for (auto cit = copy.cbegin(); cit != copy.cend(); ++cit)
                insert(cit.key(), *cit);
        }
        return *this;
    }

    QMultiHash &unite(const QHash<Key, T> &other)
    {
        for (auto cit = other.cbegin(); cit != other.cend(); ++cit)
            insert(cit.key(), *cit);
        return *this;
    }

    QMultiHash &unite(QHash<Key, T> &&other)
    {
        if (!other.isDetached()) {
            unite(other);
            return *this;
        }
        auto it = other.d->begin();
        for (const auto end = other.d->end(); it != end; ++it)
            emplace(std::move(it.node()->key), std::move(it.node()->takeValue()));
        other.clear();
        return *this;
    }

    QPair<iterator, iterator> equal_range(const Key &key)
    {
        const auto copy = isDetached() ? QMultiHash() : *this; // keep 'key' alive across the detach
        detach();
        auto pair = qAsConst(*this).equal_range(key);
        return qMakePair(iterator(pair.first.i), iterator(pair.second.i));
    }

    QPair<const_iterator, const_iterator> equal_range(const Key &key) const noexcept
    {
        if (!d)
            return qMakePair(end(), end());

        auto it = d->find(key);
        if (it.isUnused())
            return qMakePair(end(), end());
        auto end = it;
        ++end;
        return qMakePair(const_iterator(it), const_iterator(end));
    }

private:
    void detach_helper()
    {
        if (!d) {
            d = new Data;
            return;
        }
        Data *dd = new Data(*d);
        if (!d->ref.deref())
            delete d;
        d = dd;
    }

    template<typename... Args>
    iterator emplace_helper(Key &&key, Args &&...args)
    {
        auto result = d->findOrInsert(key);
        if (!result.initialized)
            Node::createInPlace(result.it.node(), std::move(key), std::forward<Args>(args)...);
        else
            result.it.node()->insertMulti(std::forward<Args>(args)...);
        ++m_size;
        return iterator(result.it);
    }

    template<typename... Args>
    iterator emplaceReplace_helper(Key &&key, Args &&...args)
    {
        auto result = d->findOrInsert(key);
        if (!result.initialized) {
            Node::createInPlace(result.it.node(), std::move(key), std::forward<Args>(args)...);
            ++m_size;
        } else {
            result.it.node()->emplaceValue(std::forward<Args>(args)...);
        }
        return iterator(result.it);
    }
};

Q_DECLARE_ASSOCIATIVE_FORWARD_ITERATOR(Hash)
Q_DECLARE_MUTABLE_ASSOCIATIVE_FORWARD_ITERATOR(Hash)
Q_DECLARE_ASSOCIATIVE_FORWARD_ITERATOR(MultiHash)
Q_DECLARE_MUTABLE_ASSOCIATIVE_FORWARD_ITERATOR(MultiHash)

template <class Key, class T>
size_t qHash(const QHash<Key, T> &key, size_t seed = 0)
    noexcept(noexcept(qHash(std::declval<Key&>())) && noexcept(qHash(std::declval<T&>())))
{
    size_t hash = 0;
    for (auto it = key.begin(), end = key.end(); it != end; ++it) {
        QtPrivate::QHashCombine combine;
        size_t h = combine(seed, it.key());
        // use + to keep the result independent of the ordering of the keys
        hash += combine(h, it.value());
    }
    return hash;
}

template <class Key, class T>
inline size_t qHash(const QMultiHash<Key, T> &key, size_t seed = 0)
    noexcept(noexcept(qHash(std::declval<Key&>())) && noexcept(qHash(std::declval<T&>())))
{
    size_t hash = 0;
    for (auto it = key.begin(), end = key.end(); it != end; ++it) {
        QtPrivate::QHashCombine combine;
        size_t h = combine(seed, it.key());
        // use + to keep the result independent of the ordering of the keys
        hash += combine(h, it.value());
    }
    return hash;
}

template <typename Key, typename T, typename Predicate>
qsizetype erase_if(QHash<Key, T> &hash, Predicate pred)
{
    return QtPrivate::associative_erase_if(hash, pred);
}

template <typename Key, typename T, typename Predicate>
qsizetype erase_if(QMultiHash<Key, T> &hash, Predicate pred)
{
    return QtPrivate::associative_erase_if(hash, pred);
}

QT_END_NAMESPACE

#endif // QHASH_H
