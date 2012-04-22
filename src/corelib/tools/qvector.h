/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QVECTOR_H
#define QVECTOR_H

#include <QtCore/qalgorithms.h>
#include <QtCore/qiterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qrefcount.h>

#include <iterator>
#include <vector>
#include <stdlib.h>
#include <string.h>
#ifdef Q_COMPILER_INITIALIZER_LISTS
#include <initializer_list>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


struct Q_CORE_EXPORT QVectorData
{
    QtPrivate::RefCount ref;
    int size;
    uint alloc : 31;
    uint capacityReserved : 1;

    qptrdiff offset;

    void* data() { return reinterpret_cast<char *>(this) + this->offset; }

    static const QVectorData shared_null;
    static QVectorData *allocate(int size, int alignment);
    static QVectorData *reallocate(QVectorData *old, int newsize, int oldsize, int alignment);
    static void free(QVectorData *data, int alignment);
    static int grow(int sizeOfHeader, int size, int sizeOfT);
};

template <typename T>
struct QVectorTypedData : QVectorData
{
    T* begin() { return reinterpret_cast<T *>(this->data()); }
    T* end() { return begin() + this->size; }

    static QVectorTypedData *sharedNull() { return static_cast<QVectorTypedData *>(const_cast<QVectorData *>(&QVectorData::shared_null)); }
};

class QRegion;

template <typename T>
class QVector
{
    typedef QVectorTypedData<T> Data;
    Data *d;

public:
    inline QVector() : d(Data::sharedNull()) { }
    explicit QVector(int size);
    QVector(int size, const T &t);
    inline QVector(const QVector<T> &v)
    {
        if (v.d->ref.ref()) {
            d = v.d;
        } else {
            d = Data::sharedNull();
            realloc(0, int(v.d->alloc));
            qCopy(v.d->begin(), v.d->end(), d->begin());
            d->size = v.d->size;
            d->capacityReserved = v.d->capacityReserved;
        }
    }

    inline ~QVector() { if (!d->ref.deref()) free(d); }
    QVector<T> &operator=(const QVector<T> &v);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QVector(QVector<T> &&other) : d(other.d) { other.d = Data::sharedNull(); }
    inline QVector<T> operator=(QVector<T> &&other)
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QVector<T> &other) { qSwap(d, other.d); }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QVector(std::initializer_list<T> args);
#endif
    bool operator==(const QVector<T> &v) const;
    inline bool operator!=(const QVector<T> &v) const { return !(*this == v); }

    inline int size() const { return d->size; }

    inline bool isEmpty() const { return d->size == 0; }

    void resize(int size);

    inline int capacity() const { return int(d->alloc); }
    void reserve(int size);
    inline void squeeze() { realloc(d->size, d->size); d->capacityReserved = 0; }

    inline void detach() { if (!isDetached()) detach_helper(); }
    inline bool isDetached() const { return !d->ref.isShared(); }
    inline void setSharable(bool sharable)
    {
        if (sharable == d->ref.isSharable())
            return;
        if (!sharable)
            detach();
        if (d != Data::sharedNull())
            d->ref.setSharable(sharable);
    }

    inline bool isSharedWith(const QVector<T> &other) const { return d == other.d; }

    inline T *data() { detach(); return d->begin(); }
    inline const T *data() const { return d->begin(); }
    inline const T *constData() const { return d->begin(); }
    void clear();

    const T &at(int i) const;
    T &operator[](int i);
    const T &operator[](int i) const;
    void append(const T &t);
    void prepend(const T &t);
    void insert(int i, const T &t);
    void insert(int i, int n, const T &t);
    void replace(int i, const T &t);
    void remove(int i);
    void remove(int i, int n);

    QVector<T> &fill(const T &t, int size = -1);

    int indexOf(const T &t, int from = 0) const;
    int lastIndexOf(const T &t, int from = -1) const;
    bool contains(const T &t) const;
    int count(const T &t) const;

