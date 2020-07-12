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

#ifndef QVARIANT_H
#define QVARIANT_H

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qmap.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qobject.h>
#ifndef QT_BOOTSTRAPPED
#include <QtCore/qbytearraylist.h>
#endif
#include <memory>

#if __has_include(<variant>) && __cplusplus >= 201703L
#include <variant>
#elif defined(Q_CLANG_QDOC)
namespace std { template<typename...> struct variant; }
#endif

QT_BEGIN_NAMESPACE


class QBitArray;
class QDataStream;
class QDate;
class QDateTime;
#if QT_CONFIG(easingcurve)
class QEasingCurve;
#endif
class QLine;
class QLineF;
class QLocale;
class QTransform;
class QStringList;
class QTime;
class QPoint;
class QPointF;
class QSize;
class QSizeF;
class QRect;
class QRectF;
#if QT_CONFIG(regularexpression)
class QRegularExpression;
#endif // QT_CONFIG(regularexpression)
class QTextFormat;
class QTextLength;
class QUrl;
class QVariant;
class QVariantComparisonHelper;

template<typename T>
inline T qvariant_cast(const QVariant &);

namespace QtPrivate {

    template <typename Derived, typename Argument, typename ReturnType>
    struct ObjectInvoker
    {
        static ReturnType invoke(Argument a)
        {
            return Derived::object(a);
        }
    };

    template <typename Derived, typename Argument, typename ReturnType>
    struct MetaTypeInvoker
    {
        static ReturnType invoke(Argument a)
        {
            return Derived::metaType(a);
        }
    };

    template <typename Derived, typename T, typename Argument, typename ReturnType, bool = IsPointerToTypeDerivedFromQObject<T>::Value>
    struct TreatAsQObjectBeforeMetaType : ObjectInvoker<Derived, Argument, ReturnType>
    {
    };

    template <typename Derived, typename T, typename Argument, typename ReturnType>
    struct TreatAsQObjectBeforeMetaType<Derived, T, Argument, ReturnType, false> : MetaTypeInvoker<Derived, Argument, ReturnType>
    {
    };

    template<typename T> struct QVariantValueHelper;
}

class Q_CORE_EXPORT QVariant
{
 public:
    enum Type {
        Invalid = QMetaType::UnknownType,
        Bool = QMetaType::Bool,
        Int = QMetaType::Int,
        UInt = QMetaType::UInt,
        LongLong = QMetaType::LongLong,
        ULongLong = QMetaType::ULongLong,
        Double = QMetaType::Double,
        Char = QMetaType::QChar,
        Map = QMetaType::QVariantMap,
        List = QMetaType::QVariantList,
        String = QMetaType::QString,
        StringList = QMetaType::QStringList,
        ByteArray = QMetaType::QByteArray,
        BitArray = QMetaType::QBitArray,
        Date = QMetaType::QDate,
        Time = QMetaType::QTime,
        DateTime = QMetaType::QDateTime,
        Url = QMetaType::QUrl,
        Locale = QMetaType::QLocale,
        Rect = QMetaType::QRect,
        RectF = QMetaType::QRectF,
        Size = QMetaType::QSize,
        SizeF = QMetaType::QSizeF,
        Line = QMetaType::QLine,
        LineF = QMetaType::QLineF,
        Point = QMetaType::QPoint,
        PointF = QMetaType::QPointF,
#if QT_CONFIG(regularexpression)
        RegularExpression = QMetaType::QRegularExpression,
#endif
        Hash = QMetaType::QVariantHash,
#if QT_CONFIG(easingcurve)
        EasingCurve = QMetaType::QEasingCurve,
#endif
        Uuid = QMetaType::QUuid,
#if QT_CONFIG(itemmodel)
        ModelIndex = QMetaType::QModelIndex,
        PersistentModelIndex = QMetaType::QPersistentModelIndex,
#endif
        LastCoreType = QMetaType::LastCoreType,

