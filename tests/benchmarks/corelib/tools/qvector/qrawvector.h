/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRAWVECTOR_H
#define QRAWVECTOR_H

#include <QtCore/qiterator.h>
#include <QtCore/qdebug.h>
#include <QtCore/qatomic.h>
#include <QtCore/qalgorithms.h>
#include <QtCore/qlist.h>
#include <QtCore/private/qtools_p.h>

#include <iterator>
#include <vector>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

QT_BEGIN_NAMESPACE


struct QVectorData
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

template <typename T>
class QRawVector
{
    typedef QVectorTypedData<T> Data;

    T *m_begin;
    int m_size;
    int m_alloc;

public:
    static Data *toBase(T *begin)
    { return (Data*)((char*)begin - offsetOfTypedData()); }
    static T *fromBase(void *d)
    { return (T*)((char*)d + offsetOfTypedData()); }

    inline QRawVector()
    { m_begin = fromBase(0); m_alloc = m_size = 0; realloc(m_size, m_alloc, true); }
    explicit QRawVector(int size);
    QRawVector(int size, const T &t);
    inline QRawVector(const QRawVector<T> &v)
    { m_begin = v.m_begin; m_alloc = v.m_alloc; m_size = v.m_size; realloc(m_size, m_alloc, true); }
    inline ~QRawVector() { free(m_begin, m_size); }
    QRawVector<T> &operator=(const QRawVector<T> &v);
    bool operator==(const QRawVector<T> &v) const;
    inline bool operator!=(const QRawVector<T> &v) const { return !(*this == v); }

    inline int size() const { return m_size; }

    inline bool isEmpty() const { return m_size == 0; }

    void resize(int size);

    inline int capacity() const { return m_alloc; }
    void reserve(int size);
    inline void squeeze() { realloc(m_size, m_size, false); }

    inline T *data() { return m_begin; }
    inline const T *data() const { return m_begin; }
    inline const T *constData() const { return m_begin; }
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

    QRawVector<T> &fill(const T &t, int size = -1);

    int indexOf(const T &t, int from = 0) const;
    int lastIndexOf(const T &t, int from = -1) const;
    bool contains(const T &t) const;
    int count(const T &t) const;

#ifdef QT_STRICT_ITERATORS
    class iterator {
    public:
        T *i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef ptrdiff_t difference_type;
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
        typedef ptrdiff_t difference_type;
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
        inline const_iterator &operator-=(int j) { i+=j; return *this; }
        inline const_iterator operator+(int j) const { return const_iterator(i+j); }
        inline const_iterator operator-(int j) const { return const_iterator(i-j); }
        inline int operator-(const_iterator j) const { return i - j.i; }
    };
    friend class const_iterator;
#else
    // STL-style
    typedef T *iterator;
    typedef const T *const_iterator;
#endif
    inline iterator begin() { return m_begin; }
    inline const_iterator begin() const { return m_begin; }
    inline const_iterator constBegin() const { return m_begin; }
    inline iterator end() { return m_begin + m_size; }
    inline const_iterator end() const { return m_begin + m_size; }
    inline const_iterator constEnd() const { return m_begin + m_size; }
    iterator insert(iterator before, int n, const T &x);
    inline iterator insert(iterator before, const T &x) { return insert(before, 1, x); }
    iterator erase(iterator begin, iterator end);
    inline iterator erase(iterator pos) { return erase(pos, pos+1); }

