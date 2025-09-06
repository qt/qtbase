// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvariant_p.h"

#include "private/qlocale_p.h"
#include "qmetatype_p.h"

#if QT_CONFIG(itemmodel)
#include "qabstractitemmodel.h"
#endif
#include "qbitarray.h"
#include "qbytearray.h"
#include "qbytearraylist.h"
#include "qcborarray.h"
#include "qcborcommon.h"
#include "qcbormap.h"
#include "qdatastream.h"
#include "qdatetime.h"
#include "qdebug.h"
#if QT_CONFIG(easingcurve)
#include "qeasingcurve.h"
#endif
#include "qhash.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qjsonobject.h"
#include "qjsonvalue.h"
#include "qline.h"
#include "qlist.h"
#include "qlocale.h"
#include "qmap.h"
#include "qpoint.h"
#include "qrect.h"
#if QT_CONFIG(regularexpression)
#include "qregularexpression.h"
#endif
#include "qsize.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "quuid.h"

#include <memory>
#include <cmath>
#include <cstring>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace { // anonymous used to hide QVariant handlers

static qlonglong qMetaTypeNumberBySize(const QVariant::Private *d)
{
    switch (d->typeInterface()->size) {
    case 1:
        return d->get<signed char>();
    case 2:
        return d->get<short>();
    case 4:
        return d->get<int>();
    case 8:
        return d->get<qlonglong>();
    }
    Q_UNREACHABLE_RETURN(0);
}

static qlonglong qMetaTypeNumber(const QVariant::Private *d)
{
    switch (d->typeInterface()->typeId) {
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
        return qMetaTypeNumberBySize(d);
    case QMetaType::Float:
        return qRound64(d->get<float>());
    case QMetaType::Double:
        return qRound64(d->get<double>());
    case QMetaType::QJsonValue:
        return d->get<QJsonValue>().toDouble();
    case QMetaType::QCborValue:
        return d->get<QCborValue>().toInteger();
    }
    Q_UNREACHABLE_RETURN(0);
}

static qulonglong qMetaTypeUNumber(const QVariant::Private *d)
{
    switch (d->typeInterface()->size) {
    case 1:
        return d->get<unsigned char>();
    case 2:
        return d->get<unsigned short>();
    case 4:
        return d->get<unsigned int>();
    case 8:
        return d->get<qulonglong>();
    }
    Q_UNREACHABLE_RETURN(0);
}

static std::optional<qlonglong> qConvertToNumber(const QVariant::Private *d, bool allowStringToBool = false)
{
    bool ok;
    switch (d->typeInterface()->typeId) {
    case QMetaType::QString: {
        const QString &s = d->get<QString>();
        if (qlonglong l = s.toLongLong(&ok); ok)
            return l;
        if (allowStringToBool) {
            if (s == "false"_L1 || s == "0"_L1)
                return 0;
            if (s == "true"_L1 || s == "1"_L1)
                return 1;
        }
        return std::nullopt;
    }
    case QMetaType::QChar:
        return d->get<QChar>().unicode();
    case QMetaType::QByteArray:
        if (qlonglong l = d->get<QByteArray>().toLongLong(&ok); ok)
            return l;
        return std::nullopt;
    case QMetaType::Bool:
        return qlonglong(d->get<bool>());
    case QMetaType::QCborValue:
        if (!d->get<QCborValue>().isInteger() && !d->get<QCborValue>().isDouble())
            break;
        return qMetaTypeNumber(d);
    case QMetaType::QJsonValue:
        if (!d->get<QJsonValue>().isDouble())
            break;
        Q_FALLTHROUGH();
    case QMetaType::Double:
    case QMetaType::Int:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
        return qMetaTypeNumber(d);
    case QMetaType::ULongLong:
    case QMetaType::UInt:
    case QMetaType::UChar:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qlonglong(qMetaTypeUNumber(d));
    }

    if (d->typeInterface()->flags & QMetaType::IsEnumeration
        || d->typeInterface()->typeId == QMetaType::QCborSimpleType)
        return qMetaTypeNumberBySize(d);

    return std::nullopt;
}

static std::optional<double> qConvertToRealNumber(const QVariant::Private *d)
{
    bool ok;
    switch (d->typeInterface()->typeId) {
    case QMetaType::QString:
        if (double r = d->get<QString>().toDouble(&ok); ok)
            return r;
        return std::nullopt;
    case QMetaType::Double:
        return d->get<double>();
    case QMetaType::Float:
        return double(d->get<float>());
    case QMetaType::Float16:
        return double(d->get<qfloat16>());
    case QMetaType::ULongLong:
    case QMetaType::UInt:
    case QMetaType::UChar:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return double(qMetaTypeUNumber(d));
    case QMetaType::QCborValue:
        return d->get<QCborValue>().toDouble();
    case QMetaType::QJsonValue:
        return d->get<QJsonValue>().toDouble();
    default:
        // includes enum conversion as well as invalid types
        if (std::optional<qlonglong> l = qConvertToNumber(d))
            return double(*l);
        return std::nullopt;
    }
}

static bool isValidMetaTypeForVariant(const QtPrivate::QMetaTypeInterface *iface, const void *copy)
{
    using namespace QtMetaTypePrivate;
    if (!iface || iface->size == 0)
        return false;

    Q_ASSERT(!isInterfaceFor<void>(iface));  // only void should have size 0
    if (!isCopyConstructible(iface) || !isDestructible(iface)) {
        // all meta types must be copyable (because QVariant is) and
        // destructible (because QVariant owns it)
        qWarning("QVariant: Provided metatype for '%s' does not support destruction and "
                 "copy construction", iface->name);
        return false;
    }
    if (!copy && !isDefaultConstructible(iface)) {
        // non-default-constructible types are acceptable, but not if you're
        // asking us to construct from nothing
        qWarning("QVariant: Cannot create type '%s' without a default constructor", iface->name);
        return false;
    }

    return true;
}

enum CustomConstructMoveOptions {
    UseCopy,  // custom construct uses the copy ctor unconditionally
    // future option: TryMove: uses move ctor if available, else copy ctor
    ForceMove, // custom construct use the move ctor (which must exist)
};

enum CustomConstructNullabilityOption {
    MaybeNull, // copy might be null, might be non-null
    NonNull, // copy is guarantueed to be non-null
    // future option: AlwaysNull?
};

// the type of d has already been set, but other field are not set
template <CustomConstructMoveOptions moveOption = UseCopy, CustomConstructNullabilityOption nullability = MaybeNull>
static void customConstruct(const QtPrivate::QMetaTypeInterface *iface, QVariant::Private *d,
                            std::conditional_t<moveOption == ForceMove, void *, const void *> copy)
{
    using namespace QtMetaTypePrivate;
    Q_ASSERT(iface);
    Q_ASSERT(iface->size);
    Q_ASSERT(!isInterfaceFor<void>(iface));
    Q_ASSERT(isCopyConstructible(iface));
    Q_ASSERT(isDestructible(iface));
    Q_ASSERT(copy || isDefaultConstructible(iface));
    if constexpr (moveOption == ForceMove)
        Q_ASSERT(isMoveConstructible(iface));
    if constexpr (nullability == NonNull)
        Q_ASSERT(copy != nullptr);

    // need to check for nullptr_t here, as this can get called by fromValue(nullptr). fromValue() uses
    // std::addressof(value) which in this case returns the address of the nullptr object.
    // ### Qt 7: remove nullptr_t special casing
    d->is_null = !copy QT6_ONLY(|| isInterfaceFor<std::nullptr_t>(iface));

    if (QVariant::Private::canUseInternalSpace(iface)) {
        d->is_shared = false;
        if (!copy && !iface->defaultCtr)
            return;     // trivial default constructor and it's OK to build in 0-filled storage, which we've already done
        if constexpr (moveOption == ForceMove && nullability == NonNull)
            moveConstruct(iface, d->data.data, copy);
        else
            construct(iface, d->data.data, copy);
    } else {
        d->data.shared = customConstructShared(iface->size, iface->alignment, [=](void *where) {
            if constexpr (moveOption == ForceMove && nullability == NonNull)
                moveConstruct(iface, where, copy);
            else
                construct(iface, where, copy);
        });
        d->is_shared = true;
    }
}

static void customClear(QVariant::Private *d)
{
    const QtPrivate::QMetaTypeInterface *iface = d->typeInterface();
    if (!iface)
        return;
    if (!d->is_shared) {
        QtMetaTypePrivate::destruct(iface, d->data.data);
    } else {
        QtMetaTypePrivate::destruct(iface, d->data.shared->data());
        QVariant::PrivateShared::free(d->data.shared);
    }
}

static QVariant::Private clonePrivate(const QVariant::Private &other)
{
    QVariant::Private d = other;
    if (d.is_shared) {
        d.data.shared->ref.ref();
    } else if (const QtPrivate::QMetaTypeInterface *iface = d.typeInterface()) {
        if (Q_LIKELY(d.canUseInternalSpace(iface))) {
            // if not trivially copyable, ask to copy (if it's trivially
            // copyable, we've already copied it)
            if (iface->copyCtr)
                QtMetaTypePrivate::copyConstruct(iface, d.data.data, other.data.data);
        } else {
            // highly unlikely, but possible case: type has changed relocatability
            // between builds
            d.data.shared = QVariant::PrivateShared::create(iface->size, iface->alignment);
            QtMetaTypePrivate::copyConstruct(iface, d.data.shared->data(), other.data.data);
        }

    }
    return d;
}

} // anonymous used to hide QVariant handlers