        Font = QMetaType::QFont,
        Pixmap = QMetaType::QPixmap,
        Brush = QMetaType::QBrush,
        Color = QMetaType::QColor,
        Palette = QMetaType::QPalette,
        Image = QMetaType::QImage,
        Polygon = QMetaType::QPolygon,
        Region = QMetaType::QRegion,
        Bitmap = QMetaType::QBitmap,
        Cursor = QMetaType::QCursor,
#if QT_CONFIG(shortcut)
        KeySequence = QMetaType::QKeySequence,
#endif
        Pen = QMetaType::QPen,
        TextLength = QMetaType::QTextLength,
        TextFormat = QMetaType::QTextFormat,
        Transform = QMetaType::QTransform,
        Matrix4x4 = QMetaType::QMatrix4x4,
        Vector2D = QMetaType::QVector2D,
        Vector3D = QMetaType::QVector3D,
        Vector4D = QMetaType::QVector4D,
        Quaternion = QMetaType::QQuaternion,
        PolygonF = QMetaType::QPolygonF,
        Icon = QMetaType::QIcon,
        LastGuiType = QMetaType::LastGuiType,

        SizePolicy = QMetaType::QSizePolicy,

        UserType = QMetaType::User,
        LastType = 0xffffffff // need this so that gcc >= 3.4 allocates 32 bits for Type
    };

    QVariant() noexcept : d() {}
    ~QVariant();
    explicit QVariant(Type type);
    explicit QVariant(QMetaType type, const void *copy = nullptr);
    QVariant(const QVariant &other);

#ifndef QT_NO_DATASTREAM
    explicit QVariant(QDataStream &s);
#endif

    QVariant(int i);
    QVariant(uint ui);
    QVariant(qlonglong ll);
    QVariant(qulonglong ull);
    QVariant(bool b);
    QVariant(double d);
    QVariant(float f);
#ifndef QT_NO_CAST_FROM_ASCII
    QT_ASCII_CAST_WARN QVariant(const char *str)
        : QVariant(QString::fromUtf8(str))
    {}
#endif

    QVariant(const QByteArray &bytearray);
    QVariant(const QBitArray &bitarray);
    QVariant(const QString &string);
    QVariant(QLatin1String string);
    QVariant(const QStringList &stringlist);
    QVariant(QChar qchar);
    QVariant(QDate date);
    QVariant(QTime time);
    QVariant(const QDateTime &datetime);
    QVariant(const QList<QVariant> &list);
    QVariant(const QMap<QString,QVariant> &map);
    QVariant(const QHash<QString,QVariant> &hash);
#ifndef QT_NO_GEOM_VARIANT
    QVariant(const QSize &size);
    QVariant(const QSizeF &size);
    QVariant(const QPoint &pt);
    QVariant(const QPointF &pt);
    QVariant(const QLine &line);
    QVariant(const QLineF &line);
    QVariant(const QRect &rect);
    QVariant(const QRectF &rect);
#endif
    QVariant(const QLocale &locale);
#if QT_CONFIG(regularexpression)
    QVariant(const QRegularExpression &re);
#endif // QT_CONFIG(regularexpression)
#if QT_CONFIG(easingcurve)
    QVariant(const QEasingCurve &easing);
#endif
    QVariant(const QUuid &uuid);
#ifndef QT_BOOTSTRAPPED
    QVariant(const QUrl &url);
    QVariant(const QJsonValue &jsonValue);
    QVariant(const QJsonObject &jsonObject);
    QVariant(const QJsonArray &jsonArray);
    QVariant(const QJsonDocument &jsonDocument);
#endif // QT_BOOTSTRAPPED
#if QT_CONFIG(itemmodel)
    QVariant(const QModelIndex &modelIndex);
    QVariant(const QPersistentModelIndex &modelIndex);
#endif

    QVariant& operator=(const QVariant &other);
    inline QVariant(QVariant &&other) noexcept : d(other.d)
    { other.d = Private(); }
    inline QVariant &operator=(QVariant &&other) noexcept
    { QVariant moved(std::move(other)); swap(moved); return *this; }

    inline void swap(QVariant &other) noexcept { qSwap(d, other.d); }

    Type type() const;
    int userType() const;
    const char *typeName() const;
    QMetaType metaType() const;

    bool canConvert(int targetTypeId) const;
    bool convert(int targetTypeId);

    inline bool isValid() const;
    bool isNull() const;