    // more Qt
    inline int count() const { return m_size; }
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    inline T& last() { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline const T &last() const { Q_ASSERT(!isEmpty()); return *(end()-1); }
    inline bool startsWith(const T &t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(const T &t) const { return !isEmpty() && last() == t; }
    QRawVector<T> mid(int pos, int length = -1) const;

    T value(int i) const;
    T value(int i, const T &defaultValue) const;

    // STL compatibility
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef ptrdiff_t difference_type;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    typedef int size_type;
    inline void push_back(const T &t) { append(t); }
    inline void push_front(const T &t) { prepend(t); }
    void pop_back() { Q_ASSERT(!isEmpty()); erase(end()-1); }
    void pop_front() { Q_ASSERT(!isEmpty()); erase(begin()); }
    inline bool empty() const { return m_size == 0; }
    inline T &front() { return first(); }
    inline const_reference front() const { return first(); }
    inline reference back() { return last(); }
    inline const_reference back() const { return last(); }

    // comfort
    QRawVector<T> &operator+=(const QRawVector<T> &l);
    inline QRawVector<T> operator+(const QRawVector<T> &l) const
    { QRawVector n = *this; n += l; return n; }
    inline QRawVector<T> &operator+=(const T &t)
    { append(t); return *this; }
    inline QRawVector<T> &operator<< (const T &t)
    { append(t); return *this; }
    inline QRawVector<T> &operator<<(const QRawVector<T> &l)
    { *this += l; return *this; }

    QList<T> toList() const;

    //static QRawVector<T> fromList(const QList<T> &list);

    static inline QRawVector<T> fromStdVector(const std::vector<T> &vector)
    { QRawVector<T> tmp; qCopy(vector.begin(), vector.end(), std::back_inserter(tmp)); return tmp; }
    inline std::vector<T> toStdVector() const
    { std::vector<T> tmp; qCopy(constBegin(), constEnd(), std::back_inserter(tmp)); return tmp; }

private:
    T *allocate(int alloc);
    void realloc(int size, int alloc, bool ref);
    void free(T *begin, int size);

    class AlignmentDummy { QVectorData header; T array[1]; };

    static Q_DECL_CONSTEXPR int offsetOfTypedData()
    {
        // (non-POD)-safe offsetof(AlignmentDummy, array)
        return (sizeof(QVectorData) + (alignOfTypedData() - 1)) & ~(alignOfTypedData() - 1);
    }
    static Q_DECL_CONSTEXPR int alignOfTypedData()
    {
#ifdef Q_ALIGNOF
        return Q_ALIGNOF(AlignmentDummy);
#else
        return sizeof(void *);
#endif
    }

public:
    QVector<T> mutateToVector()
    {
        Data *d = toBase(m_begin);
        d->ref.initializeOwned();
        d->alloc = m_alloc;
        d->size = m_size;
        d->capacityReserved = 0;
        d->offset = offsetOfTypedData();

        QVector<T> v;
        *reinterpret_cast<QVectorData **>(&v) = d;
        m_begin = fromBase(0);
        m_size = m_alloc = 0;
        return v;
    }
};


template <typename T>
void QRawVector<T>::reserve(int asize)
{ if (asize > m_alloc) realloc(m_size, asize, false); }
template <typename T>
void QRawVector<T>::resize(int asize)
{ realloc(asize, (asize > m_alloc || (asize < m_size && asize < (m_alloc >> 1)))
    ? QVectorData::grow(offsetOfTypedData(), asize, sizeof(T))
    : m_alloc, false); }
template <typename T>
inline void QRawVector<T>::clear()
{ *this = QRawVector<T>(); }
template <typename T>
inline const T &QRawVector<T>::at(int i) const
{ Q_ASSERT_X(i >= 0 && i < m_size, "QRawVector<T>::at", "index out of range");
  return m_begin[i]; }
template <typename T>
inline const T &QRawVector<T>::operator[](int i) const
{ Q_ASSERT_X(i >= 0 && i < m_size, "QRawVector<T>::operator[]", "index out of range");
  return m_begin[i]; }
template <typename T>
inline T &QRawVector<T>::operator[](int i)
{ Q_ASSERT_X(i >= 0 && i < m_size, "QRawVector<T>::operator[]", "index out of range");
  return data()[i]; }
template <typename T>
inline void QRawVector<T>::insert(int i, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= m_size, "QRawVector<T>::insert", "index out of range");
  insert(begin() + i, 1, t); }
template <typename T>
inline void QRawVector<T>::insert(int i, int n, const T &t)
{ Q_ASSERT_X(i >= 0 && i <= m_size, "QRawVector<T>::insert", "index out of range");
  insert(begin() + i, n, t); }
template <typename T>
inline void QRawVector<T>::remove(int i, int n)
{ Q_ASSERT_X(i >= 0 && n >= 0 && i + n <= m_size, "QRawVector<T>::remove", "index out of range");
  erase(begin() + i, begin() + i + n); }
template <typename T>
inline void QRawVector<T>::remove(int i)
{ Q_ASSERT_X(i >= 0 && i < m_size, "QRawVector<T>::remove", "index out of range");
  erase(begin() + i, begin() + i + 1); }
template <typename T>
inline void QRawVector<T>::prepend(const T &t)
{ insert(begin(), 1, t); }

template <typename T>
inline void QRawVector<T>::replace(int i, const T &t)
{
    Q_ASSERT_X(i >= 0 && i < m_size, "QRawVector<T>::replace", "index out of range");
    const T copy(t);
    data()[i] = copy;
}

template <typename T>
QRawVector<T> &QRawVector<T>::operator=(const QRawVector<T> &v)
{
    if (this != &v) {
        free(m_begin, m_size);
        m_alloc = v.m_alloc;
        m_size = v.m_size;
        m_begin = v.m_begin;
        realloc(m_size, m_alloc, true);
    }
    return *this;
}

template <typename T>
inline T *QRawVector<T>::allocate(int aalloc)
{
    QVectorData *d = QVectorData::allocate(offsetOfTypedData() + aalloc * sizeof(T), alignOfTypedData());
    Q_CHECK_PTR(d);
    return fromBase(d);
}

template <typename T>
QRawVector<T>::QRawVector(int asize)
{
    m_size = m_alloc = asize;
    m_begin = allocate(asize);
    if (QTypeInfo<T>::isComplex) {
        T *b = m_begin;
        T *i = m_begin + m_size;
        while (i != b)
            new (--i) T;
    } else {
        memset(m_begin, 0, asize * sizeof(T));
    }
}

template <typename T>
QRawVector<T>::QRawVector(int asize, const T &t)
{
    m_size = m_alloc = asize;
    m_begin = allocate(asize);
    T *i = m_begin + m_size;
    while (i != m_begin)
        new (--i) T(t);
}

template <typename T>
void QRawVector<T>::free(T *begin, int size)
{
    if (QTypeInfo<T>::isComplex) {
        T *i = begin + size;
        while (i-- != begin)
             i->~T();
    }
    Data *x = toBase(begin);
    x->free(x, alignOfTypedData());
}

template <typename T>
void QRawVector<T>::realloc(int asize, int aalloc, bool ref)
{
    if (QTypeInfo<T>::isComplex && asize < m_size && !ref) {
        // call the destructor on all objects that need to be
        // destroyed when shrinking
        T *pOld = m_begin + m_size;
        while (asize < m_size) {
            (--pOld)->~T();
            --m_size;
        }
    }

    int xalloc = m_alloc;
    int xsize = m_size;
    bool changed = false;
    T *xbegin = m_begin;
    if (aalloc != xalloc || ref) {
        // (re)allocate memory
        if (QTypeInfo<T>::isStatic) {
            xbegin = allocate(aalloc);
            xsize = 0;
            changed = true;
        } else if (ref) {
            xbegin = allocate(aalloc);
            if (QTypeInfo<T>::isComplex) {
                xsize = 0;
            } else {
                ::memcpy(xbegin, m_begin, qMin(aalloc, xalloc) * sizeof(T));
                xsize = m_size;
            }
            changed = true;
        } else {
            QT_TRY {
                QVectorData *mem = QVectorData::reallocate(toBase(m_begin),
                        offsetOfTypedData() + aalloc * sizeof(T),
                        offsetOfTypedData() + xalloc * sizeof(T), alignOfTypedData());
                Q_CHECK_PTR(mem);
                xbegin = fromBase(mem);
                xsize = m_size;
            } QT_CATCH (const std::bad_alloc &) {
                if (aalloc > xalloc) // ignore the error in case we are just shrinking.
                    QT_RETHROW;
            }
        }
        xalloc = aalloc;
    }

    if (QTypeInfo<T>::isComplex) {
        QT_TRY {
            T *pOld = m_begin + xsize;
            T *pNew = xbegin + xsize;
            // copy objects from the old array into the new array
            while (xsize < qMin(asize, m_size)) {
                new (pNew++) T(*pOld++);
                ++xsize;
            }
            // construct all new objects when growing
            while (xsize < asize) {
                new (pNew++) T;
                ++xsize;
            }
        } QT_CATCH (...) {
            free(xbegin, xsize);
            QT_RETHROW;
        }

    } else if (asize > xsize) {
        // initialize newly allocated memory to 0
        memset(xbegin + xsize, 0, (asize - xsize) * sizeof(T));
    }
    xsize = asize;

    if (changed) {
        if (!ref)
            free(m_begin, m_size);
    }
    m_alloc = xalloc;
    m_size = xsize;
    m_begin = xbegin;
}

template<typename T>
Q_OUTOFLINE_TEMPLATE T QRawVector<T>::value(int i) const
{
    return (i < 0 || i >= m_size) ? T() : m_begin[i];
}
template<typename T>
Q_OUTOFLINE_TEMPLATE T QRawVector<T>::value(int i, const T &defaultValue) const
{
    return (i < 0 || i >= m_size) ? defaultValue : m_begin[i];
}

template <typename T>
void QRawVector<T>::append(const T &t)
{
    if (m_size + 1 > m_alloc) {
        const T copy(t);
        realloc(m_size, QVectorData::grow(offsetOfTypedData(), m_size + 1, sizeof(T)), false);
        if (QTypeInfo<T>::isComplex)
            new (m_begin + m_size) T(copy);
        else
            m_begin[m_size] = copy;
    } else {
        if (QTypeInfo<T>::isComplex)
            new (m_begin + m_size) T(t);
        else
            m_begin[m_size] = t;
    }
    ++m_size;
}

template <typename T>
typename QRawVector<T>::iterator QRawVector<T>::insert(iterator before, size_type n, const T &t)
{
    int offset = int(before - m_begin);
    if (n != 0) {
        const T copy(t);
        if (m_size + n > m_alloc)
            realloc(m_size, QVectorData::grow(offsetOfTypedData(), m_size + n, sizeof(T)), false);
        if (QTypeInfo<T>::isStatic) {
            T *b = m_begin + m_size;
            T *i = m_begin + m_size + n;
            while (i != b)
                new (--i) T;
            i = m_begin + m_size;
            T *j = i + n;
            b = m_begin + offset;
            while (i != b)
                *--j = *--i;
            i = b+n;
            while (i != b)
                *--i = copy;
        } else {
            T *b = m_begin + offset;
            T *i = b + n;
            memmove(i, b, (m_size - offset) * sizeof(T));
            while (i != b)
                new (--i) T(copy);
        }
        m_size += n;
    }
    return m_begin + offset;
}

template <typename T>
typename QRawVector<T>::iterator QRawVector<T>::erase(iterator abegin, iterator aend)
{
    int f = int(abegin - m_begin);
    int l = int(aend - m_begin);
    int n = l - f;
    if (QTypeInfo<T>::isComplex) {
        qCopy(m_begin + l, m_begin + m_size, m_begin + f);
        T *i = m_begin + m_size;
        T *b = m_begin + m_size - n;
        while (i != b) {
            --i;
            i->~T();
        }
    } else {
        memmove(m_begin + f, m_begin + l, (m_size - l) * sizeof(T));
    }
    m_size -= n;
    return m_begin + f;
}

template <typename T>
bool QRawVector<T>::operator==(const QRawVector<T> &v) const
{
    if (m_size != v.m_size)
        return false;
    T* b = m_begin;
    T* i = b + m_size;
    T* j = v.m_begin + m_size;
    while (i != b)
        if (!(*--i == *--j))
            return false;
    return true;
}

template <typename T>
QRawVector<T> &QRawVector<T>::fill(const T &from, int asize)
{
    const T copy(from);
    resize(asize < 0 ? m_size : asize);
    if (m_size) {
        T *i = m_begin + m_size;
        T *b = m_begin;
        while (i != b)
            *--i = copy;
    }
    return *this;
}

template <typename T>
QRawVector<T> &QRawVector<T>::operator+=(const QRawVector &l)
{
    int newSize = m_size + l.m_size;
    realloc(m_size, newSize, false);

    T *w = m_begin + newSize;
    T *i = l.m_begin + l.m_size;
    T *b = l.m_begin;
    while (i != b) {
        if (QTypeInfo<T>::isComplex)
            new (--w) T(*--i);
        else
            *--w = *--i;
    }
    m_size = newSize;
    return *this;
}

template <typename T>
int QRawVector<T>::indexOf(const T &t, int from) const
{
    if (from < 0)
        from = qMax(from + m_size, 0);
    if (from < m_size) {
        T* n = m_begin + from - 1;
        T* e = m_begin + m_size;
        while (++n != e)
            if (*n == t)
                return n - m_begin;
    }
    return -1;
}

template <typename T>
int QRawVector<T>::lastIndexOf(const T &t, int from) const
{
    if (from < 0)
        from += m_size;
    else if (from >= m_size)
        from = m_size-1;
    if (from >= 0) {
        T* b = m_begin;
        T* n = m_begin + from + 1;
        while (n != b) {
            if (*--n == t)
                return n - b;
        }
    }
    return -1;
}

template <typename T>
bool QRawVector<T>::contains(const T &t) const
{
    T* b = m_begin;
    T* i = m_begin + m_size;
    while (i != b)
        if (*--i == t)
            return true;
    return false;
}

template <typename T>
int QRawVector<T>::count(const T &t) const
{
    int c = 0;
    T* b = m_begin;
    T* i = m_begin + m_size;
    while (i != b)
        if (*--i == t)
            ++c;
    return c;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QRawVector<T> QRawVector<T>::mid(int pos, int length) const
{
    if (length < 0)
        length = size() - pos;
    if (pos == 0 && length == size())
        return *this;
    QRawVector<T> copy;
    if (pos + length > size())
        length = size() - pos;
    for (int i = pos; i < pos + length; ++i)
        copy += at(i);
    return copy;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QList<T> QRawVector<T>::toList() const
{
    QList<T> result;
    for (int i = 0; i < size(); ++i)
        result.append(at(i));
    return result;
}


/*template <typename T>
Q_OUTOFLINE_TEMPLATE QRawVector<T> QList<T>::toVector() const
{
    QRawVector<T> result(size());
    for (int i = 0; i < size(); ++i)
        result[i] = at(i);
    return result;
}

template <typename T>
QRawVector<T> QRawVector<T>::fromList(const QList<T> &list)
{
    return list.toVector();
}

template <typename T>
QList<T> QList<T>::fromVector(const QRawVector<T> &vector)
{
    return vector.toList();
}
*/

Q_DECLARE_SEQUENTIAL_ITERATOR(RawVector)
Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(RawVector)

QT_END_NAMESPACE

#endif // QRAWVECTOR_H
