// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUUID_H
#define QUUID_H

#include <QtCore/qendian.h>
#include <QtCore/qstring.h>

#if defined(Q_OS_WIN) || defined(Q_QDOC)
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    ulong   Data1;
    ushort  Data2;
    ushort  Data3;
    uchar   Data4[8];
} GUID, *REFGUID, *LPGUID;
#endif
#endif

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFUUID);
Q_FORWARD_DECLARE_OBJC_CLASS(NSUUID);
#endif

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QUuid
{
    QUuid(Qt::Initialization) {}
public:
    enum Variant {
        VarUnknown        =-1,
        NCS                = 0, // 0 - -
        DCE                = 2, // 1 0 -
        Microsoft        = 6, // 1 1 0
        Reserved        = 7  // 1 1 1
    };

    enum Version {
        VerUnknown        =-1,
        Time                = 1, // 0 0 0 1
        EmbeddedPOSIX        = 2, // 0 0 1 0
        Md5                 = 3, // 0 0 1 1
        Name = Md5,
        Random                = 4,  // 0 1 0 0
        Sha1                 = 5 // 0 1 0 1
    };

    enum StringFormat {
        WithBraces      = 0,
        WithoutBraces   = 1,
        Id128           = 3
    };

    union Id128Bytes {
        quint8 data[16];
        quint16 data16[8];
        quint32 data32[4];
        quint64 data64[2];

        constexpr explicit operator QByteArrayView() const noexcept
        {
            return QByteArrayView(data, sizeof(data));
        }
    };

    constexpr QUuid() noexcept {}

    constexpr QUuid(uint l, ushort w1, ushort w2, uchar b1, uchar b2, uchar b3,
                           uchar b4, uchar b5, uchar b6, uchar b7, uchar b8) noexcept
        : data1(l), data2(w1), data3(w2), data4{b1, b2, b3, b4, b5, b6, b7, b8} {}
    QUuid(Id128Bytes id128) noexcept;

    explicit QUuid(QAnyStringView string) noexcept
        : QUuid{fromString(string)} {}
    static QUuid fromString(QAnyStringView string) noexcept;
#if QT_CORE_REMOVED_SINCE(6, 3)
    explicit QUuid(const QString &);
    static QUuid fromString(QStringView string) noexcept;
    static QUuid fromString(QLatin1StringView string) noexcept;
    explicit QUuid(const char *);
    explicit QUuid(const QByteArray &);
#endif
    QString toString(StringFormat mode = WithBraces) const;
    QByteArray toByteArray(StringFormat mode = WithBraces) const;
    Id128Bytes toBytes() const noexcept;
    QByteArray toRfc4122() const;

    static QUuid fromBytes(const void *bytes) noexcept;
#if QT_CORE_REMOVED_SINCE(6, 3)
    static QUuid fromRfc4122(const QByteArray &);
#endif
    static QUuid fromRfc4122(QByteArrayView) noexcept;

    bool isNull() const noexcept;

    constexpr bool operator==(const QUuid &orig) const noexcept
    {
        if (data1 != orig.data1 || data2 != orig.data2 ||
             data3 != orig.data3)
            return false;

        for (uint i = 0; i < 8; i++)
            if (data4[i] != orig.data4[i])
                return false;

        return true;
    }

    constexpr bool operator!=(const QUuid &orig) const noexcept
    {
        return !(*this == orig);
    }

    bool operator<(const QUuid &other) const noexcept;
    bool operator>(const QUuid &other) const noexcept;

#if defined(Q_OS_WIN) || defined(Q_QDOC)
    // On Windows we have a type GUID that is used by the platform API, so we
    // provide convenience operators to cast from and to this type.
    constexpr QUuid(const GUID &guid) noexcept
        : data1(guid.Data1), data2(guid.Data2), data3(guid.Data3),
          data4{guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]} {}

    constexpr QUuid &operator=(const GUID &guid) noexcept
    {
        *this = QUuid(guid);
        return *this;
    }

    constexpr operator GUID() const noexcept
    {
        GUID guid = { data1, data2, data3, { data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7] } };
        return guid;
    }

    constexpr bool operator==(const GUID &guid) const noexcept
    {
        return *this == QUuid(guid);
    }

    constexpr bool operator!=(const GUID &guid) const noexcept
    {
        return !(*this == guid);
    }
#endif
    static QUuid createUuid();
#ifndef QT_BOOTSTRAPPED
    static QUuid createUuidV3(const QUuid &ns, const QByteArray &baseData);
#endif
    static QUuid createUuidV5(const QUuid &ns, const QByteArray &baseData);
#ifndef QT_BOOTSTRAPPED
    static inline QUuid createUuidV3(const QUuid &ns, const QString &baseData)
    {
        return QUuid::createUuidV3(ns, baseData.toUtf8());
    }
#endif

    static inline QUuid createUuidV5(const QUuid &ns, const QString &baseData)
    {
        return QUuid::createUuidV5(ns, baseData.toUtf8());
    }

    QUuid::Variant variant() const noexcept;
    QUuid::Version version() const noexcept;

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QUuid fromCFUUID(CFUUIDRef uuid);
    CFUUIDRef toCFUUID() const Q_DECL_CF_RETURNS_RETAINED;
    static QUuid fromNSUUID(const NSUUID *uuid);
    NSUUID *toNSUUID() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

    uint    data1 = 0;
    ushort  data2 = 0;
    ushort  data3 = 0;
    uchar   data4[8] = {};
};

Q_DECLARE_TYPEINFO(QUuid, Q_PRIMITIVE_TYPE);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QUuid &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QUuid &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QUuid &);
#endif

Q_CORE_EXPORT size_t qHash(const QUuid &uuid, size_t seed = 0) noexcept;

inline QUuid::QUuid(Id128Bytes uuid) noexcept
{
    data1 = qFromBigEndian<quint32>(&uuid.data[0]);
    data2 = qFromBigEndian<quint16>(&uuid.data[4]);
    data3 = qFromBigEndian<quint16>(&uuid.data[6]);
    memcpy(data4, &uuid.data[8], sizeof(data4));
}

inline QUuid::Id128Bytes QUuid::toBytes() const noexcept
{
    Id128Bytes result = {};
    qToBigEndian(data1, &result.data[0]);
    qToBigEndian(data2, &result.data[4]);
    qToBigEndian(data3, &result.data[6]);
    memcpy(&result.data[8], data4, sizeof(data4));
    return result;
}

inline QUuid QUuid::fromBytes(const void *bytes) noexcept
{
    Id128Bytes result = {};
    memcpy(result.data, bytes, sizeof(result));
    return QUuid(result);
}

inline bool operator<=(const QUuid &lhs, const QUuid &rhs) noexcept
{ return !(rhs < lhs); }
inline bool operator>=(const QUuid &lhs, const QUuid &rhs) noexcept
{ return !(lhs < rhs); }

QT_END_NAMESPACE

#endif // QUUID_H