    void clear();

    void detach();
    inline bool isDetached() const;

    int toInt(bool *ok = nullptr) const;
    uint toUInt(bool *ok = nullptr) const;
    qlonglong toLongLong(bool *ok = nullptr) const;
    qulonglong toULongLong(bool *ok = nullptr) const;
    bool toBool() const;
    double toDouble(bool *ok = nullptr) const;
    float toFloat(bool *ok = nullptr) const;
    qreal toReal(bool *ok = nullptr) const;
    QByteArray toByteArray() const;
    QBitArray toBitArray() const;
    QString toString() const;
    QStringList toStringList() const;
    QChar toChar() const;
    QDate toDate() const;
    QTime toTime() const;
    QDateTime toDateTime() const;
    QList<QVariant> toList() const;
    QMap<QString, QVariant> toMap() const;
    QHash<QString, QVariant> toHash() const;

#ifndef QT_NO_GEOM_VARIANT
    QPoint toPoint() const;
    QPointF toPointF() const;
    QRect toRect() const;
    QSize toSize() const;
    QSizeF toSizeF() const;
    QLine toLine() const;
    QLineF toLineF() const;
    QRectF toRectF() const;
#endif
    QLocale toLocale() const;
#if QT_CONFIG(regularexpression)
    QRegularExpression toRegularExpression() const;
#endif // QT_CONFIG(regularexpression)
#if QT_CONFIG(easingcurve)
    QEasingCurve toEasingCurve() const;
#endif
    QUuid toUuid() const;
#ifndef QT_BOOTSTRAPPED
    QUrl toUrl() const;
    QJsonValue toJsonValue() const;
    QJsonObject toJsonObject() const;
    QJsonArray toJsonArray() const;
    QJsonDocument toJsonDocument() const;
#endif // QT_BOOTSTRAPPED
#if QT_CONFIG(itemmodel)
    QModelIndex toModelIndex() const;
    QPersistentModelIndex toPersistentModelIndex() const;
#endif

#ifndef QT_NO_DATASTREAM
    void load(QDataStream &ds);
    void save(QDataStream &ds) const;
#endif
    static const char *typeToName(int typeId);
    static Type nameToType(const char *name);

    void *data();
    const void *constData() const
    { return d.is_shared ? d.data.shared->data() : &d.data.ptr; }
    inline const void *data() const { return constData(); }

    template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, QVariant>>>
    void setValue(T &&avalue)
    {
        using VT = std::decay_t<T>;
        QMetaType metaType = QMetaType::fromType<VT>();
        // If possible we reuse the current QVariant private.
        if (isDetached() && d.type() == metaType) {
            *reinterpret_cast<VT *>(const_cast<void *>(constData())) = std::forward<T>(avalue);
        } else {
            *this = QVariant::fromValue<VT>(std::forward<T>(avalue));
        }
    }

    void setValue(const QVariant &avalue)
    {
        *this = avalue;
    }

    void setValue(QVariant &&avalue)
    {
        *this = std::move(avalue);
    }

    template<typename T>
    inline T value() const
    { return qvariant_cast<T>(*this); }

    template<typename T>
    static inline QVariant fromValue(const T &value)
    {
        return QVariant(QMetaType::fromType<T>(), std::addressof(value));
    }

#if (__has_include(<variant>) && __cplusplus >= 201703L) || defined(Q_CLANG_QDOC)
    template<typename... Types>
    static inline QVariant fromStdVariant(const std::variant<Types...> &value)
    {
        if (value.valueless_by_exception())
            return QVariant();
        return std::visit([](const auto &arg) { return fromValue(arg); }, value);
    }
#endif

    template<typename T>
    bool canConvert() const
    { return canConvert(qMetaTypeId<T>()); }