#ifdef QT_STRICT_ITERATORS
    class iterator {
    public:
        T *i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        inline iterator() : i(0) {}
        inline iterator(T *n) : i(n) {}
        inline iterator(const iterator &o): i(o.i){}
        inline T &operator*() const { return *i; }
        inline T *operator->() const { return i; }
        inline T &operator[](int j) const { return *(i + j); }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }
        inline bool operator<(const iterator& other) const { return i < other.i; }
        inline bool operator<=(const iterator& other) const { return i <= other.i; }
        inline bool operator>(const iterator& other) const { return i > other.i; }
        inline bool operator>=(const iterator& other) const { return i >= other.i; }
        inline iterator &operator++() { ++i; return *this; }
        inline iterator operator++(int) { T *n = i; ++i; return n; }
        inline iterator &operator--() { i--; return *this; }
        inline iterator operator--(int) { T *n = i; i--; return n; }
        inline iterator &operator+=(int j) { i+=j; return *this; }
        inline iterator &operator-=(int j) { i-=j; return *this; }
        inline iterator operator+(int j) const { return iterator(i+j); }
        inline iterator operator-(int j) const { return iterator(i-j); }
        inline int operator-(iterator j) const { return i - j.i; }
    };
    friend class iterator;

    class const_iterator {
    public:
        T *i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline const_iterator() : i(0) {}
        inline const_iterator(T *n) : i(n) {}
        inline const_iterator(const const_iterator &o): i(o.i) {}
        inline explicit const_iterator(const iterator &o): i(o.i) {}
        inline const T &operator*() const { return *i; }
        inline const T *operator->() const { return i; }
        inline const T &operator[](int j) const { return *(i + j); }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }
        inline bool operator<(const const_iterator& other) const { return i < other.i; }
        inline bool operator<=(const const_iterator& other) const { return i <= other.i; }
        inline bool operator>(const const_iterator& other) const { return i > other.i; }
        inline bool operator>=(const const_iterator& other) const { return i >= other.i; }
        inline const_iterator &operator++() { ++i; return *this; }
        inline const_iterator operator++(int) { T *n = i; ++i; return n; }
        inline const_iterator &operator--() { i--; return *this; }
        inline const_iterator operator--(int) { T *n = i; i--; return n; }
        inline const_iterator &operator+=(int j) { i+=j; return *this; }
        inline const_iterator &operator-=(int j) { i-=j; return *this; }
        inline const_iterator operator+(int j) const { return const_iterator(i+j); }
        inline const_iterator operator-(int j) const { return const_iterator(i-j); }
        inline int operator-(const_iterator j) const { return i - j.i; }
    };
    friend class const_iterator;
#else
    // STL-style
    typedef T* iterator;
    typedef const T* const_iterator;