/*!
    \class QVariant
    \inmodule QtCore
    \brief The QVariant class acts like a union for the most common Qt data types.

    \ingroup objectmodel
    \ingroup shared

    \compares equality

    A QVariant object holds a single value of a single typeId() at a
    time. (Some types are multi-valued, for example a string list.)
    You can find out what type, T, the variant holds, convert it to a
    different type using convert(), get its value using one of the
    toT() functions (e.g., toSize()), and check whether the type can
    be converted to a particular type using canConvert().

    The methods named toT() (e.g., toInt(), toString()) are const. If
    you ask for the stored type, they return a copy of the stored
    object. If you ask for a type that can be generated from the
    stored type, toT() copies and converts and leaves the object
    itself unchanged. If you ask for a type that cannot be generated
    from the stored type, the result depends on the type; see the
    function documentation for details.

    Here is some example code to demonstrate the use of QVariant:

    \snippet code/src_corelib_kernel_qvariant.cpp 0

    You can even store QList<QVariant> and QMap<QString, QVariant>
    values in a variant, so you can easily construct arbitrarily
    complex data structures of arbitrary types. This is very powerful
    and versatile, but may prove less memory and speed efficient than
    storing specific types in standard data structures.

    QVariant also supports the notion of null values. A variant is null
    if the variant contains no initialized value, or contains a null pointer.

    \snippet code/src_corelib_kernel_qvariant.cpp 1

    QVariant can be extended to support other types than those
    mentioned in the \l QMetaType::Type enum.
    See \l{Creating Custom Qt Types}{Creating Custom Qt Types} for details.

    \section1 A Note on GUI Types

    Because QVariant is part of the Qt Core module, it cannot provide
    conversion functions to data types defined in Qt GUI, such as
    QColor, QImage, and QPixmap. In other words, there is no \c
    toColor() function. Instead, you can use the QVariant::value() or
    the qvariant_cast() template function. For example:

    \snippet code/src_corelib_kernel_qvariant.cpp 2

    The inverse conversion (e.g., from QColor to QVariant) is
    automatic for all data types supported by QVariant, including
    GUI-related types:

    \snippet code/src_corelib_kernel_qvariant.cpp 3

    \section1 Using canConvert() and convert() Consecutively

    When using canConvert() and convert() consecutively, it is possible for
    canConvert() to return true, but convert() to return false. This
    is typically because canConvert() only reports the general ability of
    QVariant to convert between types given suitable data; it is still
    possible to supply data which cannot actually be converted.

    For example, \c{canConvert(QMetaType::fromType<int>())} would return true
    when called on a variant containing a string because, in principle,
    QVariant is able to convert strings of numbers to integers.
    However, if the string contains non-numeric characters, it cannot be
    converted to an integer, and any attempt to convert it will fail.
    Hence, it is important to have both functions return true for a
    successful conversion.

    \sa QMetaType
*/

/*!
    \deprecated Use \l QMetaType::Type instead.
    \enum QVariant::Type

    This enum type defines the types of variable that a QVariant can
    contain.

    \value Invalid  no type
    \value BitArray  a QBitArray
    \value Bitmap  a QBitmap
    \value Bool  a bool
    \value Brush  a QBrush
    \value ByteArray  a QByteArray
    \value Char  a QChar
    \value Color  a QColor
    \value Cursor  a QCursor
    \value Date  a QDate
    \value DateTime  a QDateTime
    \value Double  a double
    \value EasingCurve a QEasingCurve
    \value Uuid a QUuid
    \value ModelIndex a QModelIndex
    \value [since 5.5] PersistentModelIndex a QPersistentModelIndex
    \value Font  a QFont
    \value Hash a QVariantHash
    \value Icon  a QIcon
    \value Image  a QImage
    \value Int  an int
    \value KeySequence  a QKeySequence
    \value Line  a QLine
    \value LineF  a QLineF
    \value List  a QVariantList
    \value Locale  a QLocale
    \value LongLong a \l qlonglong
    \value Map  a QVariantMap
    \value Transform  a QTransform
    \value Matrix4x4  a QMatrix4x4
    \value Palette  a QPalette
    \value Pen  a QPen
    \value Pixmap  a QPixmap
    \value Point  a QPoint
    \value PointF  a QPointF
    \value Polygon a QPolygon
    \value PolygonF a QPolygonF
    \value Quaternion  a QQuaternion
    \value Rect  a QRect
    \value RectF  a QRectF
    \value RegularExpression  a QRegularExpression
    \value Region  a QRegion
    \value Size  a QSize
    \value SizeF  a QSizeF
    \value SizePolicy  a QSizePolicy
    \value String  a QString
    \value StringList  a QStringList
    \value TextFormat  a QTextFormat
    \value TextLength  a QTextLength
    \value Time  a QTime
    \value UInt  a \l uint
    \value ULongLong a \l qulonglong
    \value Url  a QUrl
    \value Vector2D  a QVector2D
    \value Vector3D  a QVector3D
    \value Vector4D  a QVector4D

    \value UserType Base value for user-defined types.

    \omitvalue LastGuiType
    \omitvalue LastCoreType
    \omitvalue LastType
*/

/*!
    \fn QVariant::QVariant(QVariant &&other)

    Move-constructs a QVariant instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    \fn QVariant &QVariant::operator=(QVariant &&other)

    Move-assigns \a other to this QVariant instance.

    \since 5.2
*/

/*!
    \fn QVariant::QVariant()

    Constructs an invalid variant.
*/

/*!
    \fn QVariant::create(int type, const void *copy)

    \internal

    Constructs a variant private of type \a type, and initializes with \a copy if
    \a copy is not \nullptr.

*/
//### Qt 7: Remove in favor of QMetaType overload
void QVariant::create(int type, const void *copy)
{
    create(QMetaType(type), copy);
}

/*!
    \fn QVariant::create(int type, const void *copy)

    \internal
    \overload
*/
void QVariant::create(QMetaType type, const void *copy)
{
    *this = QVariant::fromMetaType(type, copy);
}

/*!
    \fn QVariant::~QVariant()

    Destroys the QVariant and the contained object.
*/

QVariant::~QVariant()
{
    if (!d.is_shared || !d.data.shared->ref.deref())
        customClear(&d);
}

/*!
  \fn QVariant::QVariant(const QVariant &p)

    Constructs a copy of the variant, \a p, passed as the argument to
    this constructor.
*/

QVariant::QVariant(const QVariant &p)
    : d(clonePrivate(p.d))
{
}

/*!
    \fn template <typename T, typename... Args, QVariant::if_constructible<T, Args...> = true> QVariant::QVariant(std::in_place_type_t<T>, Args&&... args) noexcept(is_noexcept_constructible<q20::remove_cvref_t<T>, Args...>::value)

    \since 6.6
    Constructs a new variant containing a value of type \c T. The contained
    value is initialized with the arguments
    \c{std::forward<Args>(args)...}.

    This constructor is provided for STL/std::any compatibility.

    \overload

    \constraints \c T can be constructed from \a args.
 */

/*!

    \fn template <typename T, typename U, typename... Args, QVariant::if_constructible<T, std::initializer_list<U> &, Args...> = true> explicit QVariant::QVariant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) noexcept(is_noexcept_constructible<q20::remove_cvref_t<T>, std::initializer_list<U> &, Args... >::value)

    \since 6.6
    \overload
    This overload exists to support types with constructors taking an
    \c initializer_list. It behaves otherwise equivalent to the
    non-initializer list \c{in_place_type_t} overload.
*/


/*!
    \fn template <typename T, typename... Args, QVariant::if_constructible<T, Args...> = true> QVariant::emplace(Args&&... args)

    \since 6.6
    Replaces the object currently held in \c{*this} with an object of
    type \c{T}, constructed from \a{args}\c{...}. If \c{*this} was non-null,
    the previously held object is destroyed first.
    If possible, this method will reuse memory allocated by the QVariant.
    Returns a reference to the newly-created object.
 */

/*!
    \fn template <typename T, typename U, typename... Args, QVariant::if_constructible<T, std::initializer_list<U> &, Args...> = true> QVariant::emplace(std::initializer_list<U> list, Args&&... args)

    \since 6.6
    \overload
    This overload exists to support types with constructors taking an
    \c initializer_list. It behaves otherwise equivalent to the
    non-initializer list overload.
*/

QVariant::QVariant(std::in_place_t, QMetaType type) : d(type.iface())
{
    // we query the metatype instead of detecting it at compile time
    // so that we can change relocatability of internal types
    if (!Private::canUseInternalSpace(type.iface())) {
        d.data.shared = PrivateShared::create(type.sizeOf(), type.alignOf());
        d.is_shared = true;
    }
}

/*!
    \internal
    Returns a pointer to data suitable for placement new
    of an object of type \a type
    Changes the variant's metatype to \a type
 */
void *QVariant::prepareForEmplace(QMetaType type)
{
    /* There are two cases where we can reuse the existing storage
       (1) The new type fits in QVariant's SBO storage
       (2) We are using the externally allocated storage, the variant is
           detached, and the new type fits into the existing storage.
       In all other cases (3), we cannot reuse the storage.
     */
    auto typeFits = [&] {
        auto newIface = type.iface();
        auto oldIface = d.typeInterface();
        auto newSize = PrivateShared::computeAllocationSize(newIface->size, newIface->alignment);
        auto oldSize = PrivateShared::computeAllocationSize(oldIface->size, oldIface->alignment);
        return newSize <= oldSize;
    };
    if (Private::canUseInternalSpace(type.iface())) { // (1)
        clear();
        d.packedType = quintptr(type.iface()) >> 2;
        return d.data.data;
    } else if (d.is_shared && isDetached() && typeFits()) { // (2)
        QtMetaTypePrivate::destruct(d.typeInterface(), d.data.shared->data());
        // compare QVariant::PrivateShared::create
        const auto ps = d.data.shared;
        const auto align = type.alignOf();
        ps->offset =  PrivateShared::computeOffset(ps, align);
        d.packedType = quintptr(type.iface()) >> 2;
        return ps->data();
    }
    // (3)
    QVariant newVariant(std::in_place, type);
    swap(newVariant);
    // const cast is safe, we're in a non-const method
    return const_cast<void *>(d.storage());
}

/*!
  \fn QVariant::QVariant(const QString &val) noexcept

    Constructs a new variant with a string value, \a val.
*/

/*!
    \fn QVariant::QVariant(QLatin1StringView val)

    Constructs a new variant with a QString value from the Latin-1
    string viewed by \a val.
*/

/*!
  \fn QVariant::QVariant(const char *val)

    Constructs a new variant with a string value of \a val.
    The variant creates a deep copy of \a val into a QString assuming
    UTF-8 encoding on the input \a val.

    Note that \a val is converted to a QString for storing in the
    variant and QVariant::userType() will return QMetaType::QString for
    the variant.

    You can disable this operator by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications.
*/

/*!
  \fn QVariant::QVariant(const QStringList &val) noexcept

    Constructs a new variant with a string list value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QMap<QString, QVariant> &val) noexcept

    Constructs a new variant with a map of \l {QVariant}s, \a val.
*/

/*!
  \fn QVariant::QVariant(const QHash<QString, QVariant> &val) noexcept

    Constructs a new variant with a hash of \l {QVariant}s, \a val.
*/

/*!
  \fn QVariant::QVariant(QDate val) noexcept

    Constructs a new variant with a date value, \a val.
*/

/*!
  \fn QVariant::QVariant(QTime val) noexcept

    Constructs a new variant with a time value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QDateTime &val) noexcept

    Constructs a new variant with a date/time value, \a val.
*/

/*!
    \since 4.7
  \fn QVariant::QVariant(const QEasingCurve &val)

    Constructs a new variant with an easing curve value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(QUuid val) noexcept

    Constructs a new variant with an uuid value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QModelIndex &val) noexcept

    Constructs a new variant with a QModelIndex value, \a val.
*/

