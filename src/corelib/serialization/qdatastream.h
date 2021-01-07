/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QDATASTREAM_H
#define QDATASTREAM_H

#include <QtCore/qscopedpointer.h>
#include <QtCore/qiodevicebase.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qnamespace.h>

#ifdef Status
#error qdatastream.h must be included before any header file that defines Status
#endif

QT_BEGIN_NAMESPACE

class qfloat16;
class QByteArray;
class QIODevice;

#if !defined(QT_NO_DATASTREAM) || defined(QT_BOOTSTRAPPED)
class QDataStreamPrivate;
namespace QtPrivate {
class StreamStateSaver;
}
class Q_CORE_EXPORT QDataStream : public QIODeviceBase
{
public:
    enum Version {
        Qt_1_0 = 1,
        Qt_2_0 = 2,
        Qt_2_1 = 3,
        Qt_3_0 = 4,
        Qt_3_1 = 5,
        Qt_3_3 = 6,
        Qt_4_0 = 7,
        Qt_4_1 = Qt_4_0,
        Qt_4_2 = 8,
        Qt_4_3 = 9,
        Qt_4_4 = 10,
        Qt_4_5 = 11,
        Qt_4_6 = 12,
        Qt_4_7 = Qt_4_6,
        Qt_4_8 = Qt_4_7,
        Qt_4_9 = Qt_4_8,
        Qt_5_0 = 13,
        Qt_5_1 = 14,
        Qt_5_2 = 15,
        Qt_5_3 = Qt_5_2,
        Qt_5_4 = 16,
        Qt_5_5 = Qt_5_4,
        Qt_5_6 = 17,
        Qt_5_7 = Qt_5_6,
        Qt_5_8 = Qt_5_7,
        Qt_5_9 = Qt_5_8,
        Qt_5_10 = Qt_5_9,
        Qt_5_11 = Qt_5_10,
        Qt_5_12 = 18,
        Qt_5_13 = 19,
        Qt_5_14 = Qt_5_13,
        Qt_5_15 = Qt_5_14,
        Qt_6_0 = 20,
        Qt_6_1 = Qt_6_0,
        Qt_DefaultCompiledVersion = Qt_6_1
#if QT_VERSION >= 0x060200
#error Add the datastream version for this Qt version and update Qt_DefaultCompiledVersion
#endif
    };

    enum ByteOrder {
        BigEndian = QSysInfo::BigEndian,
        LittleEndian = QSysInfo::LittleEndian
    };

    enum Status {
        Ok,
        ReadPastEnd,
        ReadCorruptData,
        WriteFailed
    };

    enum FloatingPointPrecision {
        SinglePrecision,
        DoublePrecision
    };

    QDataStream();
    explicit QDataStream(QIODevice *);
    QDataStream(QByteArray *, OpenMode flags);
    QDataStream(const QByteArray &);
    ~QDataStream();

    QIODevice *device() const;
    void setDevice(QIODevice *);

    bool atEnd() const;

    Status status() const;
    void setStatus(Status status);
    void resetStatus();

    FloatingPointPrecision floatingPointPrecision() const;
    void setFloatingPointPrecision(FloatingPointPrecision precision);

    ByteOrder byteOrder() const;
    void setByteOrder(ByteOrder);

    int version() const;
    void setVersion(int);

    QDataStream &operator>>(char &i);
    QDataStream &operator>>(qint8 &i);
    QDataStream &operator>>(quint8 &i);
    QDataStream &operator>>(qint16 &i);
    QDataStream &operator>>(quint16 &i);
    QDataStream &operator>>(qint32 &i);
    inline QDataStream &operator>>(quint32 &i);
    QDataStream &operator>>(qint64 &i);
    QDataStream &operator>>(quint64 &i);
    QDataStream &operator>>(std::nullptr_t &ptr) { ptr = nullptr; return *this; }

    QDataStream &operator>>(bool &i);
    QDataStream &operator>>(qfloat16 &f);
    QDataStream &operator>>(float &f);
    QDataStream &operator>>(double &f);
    QDataStream &operator>>(char *&str);
    QDataStream &operator>>(char16_t &c);
    QDataStream &operator>>(char32_t &c);

    QDataStream &operator<<(char i);
    QDataStream &operator<<(qint8 i);
    QDataStream &operator<<(quint8 i);
    QDataStream &operator<<(qint16 i);
    QDataStream &operator<<(quint16 i);
    QDataStream &operator<<(qint32 i);
    inline QDataStream &operator<<(quint32 i);
    QDataStream &operator<<(qint64 i);
    QDataStream &operator<<(quint64 i);
    QDataStream &operator<<(std::nullptr_t) { return *this; }
    QDataStream &operator<<(bool i);
    QDataStream &operator<<(qfloat16 f);
    QDataStream &operator<<(float f);
    QDataStream &operator<<(double f);
    QDataStream &operator<<(const char *str);
    QDataStream &operator<<(char16_t c);
    QDataStream &operator<<(char32_t c);