#endif
    inline iterator begin() { detach(); return d->begin(); }
    inline const_iterator begin() const { return d->begin(); }
    inline const_iterator cbegin() const { return d->begin(); }
    inline const_iterator constBegin() const { return d->begin(); }
    inline iterator end() { detach(); return d->end(); }
    inline const_iterator end() const { return d->end(); }
    inline const_iterator cend() const { return d->end(); }
    inline const_iterator constEnd() const { return d->end(); }
    iterator insert(iterator before, int n, const T &x);
    inline iterator insert(iterator before, const T &x) { return insert(before, 1, x); }
    iterator erase(iterator begin, iterator end);
    inline iterator erase(iterator pos) { return erase(pos, pos+1); }

    // more Qt
    inline int count() const { return d->size; }
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline T& last() { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &last() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline bool startsWith(const T &t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(const T &t) const { return !isEmpty() && last() == t; }
    QVector<T> mid(int pos, int length = -1) const;

    T value(int i) const;
    T value(int i, const T &defaultValue) const;

    // STL compatibility
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef qptrdiff difference_type;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef int size_type;
    inline void push_back(const T &t) { append(t); }
    inline void push_front(const T &t) { prepend(t); }
    void pop_back() { Q_ASSERT(!isEmpty()); erase(end()-1); }
    void pop_front() { Q_ASSERT(!isEmpty()); erase(begin()); }
    inline bool empty() const
    { return d->size == 0; }
    inline T& front() { return first(); }
    inline const_reference front() const { return first(); }
    inline reference back() { return last(); }
    inline const_reference back() const { return last(); }

    // comfort
    QVector<T> &operator+=(const QVector<T> &l);
    inline QVector<T> operator+(const QVector<T> &l) const
    { QVector n = *this; n += l; return n; }
    inline QVector<T> &operator+=(const T &t)
    { append(t); return *this; }
    inline QVector<T> &operator<< (const T &t)
    { append(t); return *this; }
    inline QVector<T> &operator<<(const QVector<T> &l)
    { *this += l; return *this; }

    QList<T> toList() const;

    static QVector<T> fromList(const QList<T> &list);

    static inline QVector<T> fromStdVector(const std::vector<T> &vector)
    { QVector<T> tmp; tmp.reserve(int(vector.size())); qCopy(vector.begin(), vector.end(), std::back_inserter(tmp)); return tmp; }
    inline std::vector<T> toStdVector() const
    { std::vector<T> tmp; tmp.reserve(size()); qCopy(constBegin(), constEnd(), std::back_inserter(tmp)); return tmp; }
private:
    friend class QRegion; // Optimization for QRegion::rects()

    void detach_helper();
    Data *malloc(int alloc);
    void realloc(int size, int alloc);
    void free(Data *d);

    class AlignmentDummy { QVectorData header; T array[1]; };

    static Q_DECL_CONSTEXPR int offsetOfTypedData()
    {
        // (non-POD)-safe offsetof(AlignmentDummy, array)
        return (sizeof(QVectorData) + (alignOfTypedData() - 1)) & ~(alignOfTypedData() - 1);
    }
    static Q_DECL_CONSTEXPR int alignOfTypedData()
    {
        return Q_ALIGNOF(AlignmentDummy);
    }
};

template <typename T>
void QVector<T>::detach_helper()
{ realloc(d->size, int(d->alloc)); }
template <typename T>
void QVector<T>::reserve(int asize)
{ if (asize > int(d->alloc)) realloc(d->size, asize); if (isDetached()) d->capacityReserved = 1; }
template <typename T>
void QVector<T>::resize(int asize)
{ realloc(asize, (asize > int(d->alloc) || (!d->capacityReserved && asize < d->size && asize < int(d->alloc >> 1))) ?
          QVectorData::grow(offsetOfTypedData(), asize, sizeof(T))
          : int(d->alloc)); }
template <typename T>
inline void QVector<T>::clear()
{ *this = QVector<T>(); }
template <typename T>
inline const T &QVector<T>::at(int i) const
{ Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::at", "index out of range");
  return d->begin()[i]; }
template <typename T>
inline const T &QVector<T>::operator[](int i) const
{ Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::operator[]", "index out of range");
  return d->begin()[i]; }
template <typename T>
inline T &QVector<T>::operator[](int i)
{ Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::operator[]", "index out of range");
  return data()[i]; }
template <typename T>
inline void QVector<T>::insert(int i, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= d->size, "QVector<T>::insert", "index out of range");
  insert(begin() + i, 1, t); }
template <typename T>
inline void QVector<T>::insert(int i, int n, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= d->size, "QVector<T>::insert", "index out of range");
  insert(begin() + i, n, t); }
template <typename T>
inline void QVector<T>::remove(int i, int n)
{ Q_ASSERT_X(i >= 0 && n >= 0 && i + n <= d->size, "QVector<T>::remove", "index out of range");
  erase(begin() + i, begin() + i + n); }
template <typename T>
inline void QVector<T>::remove(int i)
{ Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::remove", "index out of range");
  erase(begin() + i, begin() + i + 1); }
template <typename T>
inline void QVector<T>::prepend(const T &t)
{ insert(begin(), 1, t); }

template <typename T>
inline void QVector<T>::replace(int i, const T &t)
{
    Q_ASSERT_X(i >= 0 && i < d->size, "QVector<T>::replace", "index out of range");
    const T copy(t);
    data()[i] = copy;
}