/*!
    \since 5.5
    \fn QVariant::QVariant(const QPersistentModelIndex &val)

    Constructs a new variant with a QPersistentModelIndex value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonValue &val)

    Constructs a new variant with a json value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonObject &val)

    Constructs a new variant with a json object value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonArray &val)

    Constructs a new variant with a json array value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QJsonDocument &val)

    Constructs a new variant with a json document value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QByteArray &val) noexcept

    Constructs a new variant with a bytearray value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QBitArray &val) noexcept

    Constructs a new variant with a bitarray value, \a val.
*/

/*!
  \fn QVariant::QVariant(QPoint val) noexcept

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(QPointF val) noexcept

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(QRectF val)

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(QLineF val) noexcept

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(QLine val) noexcept

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(QRect val) noexcept

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(QSize val) noexcept

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(QSizeF val) noexcept

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QUrl &val) noexcept

  Constructs a new variant with a url value of \a val.
 */

/*!
  \fn QVariant::QVariant(int val) noexcept

    Constructs a new variant with an integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(uint val) noexcept

    Constructs a new variant with an unsigned integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qlonglong val) noexcept

    Constructs a new variant with a long long integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qulonglong val) noexcept

    Constructs a new variant with an unsigned long long integer value, \a val.
*/


/*!
  \fn QVariant::QVariant(bool val) noexcept

    Constructs a new variant with a boolean value, \a val.
*/

/*!
  \fn QVariant::QVariant(double val) noexcept

    Constructs a new variant with a floating point value, \a val.
*/

/*!
  \fn QVariant::QVariant(float val) noexcept

    Constructs a new variant with a floating point value, \a val.
    \since 4.6
*/

/*!
    \fn QVariant::QVariant(const QList<QVariant> &val) noexcept

    Constructs a new variant with a list value, \a val.
*/

/*!
  \fn QVariant::QVariant(QChar c) noexcept

  Constructs a new variant with a char value, \a c.
*/

/*!
  \fn QVariant::QVariant(const QLocale &l) noexcept

  Constructs a new variant with a locale value, \a l.
*/

/*!
  \fn QVariant::QVariant(const QRegularExpression &re) noexcept

  \since 5.0

  Constructs a new variant with the regular expression value \a re.
*/

/*! \fn QVariant::QVariant(Type type)
    \deprecated [6.0] Use the constructor taking a QMetaType instead.

    Constructs an uninitialized variant of type \a type. This will create a
    variant in a special null state that if accessed will return a default
    constructed value of the \a type.

    \sa isNull()
*/

/*!
    Constructs a variant of type \a type, and initializes it with
    a copy of \c{*copy} if \a copy is not \nullptr (in which case, \a copy
    must point to an object of type \a type).

    Note that you have to pass the address of the object you want stored.

    Usually, you never have to use this constructor, use QVariant::fromValue()
    instead to construct variants from the pointer types represented by
    \c QMetaType::VoidStar, and \c QMetaType::QObjectStar.

    If \a type does not support copy construction and \a copy is not \nullptr,
    the variant will be invalid. Similarly, if \a copy is \nullptr and
    \a type does not support default construction, the variant will be
    invalid.

    \sa QVariant::fromMetaType, QVariant::fromValue(), QMetaType::Type
*/
QVariant::QVariant(QMetaType type, const void *copy)
    : QVariant(fromMetaType(type, copy))
{
}