 public:
    struct PrivateShared
    {
    private:
        inline PrivateShared() : ref(1) { }
    public:
        static PrivateShared *create(QMetaType type)
        {
            size_t size = type.sizeOf();
            size_t align = type.alignOf();

            size += sizeof(PrivateShared);
            if (align > sizeof(PrivateShared)) {
                // The alignment is larger than the alignment we can guarantee for the pointer
                // directly following PrivateShared, so we need to allocate some additional
                // memory to be able to fit the object into the available memory with suitable
                // alignment.
                size += align - sizeof(PrivateShared);
            }
            void *data = operator new(size);
            auto *ps = new (data) QVariant::PrivateShared();
            ps->offset = int(((quintptr(ps) + sizeof(PrivateShared) + align - 1) & ~(align - 1)) - quintptr(ps));
            return ps;
        }
        static void free(PrivateShared *p)
        {
            p->~PrivateShared();
            operator delete(p);
        }

        alignas(8) QAtomicInt ref;
        int offset;

        const void *data() const
        { return reinterpret_cast<const unsigned char *>(this) + offset; }
        void *data()
        { return reinterpret_cast<unsigned char *>(this) + offset; }
    };
    struct Private
    {
        Private() noexcept : packedType(0), is_shared(false), is_null(true) {}
        explicit Private(const QMetaType &type) noexcept : is_shared(false), is_null(false)
        {
            if (type.d_ptr)
                type.d_ptr->ref.ref();
            quintptr mt = quintptr(type.d_ptr);
            Q_ASSERT((mt & 0x3) == 0);
            packedType = mt >> 2;
        }
        explicit Private(int type) noexcept : Private(QMetaType(type)) {}
        Private(const Private &other) : Private(other.type())
        {
            data = other.data;
            is_shared = other.is_shared;
            is_null = other.is_null;
        }
        Private &operator=(const Private &other)
        {
            if (&other != this) {
                this->~Private();
                new (this) Private(other);
            }
            return *this;
        }
        Q_CORE_EXPORT ~Private();

        union Data
        {
            void *threeptr[3] = { nullptr, nullptr, nullptr };
            char c;
            uchar uc;
            short s;
            signed char sc;
            ushort us;
            int i;
            uint u;
            long l;
            ulong ul;
            bool b;
            double d;
            float f;
            qreal real;
            qlonglong ll;
            qulonglong ull;
            QObject *o;
            void *ptr;
            PrivateShared *shared;
        } data;
        quintptr packedType : sizeof(QMetaType) * 8 - 2;
        quintptr is_shared : 1;
        quintptr is_null : 1;

        template<typename T>
        static constexpr bool CanUseInternalSpace = sizeof(T) <= sizeof(QVariant::Private::Data);

        const void *storage() const
        { return is_shared ? data.shared->data() : &data; }

        const void *internalStorage() const
        { Q_ASSERT(is_shared); return &data; }

        // determine internal storage at compile time
        template<typename T>
        const T &get() const
        { return *static_cast<const T *>(CanUseInternalSpace<T> ? &data : data.shared->data()); }

        inline QMetaType type() const
        {
            return QMetaType(reinterpret_cast<QtPrivate::QMetaTypeInterface *>(packedType << 2));
        }
        inline int typeId() const
        {
            return type().id();
        }
    };
 public:
    inline bool operator==(const QVariant &v) const
    { return equals(v); }
    inline bool operator!=(const QVariant &v) const
    { return !equals(v); }

protected:
    friend inline bool operator==(const QVariant &, const QVariantComparisonHelper &);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant &);
#endif
    template<typename T>
    friend inline T qvariant_cast(const QVariant &);
    template<typename T> friend struct QtPrivate::QVariantValueHelper;
protected:
    Private d;
    void create(int type, const void *copy);
    bool equals(const QVariant &other) const;
    bool convert(const int t, void *ptr) const; // ### Qt6: drop const

private:
    // force compile error, prevent QVariant(bool) to be called
    inline QVariant(void *) = delete;
    // QVariant::Type is marked as \obsolete, but we don't want to
    // provide a constructor from its intended replacement,
    // QMetaType::Type, instead, because the idea behind these
    // constructors is flawed in the first place. But we also don't
    // want QVariant(QMetaType::String) to compile and falsely be an
    // int variant, so delete this constructor:
    QVariant(QMetaType::Type) = delete;

    // These constructors don't create QVariants of the type associcated
    // with the enum, as expected, but they would create a QVariant of
    // type int with the value of the enum value.
    // Use QVariant v = QColor(Qt::red) instead of QVariant v = Qt::red for
    // example.
    QVariant(Qt::GlobalColor) = delete;
    QVariant(Qt::BrushStyle) = delete;
    QVariant(Qt::PenStyle) = delete;
    QVariant(Qt::CursorShape) = delete;