template <typename T>
QVector<T> &QVector<T>::operator=(const QVector<T> &v)
{
    if (v.d != d) {
        QVector<T> tmp(v);
        tmp.swap(*this);
    }
    return *this;
}

template <typename T>
inline typename QVector<T>::Data *QVector<T>::malloc(int aalloc)
{
    QVectorData *vectordata = QVectorData::allocate(offsetOfTypedData() + aalloc * sizeof(T), alignOfTypedData());
    Q_CHECK_PTR(vectordata);
    return static_cast<Data *>(vectordata);
}

template <typename T>
QVector<T>::QVector(int asize)
{
    d = malloc(asize);
    d->ref.initializeOwned();
    d->size = asize;
    d->alloc = uint(d->size);
    d->capacityReserved = false;
    d->offset = offsetOfTypedData();
    if (QTypeInfo<T>::isComplex) {
        T* b = d->begin();
        T* i = d->end();
        while (i != b)
            new (--i) T;
    } else {
        memset(d->begin(), 0, asize * sizeof(T));
    }
}

template <typename T>
QVector<T>::QVector(int asize, const T &t)
{
    d = malloc(asize);
    d->ref.initializeOwned();
    d->size = asize;
    d->alloc = uint(d->size);
    d->capacityReserved = false;
    d->offset = offsetOfTypedData();
    T* i = d->end();
    while (i != d->begin())
        new (--i) T(t);
}

#ifdef Q_COMPILER_INITIALIZER_LISTS
template <typename T>
QVector<T>::QVector(std::initializer_list<T> args)
{
    d = malloc(int(args.size()));
    d->ref.initializeOwned();
    d->size = int(args.size());
    d->alloc = uint(d->size);
    d->capacityReserved = false;
    d->offset = offsetOfTypedData();
    if (QTypeInfo<T>::isComplex) {
        T* b = d->begin();
        T* i = d->end();
        const T* s = args.end();
        while (i != b)
            new(--i) T(*--s);
    } else {
        // std::initializer_list<T>::iterator is guaranteed to be
        // const T* ([support.initlist]/1), so can be memcpy'ed away from:
        ::memcpy(d->begin(), args.begin(), args.size() * sizeof(T));
    }
}
#endif

template <typename T>
void QVector<T>::free(Data *x)
{
    if (QTypeInfo<T>::isComplex) {
        T* b = x->begin();
        T* i = b + x->size;
        while (i-- != b)
             i->~T();
    }
    Data::free(x, alignOfTypedData());
}

template <typename T>
void QVector<T>::realloc(int asize, int aalloc)
{
    Q_ASSERT(asize <= aalloc);
    T *pOld;
    T *pNew;
    Data *x = d;

    if (QTypeInfo<T>::isComplex && asize < d->size && isDetached()) {
        // call the destructor on all objects that need to be
        // destroyed when shrinking
        pOld = d->begin() + d->size;
        pNew = d->begin() + asize;
        while (asize < d->size) {
            (--pOld)->~T();
            d->size--;
        }
    }

    if (aalloc != int(d->alloc) || !isDetached()) {
        // (re)allocate memory
        if (QTypeInfo<T>::isStatic) {
            x = malloc(aalloc);
            Q_CHECK_PTR(x);
            x->size = 0;
        } else if (!isDetached()) {
            x = malloc(aalloc);
            Q_CHECK_PTR(x);
            if (QTypeInfo<T>::isComplex) {
                x->size = 0;
            } else {
                ::memcpy(x, d, offsetOfTypedData() + qMin(uint(aalloc), d->alloc) * sizeof(T));
                x->size = d->size;
            }
        } else {
            QT_TRY {
                QVectorData *mem = QVectorData::reallocate(d, offsetOfTypedData() + aalloc * sizeof(T),
                                                           offsetOfTypedData() + d->alloc * sizeof(T), alignOfTypedData());
                Q_CHECK_PTR(mem);
                x = d = static_cast<Data *>(mem);
                x->size = d->size;
            } QT_CATCH (const std::bad_alloc &) {
                if (aalloc > int(d->alloc)) // ignore the error in case we are just shrinking.
                    QT_RETHROW;
            }
        }
        x->ref.initializeOwned();
        x->alloc = uint(aalloc);
        x->capacityReserved = d->capacityReserved;
        x->offset = offsetOfTypedData();
    }

    if (QTypeInfo<T>::isComplex) {
        QT_TRY {
            pOld = d->begin() + x->size;
            pNew = x->begin() + x->size;
            // copy objects from the old array into the new array
            const int toMove = qMin(asize, d->size);
            while (x->size < toMove) {
                new (pNew++) T(*pOld++);
                x->size++;
            }
            // construct all new objects when growing
            while (x->size < asize) {
                new (pNew++) T;
                x->size++;
            }
        } QT_CATCH (...) {
            free(x);
            QT_RETHROW;
        }

    } else if (asize > x->size) {
        // initialize newly allocated memory to 0
        memset(x->end(), 0, (asize - x->size) * sizeof(T));
    }
    x->size = asize;

    if (d != x) {
        if (!d->ref.deref())
            free(d);
        d = x;
    }
}