    QDataStream &readBytes(char *&, uint &len);
    int readRawData(char *, int len);

    QDataStream &writeBytes(const char *, uint len);
    int writeRawData(const char *, int len);

    int skipRawData(int len);

    void startTransaction();
    bool commitTransaction();
    void rollbackTransaction();
    void abortTransaction();

    bool isDeviceTransactionStarted() const;
private:
    Q_DISABLE_COPY(QDataStream)

    QScopedPointer<QDataStreamPrivate> d;

    QIODevice *dev;
    bool owndev;
    bool noswap;
    ByteOrder byteorder;
    int ver;
    Status q_status;

    int readBlock(char *data, int len);
    friend class QtPrivate::StreamStateSaver;
};

namespace QtPrivate {

class StreamStateSaver
{
public:
    inline StreamStateSaver(QDataStream *s) : stream(s), oldStatus(s->status())
    {
        if (!stream->isDeviceTransactionStarted())
            stream->resetStatus();
    }
    inline ~StreamStateSaver()
    {
        if (oldStatus != QDataStream::Ok) {
            stream->resetStatus();
            stream->setStatus(oldStatus);
        }
    }

private:
    QDataStream *stream;
    QDataStream::Status oldStatus;
};

template <typename Container>
QDataStream &readArrayBasedContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    quint32 n;
    s >> n;
    c.reserve(n);
    for (quint32 i = 0; i < n; ++i) {
        typename Container::value_type t;
        s >> t;
        if (s.status() != QDataStream::Ok) {
            c.clear();
            break;
        }
        c.append(t);
    }

    return s;
}

template <typename Container>
QDataStream &readListBasedContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    quint32 n;
    s >> n;
    for (quint32 i = 0; i < n; ++i) {
        typename Container::value_type t;
        s >> t;
        if (s.status() != QDataStream::Ok) {
            c.clear();
            break;
        }
        c << t;
    }

    return s;
}

template <typename Container>
QDataStream &readAssociativeContainer(QDataStream &s, Container &c)
{
    StreamStateSaver stateSaver(&s);

    c.clear();
    quint32 n;
    s >> n;
    for (quint32 i = 0; i < n; ++i) {
        typename Container::key_type k;
        typename Container::mapped_type t;
        s >> k >> t;
        if (s.status() != QDataStream::Ok) {
            c.clear();
            break;
        }
        c.insert(k, t);
    }

    return s;
}

template <typename Container>
QDataStream &writeSequentialContainer(QDataStream &s, const Container &c)
{
    s << quint32(c.size());
    for (const typename Container::value_type &t : c)
        s << t;

    return s;
}

template <typename Container>
QDataStream &writeAssociativeContainer(QDataStream &s, const Container &c)
{
    s << quint32(c.size());
    auto it = c.constBegin();
    auto end = c.constEnd();
    while (it != end) {
        s << it.key() << it.value();
        ++it;
    }

    return s;
}

template <typename Container>
QDataStream &writeAssociativeMultiContainer(QDataStream &s, const Container &c)
{
    s << quint32(c.size());
    auto it = c.constBegin();
    auto end = c.constEnd();
    while (it != end) {
        const auto rangeStart = it++;
        while (it != end && rangeStart.key() == it.key())
            ++it;
        const qint64 last = std::distance(rangeStart, it) - 1;
        for (qint64 i = last; i >= 0; --i) {
            auto next = std::next(rangeStart, i);
            s << next.key() << next.value();
        }
    }

    return s;
}

} // QtPrivate namespace

template<typename ...T>
using QDataStreamIfHasOStreamOperators =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_ostream_operator<QDataStream, T>...>, QDataStream &>;
template<typename ...T>
using QDataStreamIfHasIStreamOperators =
    std::enable_if_t<std::conjunction_v<QTypeTraits::has_istream_operator<QDataStream, T>...>, QDataStream &>;

/*****************************************************************************
  QDataStream inline functions
 *****************************************************************************/

inline QIODevice *QDataStream::device() const
{ return dev; }

inline QDataStream::ByteOrder QDataStream::byteOrder() const
{ return byteorder; }

inline int QDataStream::version() const
{ return ver; }

inline void QDataStream::setVersion(int v)
{ ver = v; }

inline QDataStream &QDataStream::operator>>(char &i)
{ return *this >> reinterpret_cast<qint8&>(i); }

inline QDataStream &QDataStream::operator>>(quint8 &i)
{ return *this >> reinterpret_cast<qint8&>(i); }