QVariant::QVariant(int val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(uint val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(qlonglong val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(qulonglong val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(bool val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(double val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(float val) noexcept : d(std::piecewise_construct_t{}, val) {}

QVariant::QVariant(const QByteArray &val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(const QBitArray &val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(const QString &val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(QChar val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(const QStringList &val) noexcept : d(std::piecewise_construct_t{}, val) {}

QVariant::QVariant(QDate val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(QTime val) noexcept : d(std::piecewise_construct_t{}, val) {}
QVariant::QVariant(const QDateTime &val) noexcept : d(std::piecewise_construct_t{}, val) {}

QVariant::QVariant(const QList<QVariant> &list) noexcept : d(std::piecewise_construct_t{}, list) {}
QVariant::QVariant(const QMap<QString, QVariant> &map) noexcept : d(std::piecewise_construct_t{}, map) {}
QVariant::QVariant(const QHash<QString, QVariant> &hash) noexcept : d(std::piecewise_construct_t{}, hash) {}

QVariant::QVariant(QLatin1StringView val) : QVariant(QString(val)) {}

#if QT_CONFIG(easingcurve)
QVariant::QVariant(const QEasingCurve &val) : d(std::piecewise_construct_t{}, val) {}
#endif
QVariant::QVariant(QPoint pt) noexcept
    : d(std::piecewise_construct_t{}, pt) {}
QVariant::QVariant(QPointF pt) noexcept(Private::FitsInInternalSize<sizeof(qreal) * 2>)
    : d(std::piecewise_construct_t{}, pt) {}
QVariant::QVariant(QRect r) noexcept(Private::FitsInInternalSize<sizeof(int) * 4>)
    : d(std::piecewise_construct_t{}, r) {}
QVariant::QVariant(QRectF r) noexcept(Private::FitsInInternalSize<sizeof(qreal) * 4>)
    : d(std::piecewise_construct_t{}, r) {}
QVariant::QVariant(QLine l) noexcept(Private::FitsInInternalSize<sizeof(int) * 4>)
    : d(std::piecewise_construct_t{}, l) {}
QVariant::QVariant(QLineF l) noexcept(Private::FitsInInternalSize<sizeof(qreal) * 4>)
    : d(std::piecewise_construct_t{}, l) {}
QVariant::QVariant(QSize s) noexcept
    : d(std::piecewise_construct_t{}, s) {}
QVariant::QVariant(QSizeF s) noexcept(Private::FitsInInternalSize<sizeof(qreal) * 2>)
    : d(std::piecewise_construct_t{}, s) {}
QVariant::QVariant(const QUrl &u) noexcept : d(std::piecewise_construct_t{}, u) {}
QVariant::QVariant(const QLocale &l) noexcept : d(std::piecewise_construct_t{}, l) {}
#if QT_CONFIG(regularexpression)
QVariant::QVariant(const QRegularExpression &re) noexcept : d(std::piecewise_construct_t{}, re) {}
#endif // QT_CONFIG(regularexpression)
QVariant::QVariant(QUuid uuid) noexcept(Private::FitsInInternalSize<16>) : d(std::piecewise_construct_t{}, uuid) {}
QVariant::QVariant(const QJsonValue &jsonValue) noexcept(Private::FitsInInternalSize<sizeof(CborValueStandIn)>)
    : d(std::piecewise_construct_t{}, jsonValue)
{ static_assert(sizeof(CborValueStandIn) == sizeof(QJsonValue)); }
QVariant::QVariant(const QJsonObject &jsonObject) noexcept : d(std::piecewise_construct_t{}, jsonObject) {}
QVariant::QVariant(const QJsonArray &jsonArray) noexcept : d(std::piecewise_construct_t{}, jsonArray) {}
QVariant::QVariant(const QJsonDocument &jsonDocument) : d(std::piecewise_construct_t{}, jsonDocument) {}
#if QT_CONFIG(itemmodel)
QVariant::QVariant(const QModelIndex &modelIndex) noexcept(Private::FitsInInternalSize<8 + 2 * sizeof(quintptr)>)
    : d(std::piecewise_construct_t{}, modelIndex) {}
QVariant::QVariant(const QPersistentModelIndex &modelIndex) : d(std::piecewise_construct_t{}, modelIndex) {}
#endif

/*! \fn QVariant::Type QVariant::type() const
    \deprecated [6.0] Use typeId() or metaType() instead.

    Returns the storage type of the value stored in the variant.
    Although this function is declared as returning QVariant::Type,
    the return value should be interpreted as QMetaType::Type. In
    particular, QVariant::UserType is returned here only if the value
    is equal or greater than QMetaType::User.

    Note that return values in the ranges QVariant::Char through
    QVariant::RegExp and QVariant::Font through QVariant::Transform
    correspond to the values in the ranges QMetaType::QChar through
    QMetaType::QRegularExpression and QMetaType::QFont through QMetaType::QQuaternion.

    Pay particular attention when working with char and QChar
    variants.  Note that there is no QVariant constructor specifically
    for type char, but there is one for QChar. For a variant of type
    QChar, this function returns QVariant::Char, which is the same as
    QMetaType::QChar, but for a variant of type \c char, this function
    returns QMetaType::Char, which is \e not the same as
    QVariant::Char.

    Also note that the types \c void*, \c long, \c short, \c unsigned
    \c long, \c unsigned \c short, \c unsigned \c char, \c float, \c
    QObject*, and \c QWidget* are represented in QMetaType::Type but
    not in QVariant::Type, and they can be returned by this function.
    However, they are considered to be user defined types when tested
    against QVariant::Type.

    To test whether an instance of QVariant contains a data type that
    is compatible with the data type you are interested in, use
    canConvert().

    \sa userType(), metaType()
*/

/*! \fn int QVariant::userType() const
    \fn int QVariant::typeId() const

    Returns the storage type of the value stored in the variant. This is
    the same as metaType().id().

    \sa metaType()
*/

/*!
    \fn QMetaType QVariant::metaType() const
    \since 6.0

    Returns the QMetaType of the value stored in the variant.
*/

/*!
    Assigns the value of the variant \a variant to this variant.
*/
QVariant &QVariant::operator=(const QVariant &variant)
{
    if (this == &variant)
        return *this;

    clear();
    d = clonePrivate(variant.d);
    return *this;
}

/*!
    \fn void QVariant::swap(QVariant &other)
    \since 4.8
    \memberswap{variant}
*/

/*!
    \fn void QVariant::detach()

    \internal
*/

void QVariant::detach()
{
    if (!d.is_shared || d.data.shared->ref.loadRelaxed() == 1)
        return;

    Q_ASSERT(isValidMetaTypeForVariant(d.typeInterface(), constData()));
    Private dd(d.typeInterface());
    // null variant is never shared; anything else is NonNull
    customConstruct<UseCopy, NonNull>(d.typeInterface(), &dd, constData());
    if (!d.data.shared->ref.deref())
        customClear(&d);
    d.data.shared = dd.data.shared;
}

/*!
    \fn bool QVariant::isDetached() const

    \internal
*/

/*!
    \fn const char *QVariant::typeName() const

    Returns the name of the type stored in the variant. The returned
    strings describe the C++ datatype used to store the data: for
    example, "QFont", "QString", or "QVariantList". An Invalid
    variant returns 0.
*/

/*!
    Convert this variant to type QMetaType::UnknownType and free up any resources
    used.
*/
void QVariant::clear()
{
    if (!d.is_shared || !d.data.shared->ref.deref())
        customClear(&d);
    d = {};
}

/*!
    \fn const char *QVariant::typeToName(int typeId)
    \deprecated [6.0] Use \c QMetaType(typeId).name() instead.

    Converts the int representation of the storage type, \a typeId, to
    its string representation.

    Returns \nullptr if the type is QMetaType::UnknownType or doesn't exist.
*/

/*!
    \fn QVariant::Type QVariant::nameToType(const char *name)
    \deprecated [6.0] Use \c QMetaType::fromName(name).id() instead

    Converts the string representation of the storage type given in \a
    name, to its enum representation.

    If the string representation cannot be converted to any enum
    representation, the variant is set to \c Invalid.
*/

#ifndef QT_NO_DATASTREAM
enum { MapFromThreeCount = 36 };
static const ushort mapIdFromQt3ToCurrent[MapFromThreeCount] =
{
    QMetaType::UnknownType,
    QMetaType::QVariantMap,
    QMetaType::QVariantList,
    QMetaType::QString,
    QMetaType::QStringList,
    QMetaType::QFont,
    QMetaType::QPixmap,
    QMetaType::QBrush,
    QMetaType::QRect,
    QMetaType::QSize,
    QMetaType::QColor,
    QMetaType::QPalette,
    0, // ColorGroup
    QMetaType::QIcon,
    QMetaType::QPoint,
    QMetaType::QImage,
    QMetaType::Int,
    QMetaType::UInt,
    QMetaType::Bool,
    QMetaType::Double,
    0, // Buggy ByteArray, QByteArray never had id == 20
    QMetaType::QPolygon,
    QMetaType::QRegion,
    QMetaType::QBitmap,
    QMetaType::QCursor,
    QMetaType::QSizePolicy,
    QMetaType::QDate,
    QMetaType::QTime,
    QMetaType::QDateTime,
    QMetaType::QByteArray,
    QMetaType::QBitArray,
#if QT_CONFIG(shortcut)
    QMetaType::QKeySequence,
#else
    0, // QKeySequence
#endif
    QMetaType::QPen,
    QMetaType::LongLong,
    QMetaType::ULongLong,
#if QT_CONFIG(easingcurve)
    QMetaType::QEasingCurve
#endif
};

// values needed to map Qt5 based type id's to Qt6 based ones
constexpr int Qt5UserType = 1024;
constexpr int Qt5LastCoreType = QMetaType::QCborMap;
constexpr int Qt5FirstGuiType = 64;
constexpr int Qt5LastGuiType = 87;
constexpr int Qt5SizePolicy = 121;
constexpr int Qt5RegExp = 27;
constexpr int Qt5KeySequence = 75;
constexpr int Qt5QQuaternion = 85;

constexpr int Qt6ToQt5GuiTypeDelta = qToUnderlying(QMetaType::FirstGuiType) - Qt5FirstGuiType;

/*!
    Internal function for loading a variant from stream \a s. Use the
    stream operators instead.

    \internal
*/
void QVariant::load(QDataStream &s)
{
    clear();

    quint32 typeId;
    s >> typeId;
    if (s.version() < QDataStream::Qt_4_0) {
        // map to Qt 5 ids
        if (typeId >= MapFromThreeCount)
            return;
        typeId = mapIdFromQt3ToCurrent[typeId];
    } else if (s.version() < QDataStream::Qt_5_0) {
        // map to Qt 5 type ids
        if (typeId == 127 /* QVariant::UserType */) {
            typeId = Qt5UserType;
        } else if (typeId >= 128 && typeId != Qt5UserType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId -= 97;
        } else if (typeId == 75 /* QSizePolicy */) {
            typeId = Qt5SizePolicy;
        } else if (typeId > 75 && typeId <= 86) {
            // and as a result these types received lower ids too
            // QKeySequence QPen QTextLength QTextFormat QTransform QMatrix4x4 QVector2D QVector3D QVector4D QQuaternion
            typeId -=1;
        }
    }
    if (s.version() < QDataStream::Qt_6_0) {
        // map from Qt 5 to Qt 6 values
        if (typeId == Qt5UserType) {
            typeId = QMetaType::User;
        } else if (typeId >= Qt5FirstGuiType && typeId <= Qt5LastGuiType) {
            typeId += Qt6ToQt5GuiTypeDelta;
        } else if (typeId == Qt5SizePolicy) {
            typeId = QMetaType::QSizePolicy;
        } else if (typeId == Qt5RegExp) {
            typeId = QMetaType::fromName("QRegExp").id();
        }
    }

    qint8 is_null = false;
    if (s.version() >= QDataStream::Qt_4_2)
        s >> is_null;
    if (typeId == QMetaType::User) {
        QByteArray name;
        s >> name;
        typeId = QMetaType::fromName(name).id();
        if (typeId == QMetaType::UnknownType) {
            s.setStatus(QDataStream::ReadCorruptData);
            qWarning("QVariant::load: unknown user type with name %s.", name.constData());
            return;
        }
    }
    create(typeId, nullptr);
    d.is_null = is_null;

    if (!isValid()) {
        if (s.version() < QDataStream::Qt_5_0) {
            // Since we wrote something, we should read something
            QString x;
            s >> x;
        }
        d.is_null = true;
        return;
    }

    // const cast is safe since we operate on a newly constructed variant
    void *data = const_cast<void *>(constData());
    if (!d.type().load(s, data)) {
        s.setStatus(QDataStream::ReadCorruptData);
        qWarning("QVariant::load: unable to load type %d.", d.type().id());
    }
}

/*!
    Internal function for saving a variant to the stream \a s. Use the
    stream operators instead.

    \internal
*/
void QVariant::save(QDataStream &s) const
{
    quint32 typeId = d.type().id();
    bool saveAsUserType = false;
    if (typeId >= QMetaType::User) {
        typeId = QMetaType::User;
        saveAsUserType = true;
    }
    if (s.version() < QDataStream::Qt_6_0) {
        // map to Qt 5 values
        if (typeId == QMetaType::User) {
            typeId = Qt5UserType;
            if (!strcmp(d.type().name(), "QRegExp")) {
                typeId = 27; // QRegExp in Qt 4/5
            }
        } else if (typeId > Qt5LastCoreType && typeId <= QMetaType::LastCoreType) {
            // the type didn't exist in Qt 5
            typeId = Qt5UserType;
            saveAsUserType = true;
        } else if (typeId >= QMetaType::FirstGuiType && typeId <= QMetaType::LastGuiType) {
            typeId -= Qt6ToQt5GuiTypeDelta;
            if (typeId > Qt5LastGuiType) {
                typeId = Qt5UserType;
                saveAsUserType = true;
            }
        } else if (typeId == QMetaType::QSizePolicy) {
            typeId = Qt5SizePolicy;
        }
    }
    if (s.version() < QDataStream::Qt_4_0) {
        int i;
        for (i = 0; i <= MapFromThreeCount - 1; ++i) {
            if (mapIdFromQt3ToCurrent[i] == typeId) {
                typeId = i;
                break;
            }
        }
        if (i >= MapFromThreeCount) {
            s << QVariant();
            return;
        }
    } else if (s.version() < QDataStream::Qt_5_0) {
        if (typeId == Qt5UserType) {
            typeId = 127; // QVariant::UserType had this value in Qt4
            saveAsUserType = true;
        } else if (typeId >= 128 - 97 && typeId <= Qt5LastCoreType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId += 97;
        } else if (typeId == Qt5SizePolicy) {
            typeId = 75;
        } else if (typeId >= Qt5KeySequence && typeId <= Qt5QQuaternion) {
            // and as a result these types received lower ids too
            typeId += 1;
        } else if (typeId > Qt5QQuaternion || typeId == QMetaType::QUuid) {
            // These existed in Qt 4 only as a custom type
            typeId = 127;
            saveAsUserType = true;
        }
    }
    const char *typeName = nullptr;
    if (saveAsUserType) {
        if (s.version() < QDataStream::Qt_6_0)
            typeName = QtMetaTypePrivate::typedefNameForType(d.type().d_ptr);
        if (!typeName)
            typeName = d.type().name();
    }
    s << typeId;
    if (s.version() >= QDataStream::Qt_4_2)
        s << qint8(d.is_null);
    if (typeName)
        s << typeName;

    if (!isValid()) {
        if (s.version() < QDataStream::Qt_5_0)
            s << QString();
        return;
    }

    if (!d.type().save(s, constData())) {
        qWarning("QVariant::save: unable to save type '%s' (type id: %d).\n",
                 d.type().name(), d.type().id());
        Q_ASSERT_X(false, "QVariant::save", "Invalid type to save");
    }
}

/*!
    \since 4.4
    \relates QVariant

    Reads a variant \a p from the stream \a s.

    \note If the stream contains types that aren't the built-in ones (see \l
    QMetaType::Type), those types must be registered using qRegisterMetaType()
    or QMetaType::registerType() before the variant can be properly loaded. If
    an unregistered type is found, QVariant will set the corrupt flag in the
    stream, stop processing and print a warning. For example, for QList<int>
    it would print the following:

    \quotation
    QVariant::load: unknown user type with name QList<int>
    \endquotation

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator>>(QDataStream &s, QVariant &p)
{
    p.load(s);
    return s;
}

/*!
    Writes a variant \a p to the stream \a s.
    \relates QVariant

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator<<(QDataStream &s, const QVariant &p)
{
    p.save(s);
    return s;
}

/*! \fn QDataStream& operator>>(QDataStream &s, QVariant::Type &p)
    \relates QVariant
    \deprecated [6.0] Stream QMetaType::Type instead.

    Reads a variant type \a p in enum representation from the stream \a s.
*/

/*! \fn QDataStream& operator<<(QDataStream &s, const QVariant::Type p)
    \relates QVariant
    \deprecated [6.0] Stream QMetaType::Type instead.

    Writes a variant type \a p to the stream \a s.
*/
#endif //QT_NO_DATASTREAM

/*!
    \fn bool QVariant::isValid() const

    Returns \c true if the storage type of this variant is not
    QMetaType::UnknownType; otherwise returns \c false.
*/

/*!
    \fn QStringList QVariant::toStringList() const

    Returns the variant as a QStringList if the variant has userType()
    \l QMetaType::QStringList, \l QMetaType::QString, or
    \l QMetaType::QVariantList of a type that can be converted to QString;
    otherwise returns an empty list.

    \sa canConvert(), convert()
*/
QStringList QVariant::toStringList() const
{
    return qvariant_cast<QStringList>(*this);
}

/*!
    Returns the variant as a QString if the variant has a userType()
    including, but not limited to:

    \l QMetaType::QString, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::QDate, \l QMetaType::QDateTime,
    \l QMetaType::Double, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QStringList, \l QMetaType::QTime, \l QMetaType::UInt, or
    \l QMetaType::ULongLong.

    Calling QVariant::toString() on an unsupported variant returns an empty
    string.

    \sa canConvert(), convert()
*/
QString QVariant::toString() const
{
    return qvariant_cast<QString>(*this);
}

/*!
    Returns the variant as a QVariantMap if the variant has type() \l
    QMetaType::QVariantMap. If it doesn't, QVariant will attempt to
    convert the type to a map and then return it. This will succeed for
    any type that has registered a converter to QVariantMap or which was
    declared as a associative container using
    \l{Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE}. If none of those
    conditions are true, this function will return an empty map.

    \sa canConvert(), convert()
*/
QVariantMap QVariant::toMap() const
{
    return qvariant_cast<QVariantMap>(*this);
}

/*!
    Returns the variant as a QHash<QString, QVariant> if the variant
    has type() \l QMetaType::QVariantHash; otherwise returns an empty map.

    \sa canConvert(), convert()
*/
QVariantHash QVariant::toHash() const
{
    return qvariant_cast<QVariantHash>(*this);
}

/*!
    \fn QDate QVariant::toDate() const

    Returns the variant as a QDate if the variant has userType()
    \l QMetaType::QDate, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid date.

    If the type() is \l QMetaType::QString, an invalid date will be returned if
    the string cannot be parsed as a Qt::ISODate format date.

    \sa canConvert(), convert()
*/
QDate QVariant::toDate() const
{
    return qvariant_cast<QDate>(*this);
}

/*!
    \fn QTime QVariant::toTime() const

    Returns the variant as a QTime if the variant has userType()
    \l QMetaType::QTime, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid time.

    If the type() is \l QMetaType::QString, an invalid time will be returned if
    the string cannot be parsed as a Qt::ISODate format time.

    \sa canConvert(), convert()
*/
QTime QVariant::toTime() const
{
    return qvariant_cast<QTime>(*this);
}

/*!
    \fn QDateTime QVariant::toDateTime() const

    Returns the variant as a QDateTime if the variant has userType()
    \l QMetaType::QDateTime, \l QMetaType::QDate, or \l QMetaType::QString;
    otherwise returns an invalid date/time.

    If the type() is \l QMetaType::QString, an invalid date/time will be
    returned if the string cannot be parsed as a Qt::ISODate format date/time.

    \sa canConvert(), convert()
*/
QDateTime QVariant::toDateTime() const
{
    return qvariant_cast<QDateTime>(*this);
}

/*!
    \since 4.7
    \fn QEasingCurve QVariant::toEasingCurve() const

    Returns the variant as a QEasingCurve if the variant has userType()
    \l QMetaType::QEasingCurve; otherwise returns a default easing curve.

    \sa canConvert(), convert()
*/
#if QT_CONFIG(easingcurve)
QEasingCurve QVariant::toEasingCurve() const
{
    return qvariant_cast<QEasingCurve>(*this);
}
#endif

/*!
    \fn QByteArray QVariant::toByteArray() const

    Returns the variant as a QByteArray if the variant has userType()
    \l QMetaType::QByteArray or \l QMetaType::QString (converted using
    QString::fromUtf8()); otherwise returns an empty byte array.

    \sa canConvert(), convert()
*/
QByteArray QVariant::toByteArray() const
{
    return qvariant_cast<QByteArray>(*this);
}

/*!
    \fn QPoint QVariant::toPoint() const

    Returns the variant as a QPoint if the variant has userType()
    \l QMetaType::QPoint or \l QMetaType::QPointF; otherwise returns a null
    QPoint.

    \sa canConvert(), convert()
*/
QPoint QVariant::toPoint() const
{
    return qvariant_cast<QPoint>(*this);
}

/*!
    \fn QRect QVariant::toRect() const

    Returns the variant as a QRect if the variant has userType()
    \l QMetaType::QRect; otherwise returns an invalid QRect.

    \sa canConvert(), convert()
*/
QRect QVariant::toRect() const
{
    return qvariant_cast<QRect>(*this);
}

/*!
    \fn QSize QVariant::toSize() const

    Returns the variant as a QSize if the variant has userType()
    \l QMetaType::QSize; otherwise returns an invalid QSize.

    \sa canConvert(), convert()
*/
QSize QVariant::toSize() const
{
    return qvariant_cast<QSize>(*this);
}

/*!
    \fn QSizeF QVariant::toSizeF() const

    Returns the variant as a QSizeF if the variant has userType() \l
    QMetaType::QSizeF; otherwise returns an invalid QSizeF.

    \sa canConvert(), convert()
*/
QSizeF QVariant::toSizeF() const
{
    return qvariant_cast<QSizeF>(*this);
}

/*!
    \fn QRectF QVariant::toRectF() const

    Returns the variant as a QRectF if the variant has userType()
    \l QMetaType::QRect or \l QMetaType::QRectF; otherwise returns an invalid
    QRectF.

    \sa canConvert(), convert()
*/
QRectF QVariant::toRectF() const
{
    return qvariant_cast<QRectF>(*this);
}

/*!
    \fn QLineF QVariant::toLineF() const

    Returns the variant as a QLineF if the variant has userType()
    \l QMetaType::QLineF; otherwise returns an invalid QLineF.

    \sa canConvert(), convert()
*/
QLineF QVariant::toLineF() const
{
    return qvariant_cast<QLineF>(*this);
}

/*!
    \fn QLine QVariant::toLine() const

    Returns the variant as a QLine if the variant has userType()
    \l QMetaType::QLine; otherwise returns an invalid QLine.

    \sa canConvert(), convert()
*/
QLine QVariant::toLine() const
{
    return qvariant_cast<QLine>(*this);
}

/*!
    \fn QPointF QVariant::toPointF() const

    Returns the variant as a QPointF if the variant has userType() \l
    QMetaType::QPoint or \l QMetaType::QPointF; otherwise returns a null
    QPointF.

    \sa canConvert(), convert()
*/
QPointF QVariant::toPointF() const
{
    return qvariant_cast<QPointF>(*this);
}

/*!
    \fn QUrl QVariant::toUrl() const

    Returns the variant as a QUrl if the variant has userType()
    \l QMetaType::QUrl; otherwise returns an invalid QUrl.

    \sa canConvert(), convert()
*/
QUrl QVariant::toUrl() const
{
    return qvariant_cast<QUrl>(*this);
}

/*!
    \fn QLocale QVariant::toLocale() const

    Returns the variant as a QLocale if the variant has userType()
    \l QMetaType::QLocale; otherwise returns an invalid QLocale.

    \sa canConvert(), convert()
*/
QLocale QVariant::toLocale() const
{
    return qvariant_cast<QLocale>(*this);
}

#if QT_CONFIG(regularexpression)
/*!
    \fn QRegularExpression QVariant::toRegularExpression() const
    \since 5.0

    Returns the variant as a QRegularExpression if the variant has userType() \l
    QRegularExpression; otherwise returns an empty QRegularExpression.

    \sa canConvert(), convert()
*/
QRegularExpression QVariant::toRegularExpression() const
{
    return qvariant_cast<QRegularExpression>(*this);
}
#endif // QT_CONFIG(regularexpression)

#if QT_CONFIG(itemmodel)
/*!
    \since 5.0

    Returns the variant as a QModelIndex if the variant has userType() \l
    QModelIndex; otherwise returns a default constructed QModelIndex.

    \sa canConvert(), convert(), toPersistentModelIndex()
*/
QModelIndex QVariant::toModelIndex() const
{
    return qvariant_cast<QModelIndex>(*this);
}

/*!
    \since 5.5

    Returns the variant as a QPersistentModelIndex if the variant has userType() \l
    QPersistentModelIndex; otherwise returns a default constructed QPersistentModelIndex.

    \sa canConvert(), convert(), toModelIndex()
*/
QPersistentModelIndex QVariant::toPersistentModelIndex() const
{
    return qvariant_cast<QPersistentModelIndex>(*this);
}
#endif // QT_CONFIG(itemmodel)

/*!
    \since 5.0

    Returns the variant as a QUuid if the variant has type()
    \l QMetaType::QUuid, \l QMetaType::QByteArray or \l QMetaType::QString;
    otherwise returns a default-constructed QUuid.

    \sa canConvert(), convert()
*/
QUuid QVariant::toUuid() const
{
    return qvariant_cast<QUuid>(*this);
}

/*!
    \since 5.0

    Returns the variant as a QJsonValue if the variant has userType() \l
    QJsonValue; otherwise returns a default constructed QJsonValue.

    \sa canConvert(), convert()
*/
QJsonValue QVariant::toJsonValue() const
{
    return qvariant_cast<QJsonValue>(*this);
}

/*!
    \since 5.0

    Returns the variant as a QJsonObject if the variant has userType() \l
    QJsonObject; otherwise returns a default constructed QJsonObject.

    \sa canConvert(), convert()
*/
QJsonObject QVariant::toJsonObject() const
{
    return qvariant_cast<QJsonObject>(*this);
}

/*!
    \since 5.0

    Returns the variant as a QJsonArray if the variant has userType() \l
    QJsonArray; otherwise returns a default constructed QJsonArray.

    \sa canConvert(), convert()
*/
QJsonArray QVariant::toJsonArray() const
{
    return qvariant_cast<QJsonArray>(*this);
}

/*!
    \since 5.0

    Returns the variant as a QJsonDocument if the variant has userType() \l
    QJsonDocument; otherwise returns a default constructed QJsonDocument.

    \sa canConvert(), convert()
*/
QJsonDocument QVariant::toJsonDocument() const
{
    return qvariant_cast<QJsonDocument>(*this);
}

/*!
    \fn QChar QVariant::toChar() const

    Returns the variant as a QChar if the variant has userType()
    \l QMetaType::QChar, \l QMetaType::Int, or \l QMetaType::UInt; otherwise
    returns an invalid QChar.

    \sa canConvert(), convert()
*/
QChar QVariant::toChar() const
{
    return qvariant_cast<QChar>(*this);
}

/*!
    Returns the variant as a QBitArray if the variant has userType()
    \l QMetaType::QBitArray; otherwise returns an empty bit array.

    \sa canConvert(), convert()
*/
QBitArray QVariant::toBitArray() const
{
    return qvariant_cast<QBitArray>(*this);
}

template <typename T>
inline T qNumVariantToHelper(const QVariant::Private &d, bool *ok)
{
    QMetaType t = QMetaType::fromType<T>();
    if (ok)
        *ok = true;

    if (d.type() == t)
        return d.get<T>();

    T ret = 0;
    bool success = QMetaType::convert(d.type(), d.storage(), t, &ret);
    if (ok)
        *ok = success;
    return ret;
}

/*!
    Returns the variant as an int if the variant has userType()
    \l QMetaType::Int, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \b{Warning:} If the value is convertible to a \l QMetaType::LongLong but is
    too large to be represented in an int, the resulting arithmetic overflow
    will not be reflected in \a ok. A simple workaround is to use
    QString::toInt().

    \sa canConvert(), convert()
*/
int QVariant::toInt(bool *ok) const
{
    return qNumVariantToHelper<int>(d, ok);
}

/*!
    Returns the variant as an unsigned int if the variant has userType()
    \l QMetaType::UInt, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::LongLong, \l QMetaType::QString, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an unsigned int; otherwise \c{*}\a{ok} is set to false.

    \b{Warning:} If the value is convertible to a \l QMetaType::ULongLong but is
    too large to be represented in an unsigned int, the resulting arithmetic
    overflow will not be reflected in \a ok. A simple workaround is to use
    QString::toUInt().

    \sa canConvert(), convert()
*/
uint QVariant::toUInt(bool *ok) const
{
    return qNumVariantToHelper<uint>(d, ok);
}

/*!
    Returns the variant as a long long int if the variant has userType()
    \l QMetaType::LongLong, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\c{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\c{ok} is set to false.

    \sa canConvert(), convert()
*/
qlonglong QVariant::toLongLong(bool *ok) const
{
    return qNumVariantToHelper<qlonglong>(d, ok);
}

/*!
    Returns the variant as an unsigned long long int if the
    variant has type() \l QMetaType::ULongLong, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
    \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString, or
    \l QMetaType::UInt; otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qulonglong QVariant::toULongLong(bool *ok) const
{
    return qNumVariantToHelper<qulonglong>(d, ok);
}

/*!
    Returns the variant as a bool if the variant has userType() Bool.

    Returns \c true if the variant has userType() \l QMetaType::Bool,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::LongLong, \l QMetaType::UInt, or \l QMetaType::ULongLong and
    the value is non-zero, or if the variant has type \l QMetaType::QString or
    \l QMetaType::QByteArray and its lower-case content is not one of the
    following: empty, "0" or "false"; otherwise returns \c false.

    \sa canConvert(), convert()
*/
bool QVariant::toBool() const
{
    auto boolType = QMetaType::fromType<bool>();
    if (d.type() == boolType)
        return d.get<bool>();

    bool res = false;
    QMetaType::convert(d.type(), constData(), boolType, &res);
    return res;
}

/*!
    Returns the variant as a double if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
double QVariant::toDouble(bool *ok) const
{
    return qNumVariantToHelper<double>(d, ok);
}

/*!
    Returns the variant as a float if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
float QVariant::toFloat(bool *ok) const
{
    return qNumVariantToHelper<float>(d, ok);
}

/*!
    Returns the variant as a qreal if the variant has userType()
    \l QMetaType::Double, \l QMetaType::Float, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::Int, \l QMetaType::LongLong,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.0.

    \since 4.6

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to a double; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(), convert()
*/
qreal QVariant::toReal(bool *ok) const
{
    return qNumVariantToHelper<qreal>(d, ok);
}

/*!
    Returns the variant as a QVariantList if the variant has userType() \l
    QMetaType::QVariantList. If it doesn't, QVariant will attempt to convert
    the type to a list and then return it. This will succeed for any type that
    has registered a converter to QVariantList or which was declared as a
    sequential container using \l{Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE}. If
    none of those conditions are true, this function will return an empty
    list.

    \sa canConvert(), convert()
*/
QVariantList QVariant::toList() const
{
    return qvariant_cast<QVariantList>(*this);
}

/*!
    \fn bool QVariant::canConvert(int targetTypeId) const
    \overload
    \deprecated [6.0] Use \c canConvert(QMetaType(targetTypeId)) instead.

    \sa QMetaType::canConvert()
*/

/*!
    \fn bool QVariant::canConvert(QMetaType type) const
    \since 6.0

    Returns \c true if the variant's type can be cast to the requested
    type, \a type. Such casting is done automatically when calling the
    toInt(), toBool(), ... methods.

    Note this function operates only on the variant's type, not the contents.
    It indicates whether there is a conversion path from this variant to \a
    type, not that the conversion will succeed when attempted.

    \sa QMetaType::canConvert()
*/


/*!
    \fn bool QVariant::convert(int targetTypeId)
    \deprecated [6.0] Use \c convert(QMetaType(targetTypeId)) instead.

    Casts the variant to the requested type, \a targetTypeId. If the cast cannot be
    done, the variant is still changed to the requested type, but is left in a cleared
    null state similar to that constructed by QVariant(Type).

    Returns \c true if the current type of the variant was successfully cast;
    otherwise returns \c false.

    A QVariant containing a pointer to a type derived from QObject will also convert
    and return true for this function if a qobject_cast to the type described
    by \a targetTypeId would succeed. Note that this only works for QObject subclasses
    which use the Q_OBJECT macro.

    \note converting QVariants that are null due to not being initialized or having
    failed a previous conversion will always fail, changing the type, remaining null,
    and returning \c false.

    \sa canConvert(), clear()
*/

/*!
    Casts the variant to the requested type, \a targetType. If the cast cannot be
    done, the variant is still changed to the requested type, but is left in a cleared
    null state similar to that constructed by QVariant(Type).

    Returns \c true if the current type of the variant was successfully cast;
    otherwise returns \c false.

    A QVariant containing a pointer to a type derived from QObject will also convert
    and return true for this function if a qobject_cast to the type described
    by \a targetType would succeed. Note that this only works for QObject subclasses
    which use the Q_OBJECT macro.

    \note converting QVariants that are null due to not being initialized or having
    failed a previous conversion will always fail, changing the type, remaining null,
    and returning \c false.

    \since 6.0

    \sa canConvert(), clear()
*/

bool QVariant::convert(QMetaType targetType)
{
    if (d.type() == targetType)
        return targetType.isValid();

    QVariant oldValue = *this;

    clear();
    create(targetType, nullptr);
    if (!oldValue.canConvert(targetType))
        return false;

    // Fail if the value is not initialized or was forced null by a previous failed convert.
    if (oldValue.d.is_null && oldValue.d.type().id() != QMetaType::Nullptr)
        return false;

    bool ok = QMetaType::convert(oldValue.d.type(), oldValue.constData(), targetType, data());
    d.is_null = !ok;
    return ok;
}

/*!
  \fn bool QVariant::convert(int type, void *ptr) const
  \internal
  Created for qvariant_cast() usage
*/
bool QVariant::convert(int type, void *ptr) const
{
    return QMetaType::convert(d.type(), constData(), QMetaType(type), ptr);
}

/*!
  \internal
*/
bool QVariant::view(int type, void *ptr)
{
    return QMetaType::view(d.type(), data(), QMetaType(type), ptr);
}

/*!
    \fn bool QVariant::operator==(const QVariant &lhs, const QVariant &rhs)

    Returns \c true if \a lhs and \a rhs are equal; otherwise returns \c false.

    QVariant uses the equality operator of the type() contained to check for
    equality.

    Variants of different types will always compare as not equal with a few
    exceptions:

    \list
    \li If both types are numeric types (integers and floatins point numbers)
    Qt will compare those types using standard C++ type promotion rules.
    \li If one type is numeric and the other one a QString, Qt will try to
    convert the QString to a matching numeric type and if successful compare
    those.
    \li If both variants contain pointers to QObject derived types, QVariant
    will check whether the types are related and point to the same object.
    \endlist

    The result of the function is not affected by the result of QVariant::isNull,
    which means that two values can be equal even if one of them is null and
    another is not.
*/

/*!
    \fn bool QVariant::operator!=(const QVariant &lhs, const QVariant &rhs)

    Returns \c false if \a lhs and \a rhs are equal; otherwise returns \c true.

    QVariant uses the equality operator of the type() contained to check for
    equality.

    Variants of different types will always compare as not equal with a few
    exceptions:

    \list
    \li If both types are numeric types (integers and floatins point numbers)
    Qt will compare those types using standard C++ type promotion rules.
    \li If one type is numeric and the other one a QString, Qt will try to
    convert the QString to a matching numeric type and if successful compare
    those.
    \li If both variants contain pointers to QObject derived types, QVariant
    will check whether the types are related and point to the same object.
    \endlist
*/

static bool qIsNumericType(uint tp)
{
    static const qulonglong numericTypeBits =
            Q_UINT64_C(1) << QMetaType::QString |
            Q_UINT64_C(1) << QMetaType::Bool |
            Q_UINT64_C(1) << QMetaType::Double |
            Q_UINT64_C(1) << QMetaType::Float16 |
            Q_UINT64_C(1) << QMetaType::Float |
            Q_UINT64_C(1) << QMetaType::Char |
            Q_UINT64_C(1) << QMetaType::Char16 |
            Q_UINT64_C(1) << QMetaType::Char32 |
            Q_UINT64_C(1) << QMetaType::SChar |
            Q_UINT64_C(1) << QMetaType::UChar |
            Q_UINT64_C(1) << QMetaType::Short |
            Q_UINT64_C(1) << QMetaType::UShort |
            Q_UINT64_C(1) << QMetaType::Int |
            Q_UINT64_C(1) << QMetaType::UInt |
            Q_UINT64_C(1) << QMetaType::Long |
            Q_UINT64_C(1) << QMetaType::ULong |
            Q_UINT64_C(1) << QMetaType::LongLong |
            Q_UINT64_C(1) << QMetaType::ULongLong;
    return tp < (CHAR_BIT * sizeof numericTypeBits) ? numericTypeBits & (Q_UINT64_C(1) << tp) : false;
}

static bool qIsFloatingPoint(uint tp)
{
    return tp == QMetaType::Double || tp == QMetaType::Float || tp == QMetaType::Float16;
}

static bool canBeNumericallyCompared(const QtPrivate::QMetaTypeInterface *iface1,
                                     const QtPrivate::QMetaTypeInterface *iface2)
{
    if (!iface1 || !iface2)
        return false;

    // We don't need QMetaType::id() here because the type Id is always stored
    // directly for all built-in types.
    bool isNumeric1 = qIsNumericType(iface1->typeId);
    bool isNumeric2 = qIsNumericType(iface2->typeId);

    // if they're both numeric (or QString), then they can be compared
    if (isNumeric1 && isNumeric2)
        return true;

    bool isEnum1 = iface1->flags & QMetaType::IsEnumeration;
    bool isEnum2 = iface2->flags & QMetaType::IsEnumeration;

    // if both are enums, we can only compare if they are the same enum
    // (the language does allow comparing two different enum types, but that's
    // usually considered poor coding and produces a warning)
    if (isEnum1 && isEnum2)
        return QMetaType(iface1) == QMetaType(iface2);

    // if one is an enum and the other is a numeric, we can compare too
    if (isEnum1 && isNumeric2)
        return true;
    if (isNumeric1 && isEnum2)
        return true;

    // we need at least one enum and one numeric...
    return false;
}

static int numericTypePromotion(const QtPrivate::QMetaTypeInterface *iface1,
                                const QtPrivate::QMetaTypeInterface *iface2)
{
    Q_ASSERT(canBeNumericallyCompared(iface1, iface2));

    // We don't need QMetaType::id() here because the type Id is always stored
    // directly for the types we're comparing against below.
    uint t1 = iface1->typeId;
    uint t2 = iface2->typeId;

    if ((t1 == QMetaType::Bool && t2 == QMetaType::QString) ||
        (t2 == QMetaType::Bool && t1 == QMetaType::QString))
        return QMetaType::Bool;

    // C++ integral ranks: (4.13 Integer conversion rank [conv.rank])
    //   bool < signed char < short < int < long < long long
    //   unsigneds have the same rank as their signed counterparts
    // C++ integral promotion rules (4.5 Integral Promotions [conv.prom])
    // - any type with rank less than int can be converted to int or unsigned int
    // 5 Expressions [expr] paragraph 9:
    // - if either operand is double, the other shall be converted to double
    // -     "       "        float,   "          "         "         float
    // - if both operands have the same type, no further conversion is needed.
    // - if both are signed or if both are unsigned, convert to the one with highest rank
    // - if the unsigned has higher or same rank, convert the signed to the unsigned one
    // - if the signed can represent all values of the unsigned, convert to the signed
    // - otherwise, convert to the unsigned corresponding to the rank of the signed

    // floating point: we deviate from the C++ standard by always using qreal
    if (qIsFloatingPoint(t1) || qIsFloatingPoint(t2))
        return QMetaType::QReal;

    auto isUnsigned = [](uint tp) {
        // only types for which sizeof(T) >= sizeof(int); lesser ones promote to int
        return tp == QMetaType::ULongLong || tp == QMetaType::ULong ||
                tp == QMetaType::UInt || tp == QMetaType::Char32;
    };
    bool isUnsigned1 = isUnsigned(t1);
    bool isUnsigned2 = isUnsigned(t2);

    // integral rules:
    // 1) if either type is a 64-bit unsigned, compare as 64-bit unsigned
    if (isUnsigned1 && iface1->size > sizeof(int))
        return QMetaType::ULongLong;
    if (isUnsigned2 && iface2->size > sizeof(int))
        return QMetaType::ULongLong;

    // 2) if either type is 64-bit, compare as 64-bit signed
    if (iface1->size > sizeof(int) || iface2->size > sizeof(int))
        return QMetaType::LongLong;

    // 3) if either type is 32-bit unsigned, compare as 32-bit unsigned
    if (isUnsigned1 || isUnsigned2)
        return QMetaType::UInt;

    // 4) otherwise, just do int promotion
    return QMetaType::Int;
}

static QPartialOrdering integralCompare(uint promotedType, const QVariant::Private *d1, const QVariant::Private *d2)
{
    // use toLongLong to retrieve the data, it gets us all the bits
    std::optional<qlonglong> l1 = qConvertToNumber(d1, promotedType == QMetaType::Bool);
    std::optional<qlonglong> l2 = qConvertToNumber(d2, promotedType == QMetaType::Bool);
    if (!l1 || !l2)
        return QPartialOrdering::Unordered;
    if (promotedType == QMetaType::UInt)
        return Qt::compareThreeWay(uint(*l1), uint(*l2));
    if (promotedType == QMetaType::LongLong)
        return Qt::compareThreeWay(qlonglong(*l1), qlonglong(*l2));
    if (promotedType == QMetaType::ULongLong)
        return Qt::compareThreeWay(qulonglong(*l1), qulonglong(*l2));

    return Qt::compareThreeWay(int(*l1), int(*l2));
}

static QPartialOrdering numericCompare(const QVariant::Private *d1, const QVariant::Private *d2)
{
    uint promotedType = numericTypePromotion(d1->typeInterface(), d2->typeInterface());
    if (promotedType != QMetaType::QReal)
        return integralCompare(promotedType, d1, d2);

    // floating point comparison
    const auto r1 = qConvertToRealNumber(d1);
    const auto r2 = qConvertToRealNumber(d2);
    if (!r1 || !r2)
        return QPartialOrdering::Unordered;

    return Qt::compareThreeWay(*r1, *r2);
}

static bool qvCanConvertMetaObject(QMetaType fromType, QMetaType toType)
{
    if ((fromType.flags() & QMetaType::PointerToQObject)
            && (toType.flags() & QMetaType::PointerToQObject)) {
        const QMetaObject *f = fromType.metaObject();
        const QMetaObject *t = toType.metaObject();
        return f && t && (f->inherits(t) || t->inherits(f));
    }
    return false;
}

static QPartialOrdering pointerCompare(const QVariant::Private *d1, const QVariant::Private *d2)
{
    return Qt::compareThreeWay(Qt::totally_ordered_wrapper(d1->get<QObject *>()),
                               Qt::totally_ordered_wrapper(d2->get<QObject *>()));
}

/*!
    \internal
 */
bool QVariant::equals(const QVariant &v) const
{
    auto metatype = d.type();

    if (metatype != v.metaType()) {
        // try numeric comparisons, with C++ type promotion rules (no conversion)
        if (canBeNumericallyCompared(metatype.iface(), v.d.type().iface()))
            return numericCompare(&d, &v.d) == QPartialOrdering::Equivalent;
        // if both types are related pointers to QObjects, check if they point to the same object
        if (qvCanConvertMetaObject(metatype, v.metaType()))
            return pointerCompare(&d, &v.d) == QPartialOrdering::Equivalent;
        return false;
    }

    // For historical reasons: QVariant() == QVariant()
    if (!metatype.isValid())
        return true;

    return metatype.equals(d.storage(), v.d.storage());
}

/*!
    Compares the objects at \a lhs and \a rhs for ordering.

    Returns QPartialOrdering::Unordered if comparison is not supported
    or the values are unordered. Otherwise, returns
    QPartialOrdering::Less, QPartialOrdering::Equivalent or
    QPartialOrdering::Greater if \a lhs is less than, equivalent
    to or greater than \a rhs, respectively.

    If the variants contain data with a different metatype, the values are considered
    unordered unless they are both of numeric or pointer types, where regular numeric or
    pointer comparison rules will be used.
    \note: If a numeric comparison is done and at least one value is NaN, QPartialOrdering::Unordered
    is returned.

    If both variants contain data of the same metatype, the method will use the
    QMetaType::compare method to determine the ordering of the two variants, which can
    also indicate that it can't establish an ordering between the two values.

    \since 6.0
    \sa QMetaType::compare(), QMetaType::isOrdered()
*/
QPartialOrdering QVariant::compare(const QVariant &lhs, const QVariant &rhs)
{
    QMetaType t = lhs.d.type();
    if (t != rhs.d.type()) {
        // try numeric comparisons, with C++ type promotion rules (no conversion)
        if (canBeNumericallyCompared(lhs.d.type().iface(), rhs.d.type().iface()))
            return numericCompare(&lhs.d, &rhs.d);
        if (qvCanConvertMetaObject(lhs.metaType(), rhs.metaType()))
            return pointerCompare(&lhs.d, &rhs.d);
        return QPartialOrdering::Unordered;
    }
    return t.compare(lhs.constData(), rhs.constData());
}

/*!
    \fn const void *QVariant::constData() const
    \fn const void* QVariant::data() const

    Returns a pointer to the contained object as a generic void* that cannot be
    written to.

    \sa get_if(), QMetaType
 */

/*!
    Returns a pointer to the contained object as a generic void* that can be
    written to.

    This function detaches the QVariant. When called on a \l{isNull}{null-QVariant},
    the QVariant will not be null after the call.

    \sa get_if(), QMetaType
*/
void *QVariant::data()
{
    detach();
    // set is_null to false, as the caller is likely to write some data into this variant
    d.is_null = false;
    return const_cast<void *>(constData());
}

/*!
    \since 6.6
    \fn template <typename T> const T* QVariant::get_if(const QVariant *v)
    \fn template <typename T> T* QVariant::get_if(QVariant *v)

    If \a v contains an object of type \c T, returns a pointer to the contained
    object, otherwise returns \nullptr.

    The overload taking a mutable \a v detaches \a v: When called on a
    \l{isNull()}{null} \a v with matching type \c T, \a v will not be null
    after the call.

    These functions are provided for compatibility with \c{std::variant}.

    \sa data()
*/

/*!
    \since 6.6
    \fn template <typename T> T &QVariant::get(QVariant &v)
    \fn template <typename T> const T &QVariant::get(const QVariant &v)
    \fn template <typename T> T &&QVariant::get(QVariant &&v)
    \fn template <typename T> const T &&QVariant::get(const QVariant &&v)

    If \a v contains an object of type \c T, returns a reference to the contained
    object, otherwise the call has undefined behavior.

    The overloads taking a mutable \a v detach \a v: When called on a
    \l{isNull()}{null} \a v with matching type \c T, \a v will not be null
    after the call.

    These functions are provided for compatibility with \c{std::variant}.

    \sa get_if(), data()
*/

/*!
    Returns \c true if this is a null variant, false otherwise.

    A variant is considered null if it contains no initialized value or a null pointer.

    \note This behavior has been changed from Qt 5, where isNull() would also
    return true if the variant contained an object of a builtin type with an isNull()
    method that returned true for that object.

    \sa convert()
*/
bool QVariant::isNull() const
{
    if (d.is_null || !metaType().isValid())
        return true;
    if (metaType().flags() & QMetaType::IsPointer)
        return d.get<void *>() == nullptr;
    return false;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug QVariant::qdebugHelper(QDebug dbg) const
{
    QDebugStateSaver saver(dbg);
    const uint typeId = d.type().id();
    dbg.nospace() << "QVariant(";
    if (typeId != QMetaType::UnknownType) {
        dbg << d.type().name() << ", ";
        bool streamed = d.type().debugStream(dbg, d.storage());
        if (!streamed && canConvert<QString>())
            dbg << toString();
    } else {
        dbg << "Invalid";
    }
    dbg << ')';
    return dbg;
}

QVariant QVariant::moveConstruct(QMetaType type, void *data)
{
    QVariant var;
    var.d = QVariant::Private(type.d_ptr);
    customConstruct<ForceMove, NonNull>(type.d_ptr, &var.d, data);
    return var;
}

QVariant QVariant::copyConstruct(QMetaType type, const void *data)
{
    QVariant var;
    var.d = QVariant::Private(type.d_ptr);
    customConstruct<UseCopy, NonNull>(type.d_ptr, &var.d, data);
    return var;
}

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

QDebug operator<<(QDebug dbg, const QVariant::Type p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVariant::"
                  << (int(p) != int(QMetaType::UnknownType)
                     ? QMetaType(p).name()
                     : "Invalid");
    return dbg;
}

QT_WARNING_POP
#endif

#endif

/*! \fn template<typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, QVariant>>> void QVariant::setValue(T &&value)

    Stores a copy of \a value. If \c{T} is a type that QVariant
    doesn't support, QMetaType is used to store the value. A compile
    error will occur if QMetaType doesn't handle the type.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 4

    \sa value(), fromValue(), canConvert()
 */

/*! \fn void QVariant::setValue(const QVariant &value)

    Copies \a value over this QVariant. It is equivalent to simply
    assigning \a value to this QVariant.
*/

/*! \fn void QVariant::setValue(QVariant &&value)

    Moves \a value over this QVariant. It is equivalent to simply
    move assigning \a value to this QVariant.
*/

/*! \fn template<typename T> T QVariant::value() const &

    Returns the stored value converted to the template type \c{T}.
    Call canConvert() to find out whether a type can be converted.
    If the value cannot be converted, a \l{default-constructed value}
    will be returned.

    If the type \c{T} is supported by QVariant, this function behaves
    exactly as toString(), toInt() etc.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 5

    If the QVariant contains a pointer to a type derived from QObject then
    \c{T} may be any QObject type. If the pointer stored in the QVariant can be
    qobject_cast to T, then that result is returned. Otherwise \nullptr is
    returned. Note that this only works for QObject subclasses which use
    the Q_OBJECT macro.

    If the QVariant contains a sequential container and \c{T} is QVariantList, the
    elements of the container will be converted into \l {QVariant}s and returned as a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa setValue(), fromValue(), canConvert(), Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE()
*/

/*! \fn template<typename T> T QVariant::view()

    Returns a mutable view of template type \c{T} on the stored value.
    Call canView() to find out whether such a view is supported.
    If no such view can be created, returns the stored value converted to the
    template type \c{T}. Call canConvert() to find out whether a type can be
    converted. If the value can neither be viewed nor converted, a
    \l{default-constructed value} will be returned.

    \sa canView(), Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE()
*/

/*! \fn template<typename T> bool QVariant::canConvert() const

    Returns \c true if the variant can be converted to the template type \c{T},
    otherwise false.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 6

    A QVariant containing a pointer to a type derived from QObject will also return true for this
    function if a qobject_cast to the template type \c{T} would succeed. Note that this only works
    for QObject subclasses which use the Q_OBJECT macro.

    \sa convert()
*/

/*! \fn template<typename T> bool QVariant::canView() const

    Returns \c true if a mutable view of the template type \c{T} can be created on this variant,
    otherwise \c false.

    \sa value()
*/

/*! \fn template<typename T> static QVariant QVariant::fromValue(const T &value)

    Returns a QVariant containing a copy of \a value. Behaves
    exactly like setValue() otherwise.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 7

    \sa setValue(), value()
*/

/*! \fn template<typename T, QVariant::if_rvalue<T> = true> static QVariant QVariant::fromValue(T &&value)

    \since 6.6
    \overload
*/

/*! \fn template<typename... Types> QVariant QVariant::fromStdVariant(const std::variant<Types...> &value)
    \since 5.11

    Returns a QVariant with the type and value of the active variant of \a value. If
    the active type is std::monostate a default QVariant is returned.

    \note With this method you do not need to register the variant as a Qt metatype,
    since the std::variant is resolved before being stored. The component types
    should be registered however.

    \sa fromValue()
*/

/*!
    \fn template<typename... Types> QVariant QVariant::fromStdVariant(std::variant<Types...> &&value)
    \since 6.6
    \overload
*/


/*!
    \since 6.7

    Creates a variant of type \a type, and initializes it with
    a copy of \c{*copy} if \a copy is not \nullptr (in which case, \a copy
    must point to an object of type \a type).

    Note that you have to pass the address of the object you want stored.

    Usually, you never have to use this constructor, use QVariant::fromValue()
    instead to construct variants from the pointer types represented by
    \c QMetaType::VoidStar, and \c QMetaType::QObjectStar.

    If \a type does not support copy construction and \a copy is not \nullptr,
    the variant will be invalid. Similarly, if \a copy is \nullptr and
    \a type does not support default construction, the variant will be
    invalid.

    Returns the QVariant created as described above.

    \sa QVariant::fromValue(), QMetaType::Type
*/
QVariant QVariant::fromMetaType(QMetaType type, const void *copy)
{
    QVariant result;
    type.registerType();
    const auto iface = type.iface();
    if (isValidMetaTypeForVariant(iface, copy)) {
        result.d = Private(iface);
        customConstruct(iface, &result.d, copy);
    }
    return result;
}

/*!
    \fn template<typename T> T qvariant_cast(const QVariant &value)
    \relates QVariant

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to QVariant::value().

    \sa QVariant::value()
*/

/*!
    \fn template<typename T> T QVariant::qvariant_cast(QVariant &&value)
    \overload
    \since 6.7

    Returns the given \a value converted to the template type \c{T}.
*/

/*! \fn template<typename T> T qVariantValue(const QVariant &value)
    \relates QVariant
    \deprecated

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to
    \l{QVariant::value()}{QVariant::value}<T>(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::value(), qvariant_cast()
*/

/*! \fn bool qVariantCanConvert(const QVariant &value)
    \relates QVariant
    \deprecated

    Returns \c true if the given \a value can be converted to the
    template type specified; otherwise returns \c false.

    This function is equivalent to QVariant::canConvert(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::canConvert()
*/

/*!
    \typedef QVariantList
    \relates QVariant

    Synonym for QList<QVariant>.
*/

/*!
    \typedef QVariantMap
    \relates QVariant

    Synonym for QMap<QString, QVariant>.
*/

/*!
    \typedef QVariantHash
    \relates QVariant
    \since 4.5

    Synonym for QHash<QString, QVariant>.
*/

/*!
    \typedef QVariant::DataPtr
    \internal
*/
/*! \typedef QVariant::f_construct
  \internal
*/

/*! \typedef QVariant::f_clear
  \internal
*/

/*! \typedef QVariant::f_null
  \internal
*/

/*! \typedef QVariant::f_load
  \internal
*/

/*! \typedef QVariant::f_save
  \internal
*/

/*! \typedef QVariant::f_compare
  \internal
*/

/*! \typedef QVariant::f_convert
  \internal
*/

/*! \typedef QVariant::f_canConvert
  \internal
*/

/*! \typedef QVariant::f_debugStream
  \internal
*/

/*!
    \fn DataPtr &QVariant::data_ptr()
    \internal
*/

/*!
    \fn const DataPtr &QVariant::data_ptr() const
    \internal
*/

/*!
    \internal
 */
const void *QtPrivate::QVariantTypeCoercer::convert(const QVariant &value, const QMetaType &type)
{
    if (type == QMetaType::fromType<QVariant>())
        return &value;

    if (type == value.metaType())
        return value.constData();

    if (value.canConvert(type)) {
        converted = value;
        if (converted.convert(type))
            return converted.constData();
    }

    return nullptr;
}

/*!
    \internal
 */
const void *QtPrivate::QVariantTypeCoercer::coerce(const QVariant &value, const QMetaType &type)
{
    if (const void *result = convert(value, type))
        return result;

    converted = QVariant(type);
    return converted.constData();
}

/*!
    \class QVariantRef
    \since 6.0
    \inmodule QtCore
    \brief The QVariantRef acts as a non-const reference to a QVariant.

    As the generic iterators don't actually instantiate a QVariant on each
    step, they cannot return a reference to one from operator*(). QVariantRef
    provides the same functionality as an actual reference to a QVariant would,
    but is backed by a pointer given as template parameter. The template is
    implemented for pointers of type QSequentialIterator and
    QAssociativeIterator.
*/

/*!
    \fn template<typename Pointer> QVariantRef<Pointer>::QVariantRef(const Pointer *pointer)

    Creates a QVariantRef from an \a pointer.
 */

/*!
    \fn template<typename Pointer> QVariantRef<Pointer> &QVariantRef<Pointer>::operator=(const QVariant &value)

    Assigns a new \a value to the value pointed to by the pointer this
    QVariantRef refers to.
 */

/*!
    \fn template<typename Pointer> QVariantRef<Pointer> &QVariantRef<Pointer>::operator=(const QVariantRef &value)

    Assigns a new \a value to the value pointed to by the pointer this
    QVariantRef refers to.
 */

/*!
    \fn template<typename Pointer> QVariantRef<Pointer> &QVariantRef<Pointer>::operator=(QVariantRef &&value)

    Assigns a new \a value to the value pointed to by the pointer this
    QVariantRef refers to.
*/

/*!
    \fn template<typename Pointer> QVariantRef<Pointer>::operator QVariant() const

    Resolves the QVariantRef to an actual QVariant.
*/

/*!
    \fn template<typename Pointer> void swap(QVariantRef<Pointer> a, QVariantRef<Pointer> b)

    Swaps the values pointed to by the pointers the QVariantRefs
    \a a and \a b refer to.
*/

/*!
    \class QVariantConstPointer
    \since 6.0
    \inmodule QtCore
    \brief Emulated const pointer to QVariant based on a pointer.

    QVariantConstPointer wraps a QVariant and returns it from its operator*().
    This makes it suitable as replacement for an actual const pointer. We cannot
    return an actual const pointer from generic iterators as the iterators don't
    hold an actual QVariant.
*/

/*!
    Constructs a QVariantConstPointer from a \a variant.
 */
QVariantConstPointer::QVariantConstPointer(QVariant variant)
    : m_variant(std::move(variant))
{
}

/*!
    Dereferences the QVariantConstPointer to retrieve its internal QVariant.
 */
QVariant QVariantConstPointer::operator*() const
{
    return m_variant;
}

/*!
    Returns a const pointer to the QVariant, conforming to the
    conventions for operator->().
 */
const QVariant *QVariantConstPointer::operator->() const
{
    return &m_variant;
}

/*!
    \class QVariantPointer
    \since 6.0
    \inmodule QtCore
    \brief QVariantPointer is a template class that emulates a pointer to QVariant based on a pointer.

    QVariantConstPointer wraps a pointer and returns QVariantRef to it from its
    operator*(). This makes it suitable as replacement for an actual pointer. We
    cannot return an actual pointer from generic iterators as the iterators don't
    hold an actual QVariant.
*/

/*!
    \fn template<typename Pointer> QVariantPointer<Pointer>::QVariantPointer(const Pointer *pointer)

    Constructs a QVariantPointer from the given \a pointer.
 */

/*!
    \fn template<typename Pointer> QVariantRef<Pointer> QVariantPointer<Pointer>::operator*() const

    Dereferences the QVariantPointer to a QVariantRef.
 */

/*!
    \fn template<typename Pointer> Pointer QVariantPointer<Pointer>::operator->() const

    Dereferences and returns the pointer. The pointer is expected to also
    implement operator->().
 */

QT_END_NAMESPACE