template<typename T>
Q_OUTOFLINE_TEMPLATE T QVector<T>::value(int i) const
{
    if (i < 0 || i >= d->size) {
        return T();
    }
    return d->begin()[i];
}
template<typename T>
Q_OUTOFLINE_TEMPLATE T QVector<T>::value(int i, const T &defaultValue) const
{
    return ((i < 0 || i >= d->size) ? defaultValue : d->begin()[i]);
}

template <typename T>
void QVector<T>::append(const T &t)
{
    if (!isDetached() || d->size + 1 > int(d->alloc)) {
        const T copy(t);
        realloc(d->size, (d->size + 1 > int(d->alloc)) ?
                    QVectorData::grow(offsetOfTypedData(), d->size + 1, sizeof(T))
                    : int(d->alloc));
        if (QTypeInfo<T>::isComplex)
            new (d->end()) T(copy);
        else
            *d->end() = copy;
    } else {
        if (QTypeInfo<T>::isComplex)
            new (d->end()) T(t);
        else
            *d->end() = t;
    }
    ++d->size;
}

template <typename T>
typename QVector<T>::iterator QVector<T>::insert(iterator before, size_type n, const T &t)
{
    int offset = int(before - d->begin());
    if (n != 0) {
        const T copy(t);
        if (!isDetached() || d->size + n > int(d->alloc))
            realloc(d->size, QVectorData::grow(offsetOfTypedData(), d->size + n, sizeof(T)));
        if (QTypeInfo<T>::isStatic) {
            T *b = d->end();
            T *i = d->end() + n;
            while (i != b)
                new (--i) T;
            i = d->end();
            T *j = i + n;
            b = d->begin() + offset;
            while (i != b)
                *--j = *--i;
            i = b+n;
            while (i != b)
                *--i = copy;
        } else {
            T *b = d->begin() + offset;
            T *i = b + n;
            memmove(i, b, (d->size - offset) * sizeof(T));
            while (i != b)
                new (--i) T(copy);
        }
        d->size += n;
    }
    return d->begin() + offset;
}

template <typename T>
typename QVector<T>::iterator QVector<T>::erase(iterator abegin, iterator aend)
{
    int f = int(abegin - d->begin());
    int l = int(aend - d->begin());
    int n = l - f;
    detach();
    if (QTypeInfo<T>::isComplex) {
        qCopy(d->begin()+l, d->end(), d->begin()+f);
        T *i = d->end();
        T* b = d->end()-n;
        while (i != b) {
            --i;
            i->~T();
        }
    } else {
        memmove(d->begin() + f, d->begin() + l, (d->size-l)*sizeof(T));
    }
    d->size -= n;
    return d->begin() + f;
}