#ifdef QT_NO_CAST_FROM_ASCII
    // force compile error when implicit conversion is not wanted
    inline QVariant(const char *) = delete;
#endif
public:
    typedef Private DataPtr;
    inline DataPtr &data_ptr() { return d; }
    inline const DataPtr &data_ptr() const { return d; }
};

template<>
inline QVariant QVariant::fromValue(const QVariant &value)
{
    return value;
}

#if __has_include(<variant>) && __cplusplus >= 201703L
template<>
inline QVariant QVariant::fromValue(const std::monostate &)
{
    return QVariant();
}
#endif

inline bool QVariant::isValid() const
{
    return d.type().isValid();
}

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream& operator>> (QDataStream& s, QVariant& p);
Q_CORE_EXPORT QDataStream& operator<< (QDataStream& s, const QVariant& p);
Q_CORE_EXPORT QDataStream& operator>> (QDataStream& s, QVariant::Type& p);
Q_CORE_EXPORT QDataStream& operator<< (QDataStream& s, const QVariant::Type p);
#endif

inline bool QVariant::isDetached() const
{ return !d.is_shared || d.data.shared->ref.loadRelaxed() == 1; }


#ifdef Q_QDOC
    inline bool operator==(const QVariant &v1, const QVariant &v2);
    inline bool operator!=(const QVariant &v1, const QVariant &v2);
#else

/* Helper class to add one more level of indirection to prevent
   implicit casts.
*/
class QVariantComparisonHelper
{
public:
    inline QVariantComparisonHelper(const QVariant &var)
        : v(&var) {}
private:
    friend inline bool operator==(const QVariant &, const QVariantComparisonHelper &);
    const QVariant *v;
};

inline bool operator==(const QVariant &v1, const QVariantComparisonHelper &v2)
{
    return v1.equals(*v2.v);
}

inline bool operator!=(const QVariant &v1, const QVariantComparisonHelper &v2)
{
    return !operator==(v1, v2);
}
#endif
Q_DECLARE_SHARED(QVariant)

class Q_CORE_EXPORT QSequentialIterable
{
    QtMetaTypePrivate::QSequentialIterableImpl m_impl;
public:
    struct Q_CORE_EXPORT const_iterator
    {
    private:
        QtMetaTypePrivate::QSequentialIterableImpl m_impl;
        QAtomicInt *ref;
        friend class QSequentialIterable;
        explicit const_iterator(const QSequentialIterable &iter, QAtomicInt *ref_);

        explicit const_iterator(const QtMetaTypePrivate::QSequentialIterableImpl &impl, QAtomicInt *ref_);

        void begin();
        void end();
    public:
        ~const_iterator();

        const_iterator(const const_iterator &other);

        const_iterator& operator=(const const_iterator &other);

        const QVariant operator*() const;
        bool operator==(const const_iterator &o) const;
        bool operator!=(const const_iterator &o) const;
        const_iterator &operator++();
        const_iterator operator++(int);
        const_iterator &operator--();
        const_iterator operator--(int);
        const_iterator &operator+=(int j);
        const_iterator &operator-=(int j);
        const_iterator operator+(int j) const;
        const_iterator operator-(int j) const;
        friend inline const_iterator operator+(int j, const const_iterator &k) { return k + j; }
    };

    friend struct const_iterator;

    explicit QSequentialIterable(const QtMetaTypePrivate::QSequentialIterableImpl &impl);

    const_iterator begin() const;
    const_iterator end() const;

    QVariant at(int idx) const;
    int size() const;

    bool canReverseIterate() const;
};

class Q_CORE_EXPORT QAssociativeIterable
{
    QtMetaTypePrivate::QAssociativeIterableImpl m_impl;
public:
    struct Q_CORE_EXPORT const_iterator
    {
    private:
        QtMetaTypePrivate::QAssociativeIterableImpl m_impl;
        QAtomicInt *ref;
        friend class QAssociativeIterable;
        explicit const_iterator(const QAssociativeIterable &iter, QAtomicInt *ref_);