inline QDataStream &QDataStream::operator>>(quint16 &i)
{ return *this >> reinterpret_cast<qint16&>(i); }

inline QDataStream &QDataStream::operator>>(quint32 &i)
{ return *this >> reinterpret_cast<qint32&>(i); }

inline QDataStream &QDataStream::operator>>(quint64 &i)
{ return *this >> reinterpret_cast<qint64&>(i); }

inline QDataStream &QDataStream::operator<<(char i)
{ return *this << qint8(i); }

inline QDataStream &QDataStream::operator<<(quint8 i)
{ return *this << qint8(i); }

inline QDataStream &QDataStream::operator<<(quint16 i)
{ return *this << qint16(i); }

inline QDataStream &QDataStream::operator<<(quint32 i)
{ return *this << qint32(i); }

inline QDataStream &QDataStream::operator<<(quint64 i)
{ return *this << qint64(i); }

template <typename Enum>
inline QDataStream &operator<<(QDataStream &s, QFlags<Enum> e)
{ return s << typename QFlags<Enum>::Int(e); }

template <typename Enum>
inline QDataStream &operator>>(QDataStream &s, QFlags<Enum> &e)
{
    typename QFlags<Enum>::Int i;
    s >> i;
    e = QFlag(i);
    return s;
}

template <typename T>
typename std::enable_if_t<std::is_enum<T>::value, QDataStream &>
operator<<(QDataStream &s, const T &t)
{ return s << static_cast<typename std::underlying_type<T>::type>(t); }

template <typename T>
typename std::enable_if_t<std::is_enum<T>::value, QDataStream &>
operator>>(QDataStream &s, T &t)
{ return s >> reinterpret_cast<typename std::underlying_type<T>::type &>(t); }

template<typename T>
inline QDataStreamIfHasIStreamOperators<T> operator>>(QDataStream &s, QList<T> &v)
{
    return QtPrivate::readArrayBasedContainer(s, v);
}

template<typename T>
inline QDataStreamIfHasOStreamOperators<T> operator<<(QDataStream &s, const QList<T> &v)
{
    return QtPrivate::writeSequentialContainer(s, v);
}

template <typename T>
inline QDataStreamIfHasIStreamOperators<T> operator>>(QDataStream &s, QSet<T> &set)
{
    return QtPrivate::readListBasedContainer(s, set);
}

template <typename T>
inline QDataStreamIfHasOStreamOperators<T> operator<<(QDataStream &s, const QSet<T> &set)
{
    return QtPrivate::writeSequentialContainer(s, set);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperators<Key, T> operator>>(QDataStream &s, QHash<Key, T> &hash)
{
    return QtPrivate::readAssociativeContainer(s, hash);
}

template <class Key, class T>

inline QDataStreamIfHasOStreamOperators<Key, T> operator<<(QDataStream &s, const QHash<Key, T> &hash)
{
    return QtPrivate::writeAssociativeContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperators<Key, T> operator>>(QDataStream &s, QMultiHash<Key, T> &hash)
{
    return QtPrivate::readAssociativeContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperators<Key, T> operator<<(QDataStream &s, const QMultiHash<Key, T> &hash)
{
    return QtPrivate::writeAssociativeMultiContainer(s, hash);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperators<Key, T> operator>>(QDataStream &s, QMap<Key, T> &map)
{
    return QtPrivate::readAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperators<Key, T> operator<<(QDataStream &s, const QMap<Key, T> &map)
{
    return QtPrivate::writeAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasIStreamOperators<Key, T> operator>>(QDataStream &s, QMultiMap<Key, T> &map)
{
    return QtPrivate::readAssociativeContainer(s, map);
}

template <class Key, class T>
inline QDataStreamIfHasOStreamOperators<Key, T> operator<<(QDataStream &s, const QMultiMap<Key, T> &map)
{
    return QtPrivate::writeAssociativeMultiContainer(s, map);
}

#ifndef QT_NO_DATASTREAM
template <class T1, class T2>
inline QDataStreamIfHasIStreamOperators<T1, T2> operator>>(QDataStream& s, std::pair<T1, T2> &p)
{
    s >> p.first >> p.second;
    return s;
}

template <class T1, class T2>
inline QDataStreamIfHasOStreamOperators<T1, T2> operator<<(QDataStream& s, const std::pair<T1, T2> &p)
{
    s << p.first << p.second;
    return s;
}
#endif

inline QDataStream &operator>>(QDataStream &s, QKeyCombination &combination)
{
    int combined;
    s >> combined;
    combination = QKeyCombination::fromCombined(combined);
    return s;
}

inline QDataStream &operator<<(QDataStream &s, QKeyCombination combination)
{
    return s << combination.toCombined();
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QDATASTREAM_H