template <typename T>
bool QVector<T>::operator==(const QVector<T> &v) const
{
    if (d->size != v.d->size)
        return false;
    if (d == v.d)
        return true;
    T* b = d->begin();
    T* i = b + d->size;
    T* j = v.d->end();
    while (i != b)
        if (!(*--i == *--j))
            return false;
    return true;
}

template <typename T>
QVector<T> &QVector<T>::fill(const T &from, int asize)
{
    const T copy(from);
    resize(asize < 0 ? d->size : asize);
    if (d->size) {
        T *i = d->end();
        T *b = d->begin();
        while (i != b)
            *--i = copy;
    }
    return *this;
}

template <typename T>
QVector<T> &QVector<T>::operator+=(const QVector &l)
{
    int newSize = d->size + l.d->size;
    realloc(d->size, newSize);

    T *w = d->begin() + newSize;
    T *i = l.d->end();
    T *b = l.d->begin();
    while (i != b) {
        if (QTypeInfo<T>::isComplex)
            new (--w) T(*--i);
        else
            *--w = *--i;
    }
    d->size = newSize;
    return *this;
}

template <typename T>
int QVector<T>::indexOf(const T &t, int from) const
{
    if (from < 0)
        from = qMax(from + d->size, 0);
    if (from < d->size) {
        T* n = d->begin() + from - 1;
        T* e = d->end();
        while (++n != e)
            if (*n == t)
                return n - d->begin();
    }
    return -1;
}

template <typename T>
int QVector<T>::lastIndexOf(const T &t, int from) const
{
    if (from < 0)
        from += d->size;
    else if (from >= d->size)
        from = d->size-1;
    if (from >= 0) {
        T* b = d->begin();
        T* n = d->begin() + from + 1;
        while (n != b) {
            if (*--n == t)
                return n - b;
        }
    }
    return -1;
}

template <typename T>
bool QVector<T>::contains(const T &t) const
{
    T* b = d->begin();
    T* i = d->end();
    while (i != b)
        if (*--i == t)
            return true;
    return false;
}

template <typename T>
int QVector<T>::count(const T &t) const
{
    int c = 0;
    T* b = d->begin();
    T* i = d->end();
    while (i != b)
        if (*--i == t)
            ++c;
    return c;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QVector<T> QVector<T>::mid(int pos, int length) const
{
    if (length < 0)
        length = size() - pos;
    if (pos == 0 && length == size())
        return *this;
    if (pos + length > size())
        length = size() - pos;
    QVector<T> copy;
    copy.reserve(length);
    for (int i = pos; i < pos + length; ++i)
        copy += at(i);
    return copy;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QList<T> QVector<T>::toList() const
{
    QList<T> result;
    result.reserve(size());
    for (int i = 0; i < size(); ++i)
        result.append(at(i));
    return result;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QVector<T> QList<T>::toVector() const
{
    QVector<T> result(size());
    for (int i = 0; i < size(); ++i)
        result[i] = at(i);
    return result;
}

template <typename T>
QVector<T> QVector<T>::fromList(const QList<T> &list)
{
    return list.toVector();
}

template <typename T>
QList<T> QList<T>::fromVector(const QVector<T> &vector)
{
    return vector.toList();
}

Q_DECLARE_SEQUENTIAL_ITERATOR(Vector)
Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(Vector)

/*
   ### Qt 5:
   ### This needs to be removed for next releases of Qt. It is a workaround for vc++ because
   ### Qt exports QPolygon and QPolygonF that inherit QVector<QPoint> and
   ### QVector<QPointF> respectively.
*/

#ifdef Q_CC_MSVC
QT_BEGIN_INCLUDE_NAMESPACE
#include <QtCore/QPointF>
#include <QtCore/QPoint>
QT_END_INCLUDE_NAMESPACE

#if defined(QT_BUILD_CORE_LIB)
#define Q_TEMPLATE_EXTERN
#else
#define Q_TEMPLATE_EXTERN extern
#endif
Q_TEMPLATE_EXTERN template class Q_CORE_EXPORT QVector<QPointF>;
Q_TEMPLATE_EXTERN template class Q_CORE_EXPORT QVector<QPoint>;
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QVECTOR_H