        explicit const_iterator(const QtMetaTypePrivate::QAssociativeIterableImpl &impl, QAtomicInt *ref_);

        void begin();
        void end();
        void find(const QVariant &key);
    public:
        ~const_iterator();
        const_iterator(const const_iterator &other);

        const_iterator& operator=(const const_iterator &other);

        const QVariant key() const;

        const QVariant value() const;

        const QVariant operator*() const;
        bool operator==(const const_iterator &o) const;
        bool operator!=(const const_iterator &o) const;
        const_iterator &operator++();
        const_iterator operator++(int);
        const_iterator &operator--();
        const_iterator operator--(int);
        const_iterator &operator+=(int j);
        const_iterator &operator-=(int j);
        const_iterator operator+(int j) const;
        const_iterator operator-(int j) const;
        friend inline const_iterator operator+(int j, const const_iterator &k) { return k + j; }
    };

    friend struct const_iterator;

    explicit QAssociativeIterable(const QtMetaTypePrivate::QAssociativeIterableImpl &impl);

    const_iterator begin() const;
    const_iterator end() const;
    const_iterator find(const QVariant &key) const;

    QVariant value(const QVariant &key) const;

    int size() const;
};

#ifndef QT_MOC
namespace QtPrivate {
    template<typename T>
    struct QVariantValueHelper : TreatAsQObjectBeforeMetaType<QVariantValueHelper<T>, T, const QVariant &, T>
    {
        static T metaType(const QVariant &v)
        {
            const int vid = qMetaTypeId<T>();
            if (vid == v.userType())
                return *reinterpret_cast<const T *>(v.constData());
            T t;
            if (v.convert(vid, &t))
                return t;
            return T();
        }
#ifndef QT_NO_QOBJECT
        static T object(const QVariant &v)
        {
            return qobject_cast<T>(QMetaType::typeFlags(v.userType()) & QMetaType::PointerToQObject
                ? v.d.data.o
                : QVariantValueHelper::metaType(v));
        }
#endif
    };

    template<typename T>
    struct QVariantValueHelperInterface : QVariantValueHelper<T>
    {
    };

    template<>
    struct QVariantValueHelperInterface<QSequentialIterable>
    {
        static QSequentialIterable invoke(const QVariant &v)
        {
            const int typeId = v.userType();
            if (typeId == qMetaTypeId<QVariantList>()) {
                return QSequentialIterable(QtMetaTypePrivate::QSequentialIterableImpl(reinterpret_cast<const QVariantList*>(v.constData())));
            }
            if (typeId == qMetaTypeId<QStringList>()) {
                return QSequentialIterable(QtMetaTypePrivate::QSequentialIterableImpl(reinterpret_cast<const QStringList*>(v.constData())));
            }
#ifndef QT_BOOTSTRAPPED
            if (typeId == qMetaTypeId<QByteArrayList>()) {
                return QSequentialIterable(QtMetaTypePrivate::QSequentialIterableImpl(reinterpret_cast<const QByteArrayList*>(v.constData())));
            }
#endif
            return QSequentialIterable(qvariant_cast<QtMetaTypePrivate::QSequentialIterableImpl>(v));
        }
    };
    template<>
    struct QVariantValueHelperInterface<QAssociativeIterable>
    {
        static QAssociativeIterable invoke(const QVariant &v)
        {
            const int typeId = v.userType();
            if (typeId == qMetaTypeId<QVariantMap>()) {
                return QAssociativeIterable(QtMetaTypePrivate::QAssociativeIterableImpl(reinterpret_cast<const QVariantMap*>(v.constData())));
            }
            if (typeId == qMetaTypeId<QVariantHash>()) {
                return QAssociativeIterable(QtMetaTypePrivate::QAssociativeIterableImpl(reinterpret_cast<const QVariantHash*>(v.constData())));
            }
            return QAssociativeIterable(qvariant_cast<QtMetaTypePrivate::QAssociativeIterableImpl>(v));
        }
    };
    template<>
    struct QVariantValueHelperInterface<QVariantList>
    {
        static QVariantList invoke(const QVariant &v)
        {
            const int typeId = v.userType();
            if (typeId == qMetaTypeId<QStringList>() || typeId == qMetaTypeId<QByteArrayList>() ||
                (QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QtMetaTypePrivate::QSequentialIterableImpl>()) && !QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QVariantList>()))) {
                QSequentialIterable iter = QVariantValueHelperInterface<QSequentialIterable>::invoke(v);
                QVariantList l;
                l.reserve(iter.size());
                for (QSequentialIterable::const_iterator it = iter.begin(), end = iter.end(); it != end; ++it)
                    l << *it;
                return l;
            }
            return QVariantValueHelper<QVariantList>::invoke(v);
        }
    };
    template<>
    struct QVariantValueHelperInterface<QVariantHash>
    {
        static QVariantHash invoke(const QVariant &v)
        {
            const int typeId = v.userType();
            if (typeId == qMetaTypeId<QVariantMap>() || ((QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QtMetaTypePrivate::QAssociativeIterableImpl>())) && !QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QVariantHash>()))) {
                QAssociativeIterable iter = QVariantValueHelperInterface<QAssociativeIterable>::invoke(v);
                QVariantHash l;
                l.reserve(iter.size());
                for (QAssociativeIterable::const_iterator it = iter.begin(), end = iter.end(); it != end; ++it)
                    l.insert(it.key().toString(), it.value());
                return l;
            }
            return QVariantValueHelper<QVariantHash>::invoke(v);
        }
    };
    template<>
    struct QVariantValueHelperInterface<QVariantMap>
    {
        static QVariantMap invoke(const QVariant &v)
        {
            const int typeId = v.userType();
            if (typeId == qMetaTypeId<QVariantHash>() || (QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QtMetaTypePrivate::QAssociativeIterableImpl>()) && !QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QVariantMap>()))) {
                QAssociativeIterable iter = QVariantValueHelperInterface<QAssociativeIterable>::invoke(v);
                QVariantMap l;
                for (QAssociativeIterable::const_iterator it = iter.begin(), end = iter.end(); it != end; ++it)
                    l.insert(it.key().toString(), it.value());
                return l;
            }
            return QVariantValueHelper<QVariantMap>::invoke(v);
        }
    };
    template<>
    struct QVariantValueHelperInterface<QPair<QVariant, QVariant> >
    {
        static QPair<QVariant, QVariant> invoke(const QVariant &v)
        {
            const int typeId = v.userType();

            if (QMetaType::hasRegisteredConverterFunction(typeId, qMetaTypeId<QtMetaTypePrivate::QPairVariantInterfaceImpl>()) && !(typeId == qMetaTypeId<QPair<QVariant, QVariant> >())) {
                QtMetaTypePrivate::QPairVariantInterfaceImpl pi = v.value<QtMetaTypePrivate::QPairVariantInterfaceImpl>();
                QVariant v1(pi._metaType_first);
                void *dataPtr;
                if (pi._metaType_first == QMetaType::fromType<QVariant>())
                    dataPtr = &v1;
                else
                    dataPtr = v1.data();
                pi.first(dataPtr);

                QVariant v2(pi._metaType_second);
                if (pi._metaType_second == QMetaType::fromType<QVariant>())
                    dataPtr = &v2;
                else
                    dataPtr = v2.data();
                pi.second(dataPtr);

                return QPair<QVariant, QVariant>(v1, v2);
            }
            return QVariantValueHelper<QPair<QVariant, QVariant> >::invoke(v);
        }
    };
}

template<typename T> inline T qvariant_cast(const QVariant &v)
{
    return QtPrivate::QVariantValueHelperInterface<T>::invoke(v);
}

template<> inline QVariant qvariant_cast<QVariant>(const QVariant &v)
{
    if (v.userType() == QMetaType::QVariant)
        return *reinterpret_cast<const QVariant *>(v.constData());
    return v;
}

#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant &);
Q_CORE_EXPORT QDebug operator<<(QDebug, const QVariant::Type);
#endif

QT_END_NAMESPACE

#endif // QVARIANT_H
