/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMETATYPE_H
#define QMETATYPE_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qisenum.h>
#ifndef QT_NO_QOBJECT
#include <QtCore/qobjectdefs.h>
#endif
#include <new>

#ifdef Bool
#error qmetatype.h must be included before any header file that defines Bool
#endif

QT_BEGIN_NAMESPACE


// F is a tuple: (QMetaType::TypeName, QMetaType::TypeNameID, RealType)
#define QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F)\
    F(Void, 43, void) \
    F(Bool, 1, bool) \
    F(Int, 2, int) \
    F(UInt, 3, uint) \
    F(LongLong, 4, qlonglong) \
    F(ULongLong, 5, qulonglong) \
    F(Double, 6, double) \
    F(Long, 32, long) \
    F(Short, 33, short) \
    F(Char, 34, char) \
    F(ULong, 35, ulong) \
    F(UShort, 36, ushort) \
    F(UChar, 37, uchar) \
    F(Float, 38, float) \
    F(SChar, 40, signed char) \

#define QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(F)\
    F(VoidStar, 31, void*) \

#define QT_FOR_EACH_STATIC_CORE_CLASS(F)\
    F(QChar, 7, QChar) \
    F(QString, 10, QString) \
    F(QStringList, 11, QStringList) \
    F(QByteArray, 12, QByteArray) \
    F(QBitArray, 13, QBitArray) \
    F(QDate, 14, QDate) \
    F(QTime, 15, QTime) \
    F(QDateTime, 16, QDateTime) \
    F(QUrl, 17, QUrl) \
    F(QLocale, 18, QLocale) \
    F(QRect, 19, QRect) \
    F(QRectF, 20, QRectF) \
    F(QSize, 21, QSize) \
    F(QSizeF, 22, QSizeF) \
    F(QLine, 23, QLine) \
    F(QLineF, 24, QLineF) \
    F(QPoint, 25, QPoint) \
    F(QPointF, 26, QPointF) \
    F(QRegExp, 27, QRegExp) \
    F(QEasingCurve, 29, QEasingCurve) \
    F(QUuid, 30, QUuid) \
    F(QVariant, 41, QVariant) \
    F(QModelIndex, 42, QModelIndex) \
    F(QRegularExpression, 44, QRegularExpression) \
    F(QJsonValue, 45, QJsonValue) \
    F(QJsonObject, 46, QJsonObject) \
    F(QJsonArray, 47, QJsonArray) \
    F(QJsonDocument, 48, QJsonDocument) \

#define QT_FOR_EACH_STATIC_CORE_POINTER(F)\
    F(QObjectStar, 39, QObject*)

#define QT_FOR_EACH_STATIC_CORE_TEMPLATE(F)\
    F(QVariantMap, 8, QVariantMap) \
    F(QVariantList, 9, QVariantList) \
    F(QVariantHash, 28, QVariantHash) \

#define QT_FOR_EACH_STATIC_GUI_CLASS(F)\
    F(QFont, 64, QFont) \
    F(QPixmap, 65, QPixmap) \
    F(QBrush, 66, QBrush) \
    F(QColor, 67, QColor) \
    F(QPalette, 68, QPalette) \
    F(QIcon, 69, QIcon) \
    F(QImage, 70, QImage) \
    F(QPolygon, 71, QPolygon) \
    F(QRegion, 72, QRegion) \
    F(QBitmap, 73, QBitmap) \
    F(QCursor, 74, QCursor) \
    F(QKeySequence, 75, QKeySequence) \
    F(QPen, 76, QPen) \
    F(QTextLength, 77, QTextLength) \
    F(QTextFormat, 78, QTextFormat) \
    F(QMatrix, 79, QMatrix) \
    F(QTransform, 80, QTransform) \
    F(QMatrix4x4, 81, QMatrix4x4) \
    F(QVector2D, 82, QVector2D) \
    F(QVector3D, 83, QVector3D) \
    F(QVector4D, 84, QVector4D) \
    F(QQuaternion, 85, QQuaternion) \
    F(QPolygonF, 86, QPolygonF) \


#define QT_FOR_EACH_STATIC_WIDGETS_CLASS(F)\
    F(QSizePolicy, 121, QSizePolicy) \

// ### FIXME kill that set
#define QT_FOR_EACH_STATIC_HACKS_TYPE(F)\
    F(QMetaTypeId2<qreal>::MetaType, -1, qreal)

// F is a tuple: (QMetaType::TypeName, QMetaType::TypeNameID, AliasingType, "RealType")
#define QT_FOR_EACH_STATIC_ALIAS_TYPE(F)\
    F(ULong, -1, ulong, "unsigned long") \
    F(UInt, -1, uint, "unsigned int") \
    F(UShort, -1, ushort, "unsigned short") \
    F(UChar, -1, uchar, "unsigned char") \
    F(LongLong, -1, qlonglong, "long long") \
    F(ULongLong, -1, qulonglong, "unsigned long long") \
    F(SChar, -1, signed char, "qint8") \
    F(UChar, -1, uchar, "quint8") \
    F(Short, -1, short, "qint16") \
    F(UShort, -1, ushort, "quint16") \
    F(Int, -1, int, "qint32") \
    F(UInt, -1, uint, "quint32") \
    F(LongLong, -1, qlonglong, "qint64") \
    F(ULongLong, -1, qulonglong, "quint64") \
    F(QVariantList, -1, QVariantList, "QList<QVariant>") \
    F(QVariantMap, -1, QVariantMap, "QMap<QString,QVariant>") \
    F(QVariantHash, -1, QVariantHash, "QHash<QString,QVariant>") \

#define QT_FOR_EACH_STATIC_TYPE(F)\
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F)\
    QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(F)\
    QT_FOR_EACH_STATIC_CORE_CLASS(F)\
    QT_FOR_EACH_STATIC_CORE_POINTER(F)\
    QT_FOR_EACH_STATIC_CORE_TEMPLATE(F)\
    QT_FOR_EACH_STATIC_GUI_CLASS(F)\
    QT_FOR_EACH_STATIC_WIDGETS_CLASS(F)\

#define QT_DEFINE_METATYPE_ID(TypeName, Id, Name) \
    TypeName = Id,

class QDataStream;
class QMetaTypeInterface;
struct QMetaObject;

class Q_CORE_EXPORT QMetaType {
    enum ExtensionFlag { NoExtensionFlags,
                         CreateEx = 0x1, DestroyEx = 0x2,
                         ConstructEx = 0x4, DestructEx = 0x8,
                         NameEx = 0x10, SizeEx = 0x20,
                         CtorEx = 0x40, DtorEx = 0x80,
                         FlagsEx = 0x100, MetaObjectEx = 0x200
                       };
public:
#ifndef Q_QDOC
    // The code that actually gets compiled.
    enum Type {
        // these are merged with QVariant
        QT_FOR_EACH_STATIC_TYPE(QT_DEFINE_METATYPE_ID)

        FirstCoreType = Bool,
        LastCoreType = QJsonDocument,
        FirstGuiType = QFont,
        LastGuiType = QPolygonF,
        FirstWidgetsType = QSizePolicy,
        LastWidgetsType = QSizePolicy,
        HighestInternalId = LastWidgetsType,

        QReal = sizeof(qreal) == sizeof(double) ? Double : Float,

        UnknownType = 0,
        User = 1024
    };
#else
    // If we are using QDoc it fakes the Type enum looks like this.
    enum Type {
        UnknownType = 0, Bool = 1, Int = 2, UInt = 3, LongLong = 4, ULongLong = 5,
        Double = 6, Long = 32, Short = 33, Char = 34, ULong = 35, UShort = 36,
        UChar = 37, Float = 38,
        VoidStar = 31,
        QChar = 7, QString = 10, QStringList = 11, QByteArray = 12,
        QBitArray = 13, QDate = 14, QTime = 15, QDateTime = 16, QUrl = 17,
        QLocale = 18, QRect = 19, QRectF = 20, QSize = 21, QSizeF = 22,
        QLine = 23, QLineF = 24, QPoint = 25, QPointF = 26, QRegExp = 27,
        QEasingCurve = 29, QUuid = 30, QVariant = 41, QModelIndex = 42,
        QRegularExpression = 44,
        QJsonValue = 45, QJsonObject = 46, QJsonArray = 47, QJsonDocument = 48,
        QObjectStar = 39, SChar = 40,
        Void = 43,
        QVariantMap = 8, QVariantList = 9, QVariantHash = 28,
        QFont = 64, QPixmap = 65, QBrush = 66, QColor = 67, QPalette = 68,
        QIcon = 69, QImage = 70, QPolygon = 71, QRegion = 72, QBitmap = 73,
        QCursor = 74, QKeySequence = 75, QPen = 76, QTextLength = 77, QTextFormat = 78,
        QMatrix = 79, QTransform = 80, QMatrix4x4 = 81, QVector2D = 82,
        QVector3D = 83, QVector4D = 84, QQuaternion = 85, QPolygonF = 86,
        QSizePolicy = 121,
        User = 1024
    };
#endif

    enum TypeFlag {
        NeedsConstruction = 0x1,
        NeedsDestruction = 0x2,
        MovableType = 0x4,
        PointerToQObject = 0x8,
        IsEnumeration = 0x10,
        SharedPointerToQObject = 0x20,
        WeakPointerToQObject = 0x40,
        TrackingPointerToQObject = 0x80,
        WasDeclaredAsMetaType = 0x100
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

    typedef void (*Deleter)(void *);
    typedef void *(*Creator)(const void *);

    typedef void (*Destructor)(void *);
    typedef void *(*Constructor)(void *, const void *);

    typedef void (*SaveOperator)(QDataStream &, const void *);
    typedef void (*LoadOperator)(QDataStream &, void *);
#ifndef QT_NO_DATASTREAM
    static void registerStreamOperators(const char *typeName, SaveOperator saveOp,
                                        LoadOperator loadOp);
    static void registerStreamOperators(int type, SaveOperator saveOp,
                                        LoadOperator loadOp);
#endif
    static int registerType(const char *typeName, Deleter deleter,
                            Creator creator);
    static int registerType(const char *typeName, Deleter deleter,
                            Creator creator,
                            Destructor destructor,
                            Constructor constructor,
                            int size,
                            QMetaType::TypeFlags flags,
                            const QMetaObject *metaObject);
    static int registerNormalizedType(const QT_PREPEND_NAMESPACE(QByteArray) &normalizedTypeName, Deleter deleter,
                            Creator creator,
                            Destructor destructor,
                            Constructor constructor,
                            int size,
                            QMetaType::TypeFlags flags,
                            const QMetaObject *metaObject);
    static int registerTypedef(const char *typeName, int aliasId);
    static int registerNormalizedTypedef(const QT_PREPEND_NAMESPACE(QByteArray) &normalizedTypeName, int aliasId);
    static int type(const char *typeName);
    static const char *typeName(int type);
    static int sizeOf(int type);
    static TypeFlags typeFlags(int type);
    static const QMetaObject *metaObjectForType(int type);
    static bool isRegistered(int type);
    static void *create(int type, const void *copy = 0);
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static void *construct(int type, const void *copy = 0)
    { return create(type, copy); }
#endif
    static void destroy(int type, void *data);
    static void *construct(int type, void *where, const void *copy);
    static void destruct(int type, void *where);

#ifndef QT_NO_DATASTREAM
    static bool save(QDataStream &stream, int type, const void *data);
    static bool load(QDataStream &stream, int type, void *data);
#endif

    explicit QMetaType(const int type);
    inline ~QMetaType();

    inline bool isValid() const;
    inline bool isRegistered() const;
    inline int sizeOf() const;
    inline TypeFlags flags() const;
    inline const QMetaObject *metaObject() const;

    inline void *create(const void *copy = 0) const;
    inline void destroy(void *data) const;
    inline void *construct(void *where, const void *copy = 0) const;
    inline void destruct(void *data) const;
private:
    static QMetaType typeInfo(const int type);
    inline QMetaType(const ExtensionFlag extensionFlags, const QMetaTypeInterface *info,
                     Creator creator,
                     Deleter deleter,
                     SaveOperator saveOp,
                     LoadOperator loadOp,
                     Constructor constructor,
                     Destructor destructor,
                     uint sizeOf,
                     uint theTypeFlags,
                     int typeId,
                     const QMetaObject *metaObject);
    QMetaType(const QMetaType &other);
    QMetaType &operator =(const QMetaType &);
    inline bool isExtended(const ExtensionFlag flag) const { return m_extensionFlags & flag; }

    // Methods used for future binary compatible extensions
    void ctor(const QMetaTypeInterface *info);
    void dtor();
    uint sizeExtended() const;
    QMetaType::TypeFlags flagsExtended() const;
    const QMetaObject *metaObjectExtended() const;
    void *createExtended(const void *copy = 0) const;
    void destroyExtended(void *data) const;
    void *constructExtended(void *where, const void *copy = 0) const;
    void destructExtended(void *data) const;

    Creator m_creator;
    Deleter m_deleter;
    SaveOperator m_saveOp;
    LoadOperator m_loadOp;
    Constructor m_constructor;
    Destructor m_destructor;
    void *m_extension; // space reserved for future use
    uint m_size;
    uint m_typeFlags;
    uint m_extensionFlags;
    int m_typeId;
    const QMetaObject *m_metaObject;
};

#undef QT_DEFINE_METATYPE_ID

Q_DECLARE_OPERATORS_FOR_FLAGS(QMetaType::TypeFlags)

namespace QtMetaTypePrivate {
template <typename T, bool Accepted = true>
struct QMetaTypeFunctionHelper {
    static void Delete(void *t)
    {
        delete static_cast<T*>(t);
    }

    static void *Create(const void *t)
    {
        if (t)
            return new T(*static_cast<const T*>(t));
        return new T();
    }

    static void Destruct(void *t)
    {
        Q_UNUSED(t) // Silence MSVC that warns for POD types.
        static_cast<T*>(t)->~T();
    }

    static void *Construct(void *where, const void *t)
    {
        if (t)
            return new (where) T(*static_cast<const T*>(t));
        return new (where) T;
    }
#ifndef QT_NO_DATASTREAM
    static void Save(QDataStream &stream, const void *t)
    {
        stream << *static_cast<const T*>(t);
    }

    static void Load(QDataStream &stream, void *t)
    {
        stream >> *static_cast<T*>(t);
    }
#endif // QT_NO_DATASTREAM
};

template <typename T>
struct QMetaTypeFunctionHelper<T, /* Accepted */ false> {
    static void Delete(void *) {}
    static void *Create(const void *) { return 0; }
    static void Destruct(void *) {}
    static void *Construct(void *, const void *) { return 0; }
#ifndef QT_NO_DATASTREAM
    static void Save(QDataStream &, const void *) {}
    static void Load(QDataStream &, void *) {}
#endif // QT_NO_DATASTREAM
};
template <>
struct QMetaTypeFunctionHelper<void, /* Accepted */ true>
        : public QMetaTypeFunctionHelper<void, /* Accepted */ false>
{};
}

class QObject;
class QWidget;
template <class T> class QSharedPointer;
template <class T> class QWeakPointer;
template <class T> class QPointer;

namespace QtPrivate
{
    template<typename T>
    struct IsPointerToTypeDerivedFromQObject
    {
        enum { Value = false };
    };

    // Specialize to avoid sizeof(void) warning
    template<>
    struct IsPointerToTypeDerivedFromQObject<void*>
    {
        enum { Value = false };
    };
    template<>
    struct IsPointerToTypeDerivedFromQObject<QObject*>
    {
        enum { Value = true };
    };

    template<typename T>
    struct IsPointerToTypeDerivedFromQObject<T*>
    {
        typedef qint8 yes_type;
        typedef qint64 no_type;

#ifndef QT_NO_QOBJECT
        static yes_type checkType(QObject* );
#endif
        static no_type checkType(...);
        Q_STATIC_ASSERT_X(sizeof(T), "Type argument of Q_DECLARE_METATYPE(T*) must be fully defined");
        enum { Value = sizeof(checkType(static_cast<T*>(0))) == sizeof(yes_type) };
    };

    template<typename T, bool = IsPointerToTypeDerivedFromQObject<T>::Value>
    struct MetaObjectForType
    {
        static inline const QMetaObject *value() { return 0; }
    };

    template<typename T>
    struct MetaObjectForType<T*, /* isPointerToTypeDerivedFromQObject = */ true>
    {
        static inline const QMetaObject *value() { return &T::staticMetaObject; }
    };

    template<typename T>
    struct IsSharedPointerToTypeDerivedFromQObject
    {
        enum { Value = false };
    };

    template<typename T>
    struct IsSharedPointerToTypeDerivedFromQObject<QSharedPointer<T> > : IsPointerToTypeDerivedFromQObject<T*>
    {
    };

    template<typename T>
    struct IsWeakPointerToTypeDerivedFromQObject
    {
        enum { Value = false };
    };

    template<typename T>
    struct IsWeakPointerToTypeDerivedFromQObject<QWeakPointer<T> > : IsPointerToTypeDerivedFromQObject<T*>
    {
    };

    template<typename T>
    struct IsTrackingPointerToTypeDerivedFromQObject
    {
        enum { Value = false };
    };

    template<typename T>
    struct IsTrackingPointerToTypeDerivedFromQObject<QPointer<T> >
    {
        enum { Value = true };
    };

    Q_CORE_EXPORT bool isBuiltinType(const QByteArray &type);
} // namespace QtPrivate

template <typename T, bool = QtPrivate::IsPointerToTypeDerivedFromQObject<T>::Value>
struct QMetaTypeIdQObject
{
    enum {
        Defined = 0
    };
};

template <typename T>
struct QMetaTypeId : public QMetaTypeIdQObject<T>
{
};

template <typename T>
struct QMetaTypeId2
{
    enum { Defined = QMetaTypeId<T>::Defined, IsBuiltIn=false };
    static inline Q_DECL_CONSTEXPR int qt_metatype_id() { return QMetaTypeId<T>::qt_metatype_id(); }
};

template <typename T>
struct QMetaTypeId2<const T&> : QMetaTypeId2<T> {};

namespace QtPrivate {
    template <typename T, bool Defined = QMetaTypeId2<T>::Defined>
    struct QMetaTypeIdHelper {
        static inline Q_DECL_CONSTEXPR int qt_metatype_id()
        { return QMetaTypeId2<T>::qt_metatype_id(); }
    };
    template <typename T> struct QMetaTypeIdHelper<T, false> {
        static inline Q_DECL_CONSTEXPR int qt_metatype_id()
        { return -1; }
    };

    // Function pointers don't derive from QObject
    template <class Result> struct IsPointerToTypeDerivedFromQObject<Result(*)()> { enum { Value = false }; };
    template <class Result, class Arg0> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0)> { enum { Value = false }; };
    template <class Result, class Arg0, class Arg1> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0, Arg1)> { enum { Value = false }; };
    template <class Result, class Arg0, class Arg1, class Arg2> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0, Arg1, Arg2)> { enum { Value = false }; };

    template<typename T>
    struct QMetaTypeTypeFlags
    {
        enum { Flags = (!QTypeInfo<T>::isStatic ? QMetaType::MovableType : 0)
                     | (QTypeInfo<T>::isComplex ? QMetaType::NeedsConstruction : 0)
                     | (QTypeInfo<T>::isComplex ? QMetaType::NeedsDestruction : 0)
                     | (IsPointerToTypeDerivedFromQObject<T>::Value ? QMetaType::PointerToQObject : 0)
                     | (IsSharedPointerToTypeDerivedFromQObject<T>::Value ? QMetaType::SharedPointerToQObject : 0)
                     | (IsWeakPointerToTypeDerivedFromQObject<T>::Value ? QMetaType::WeakPointerToQObject : 0)
                     | (IsTrackingPointerToTypeDerivedFromQObject<T>::Value ? QMetaType::TrackingPointerToQObject : 0)
                     | (Q_IS_ENUM(T) ? QMetaType::IsEnumeration : 0)
             };
    };

    template<typename T, bool defined>
    struct MetaTypeDefinedHelper
    {
        enum DefinedType { Defined = defined };
    };
}

template <typename T>
int qRegisterNormalizedMetaType(const QT_PREPEND_NAMESPACE(QByteArray) &normalizedTypeName
#ifndef Q_QDOC
    , T * dummy = 0
    , typename QtPrivate::MetaTypeDefinedHelper<T, QMetaTypeId2<T>::Defined && !QMetaTypeId2<T>::IsBuiltIn>::DefinedType defined = QtPrivate::MetaTypeDefinedHelper<T, QMetaTypeId2<T>::Defined && !QMetaTypeId2<T>::IsBuiltIn>::Defined
#endif
)
{
#ifndef QT_NO_QOBJECT
    Q_ASSERT_X(normalizedTypeName == QMetaObject::normalizedType(normalizedTypeName.constData()), "qRegisterNormalizedMetaType", "qRegisterNormalizedMetaType was called with a not normalized type name, please call qRegisterMetaType instead.");
#endif
    const int typedefOf = dummy ? -1 : QtPrivate::QMetaTypeIdHelper<T>::qt_metatype_id();
    if (typedefOf != -1)
        return QMetaType::registerNormalizedTypedef(normalizedTypeName, typedefOf);

    QMetaType::TypeFlags flags(QtPrivate::QMetaTypeTypeFlags<T>::Flags);

    if (defined)
        flags |= QMetaType::WasDeclaredAsMetaType;

    return QMetaType::registerNormalizedType(normalizedTypeName,
                                   QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Delete,
                                   QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Create,
                                   QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Destruct,
                                   QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Construct,
                                   int(sizeof(T)),
                                   flags,
                                   QtPrivate::MetaObjectForType<T>::value());
}

template <typename T>
int qRegisterMetaType(const char *typeName
#ifndef Q_QDOC
    , T * dummy = 0
    , typename QtPrivate::MetaTypeDefinedHelper<T, QMetaTypeId2<T>::Defined && !QMetaTypeId2<T>::IsBuiltIn>::DefinedType defined = QtPrivate::MetaTypeDefinedHelper<T, QMetaTypeId2<T>::Defined && !QMetaTypeId2<T>::IsBuiltIn>::Defined
#endif
)
{
#ifdef QT_NO_QOBJECT
    QT_PREPEND_NAMESPACE(QByteArray) normalizedTypeName = typeName;
#else
    QT_PREPEND_NAMESPACE(QByteArray) normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif
    return qRegisterNormalizedMetaType<T>(normalizedTypeName, dummy, defined);
}

#ifndef QT_NO_DATASTREAM
template <typename T>
void qRegisterMetaTypeStreamOperators(const char *typeName
#ifndef Q_QDOC
    , T * /* dummy */ = 0
#endif
)
{
    qRegisterMetaType<T>(typeName);
    QMetaType::registerStreamOperators(typeName, QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Save,
                                                 QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Load);
}
#endif // QT_NO_DATASTREAM

template <typename T>
inline Q_DECL_CONSTEXPR int qMetaTypeId()
{
    Q_STATIC_ASSERT_X(QMetaTypeId2<T>::Defined, "Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object system");
    return QMetaTypeId2<T>::qt_metatype_id();
}

template <typename T>
inline Q_DECL_CONSTEXPR int qRegisterMetaType()
{
    return qMetaTypeId<T>();
}

#if QT_DEPRECATED_SINCE(5, 1) && !defined(Q_QDOC)
// There used to be a T *dummy = 0 argument in Qt 4.0 to support MSVC6
template <typename T>
QT_DEPRECATED inline Q_DECL_CONSTEXPR int qMetaTypeId(T *)
{ return qMetaTypeId<T>(); }
#ifndef Q_CC_SUN
template <typename T>
QT_DEPRECATED inline Q_DECL_CONSTEXPR int qRegisterMetaType(T *)
{ return qRegisterMetaType<T>(); }
#endif
#endif

template <typename T>
struct QMetaTypeIdQObject<T*, /* isPointerToTypeDerivedFromQObject */ true>
{
    enum {
        Defined = 1
    };

    static int qt_metatype_id()
    {
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0);
        if (const int id = metatype_id.loadAcquire())
            return id;
        const char * const cName = T::staticMetaObject.className();
        QByteArray typeName;
        typeName.reserve(int(strlen(cName)) + 1);
        typeName.append(cName).append('*');
        const int newId = qRegisterNormalizedMetaType<T*>(
                        typeName,
                        reinterpret_cast<T**>(quintptr(-1)));
        metatype_id.storeRelease(newId);
        return newId;
    }
};

#ifndef QT_NO_DATASTREAM
template <typename T>
inline int qRegisterMetaTypeStreamOperators()
{
    int id = qMetaTypeId<T>();
    QMetaType::registerStreamOperators(id, QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Save,
                                           QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Load);
    return id;
}
#endif

#define Q_DECLARE_OPAQUE_POINTER(POINTER)                               \
    QT_BEGIN_NAMESPACE namespace QtPrivate {                            \
        template <>                                                     \
        struct IsPointerToTypeDerivedFromQObject<POINTER >              \
        {                                                               \
            enum { Value = false };                                     \
        };                                                              \
    } QT_END_NAMESPACE                                                  \
    /**/

#define Q_DECLARE_METATYPE(TYPE)                                        \
    QT_BEGIN_NAMESPACE                                                  \
    template <>                                                         \
    struct QMetaTypeId< TYPE >                                          \
    {                                                                   \
        enum { Defined = 1 };                                           \
        static int qt_metatype_id()                                     \
            {                                                           \
                static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0); \
                if (const int id = metatype_id.loadAcquire())           \
                    return id;                                          \
                const int newId = qRegisterMetaType< TYPE >(#TYPE,      \
                              reinterpret_cast< TYPE *>(quintptr(-1))); \
                metatype_id.storeRelease(newId);                        \
                return newId;                                           \
            }                                                           \
    };                                                                  \
    QT_END_NAMESPACE


#define Q_DECLARE_BUILTIN_METATYPE(TYPE, METATYPEID, NAME) \
    QT_BEGIN_NAMESPACE \
    template<> struct QMetaTypeId2<NAME> \
    { \
        enum { Defined = 1, IsBuiltIn = true, MetaType = METATYPEID };   \
        static inline Q_DECL_CONSTEXPR int qt_metatype_id() { return METATYPEID; } \
    }; \
    QT_END_NAMESPACE

#define QT_FORWARD_DECLARE_STATIC_TYPES_ITER(TypeName, TypeId, Name) \
    class Name;

QT_FOR_EACH_STATIC_CORE_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)
QT_FOR_EACH_STATIC_GUI_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)

#undef QT_FORWARD_DECLARE_STATIC_TYPES_ITER

typedef QList<QVariant> QVariantList;
typedef QMap<QString, QVariant> QVariantMap;
typedef QHash<QString, QVariant> QVariantHash;

#define Q_DECLARE_METATYPE_TEMPLATE_1ARG(SINGLE_ARG_TEMPLATE) \
template <typename T> \
struct QMetaTypeId< SINGLE_ARG_TEMPLATE<T> > \
{ \
    enum { \
        Defined = QMetaTypeId2<T>::Defined \
    }; \
    static int qt_metatype_id() \
    { \
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0); \
        if (const int id = metatype_id.load()) \
            return id; \
        const char *tName = QMetaType::typeName(qMetaTypeId<T>()); \
        Q_ASSERT(tName); \
        const int tNameLen = qstrlen(tName); \
        QByteArray typeName; \
        typeName.reserve(int(sizeof(#SINGLE_ARG_TEMPLATE)) + 1 + tNameLen + 1 + 1); \
        typeName.append(#SINGLE_ARG_TEMPLATE, int(sizeof(#SINGLE_ARG_TEMPLATE)) - 1) \
            .append('<').append(tName, tNameLen); \
        if (typeName.endsWith('>')) \
            typeName.append(' '); \
        typeName.append('>'); \
        const int newId = qRegisterNormalizedMetaType< SINGLE_ARG_TEMPLATE<T> >( \
                        typeName, \
                        reinterpret_cast< SINGLE_ARG_TEMPLATE<T> *>(quintptr(-1))); \
        metatype_id.storeRelease(newId); \
        return newId; \
    } \
};

#define Q_DECLARE_METATYPE_TEMPLATE_2ARG(DOUBLE_ARG_TEMPLATE) \
template<typename T, typename U> \
struct QMetaTypeId< DOUBLE_ARG_TEMPLATE<T, U> > \
{ \
    enum { \
        Defined = QMetaTypeId2<T>::Defined && QMetaTypeId2<U>::Defined \
    }; \
    static int qt_metatype_id() \
    { \
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0); \
        if (const int id = metatype_id.loadAcquire()) \
            return id; \
        const char *tName = QMetaType::typeName(qMetaTypeId<T>()); \
        const char *uName = QMetaType::typeName(qMetaTypeId<U>()); \
        Q_ASSERT(tName); \
        Q_ASSERT(uName); \
        const int tNameLen = qstrlen(tName); \
        const int uNameLen = qstrlen(uName); \
        QByteArray typeName; \
        typeName.reserve(int(sizeof(#DOUBLE_ARG_TEMPLATE)) + 1 + tNameLen + 1 + uNameLen + 1 + 1); \
        typeName.append(#DOUBLE_ARG_TEMPLATE, int(sizeof(#DOUBLE_ARG_TEMPLATE)) - 1) \
            .append('<').append(tName, tNameLen).append(',').append(uName, uNameLen); \
        if (typeName.endsWith('>')) \
            typeName.append(' '); \
        typeName.append('>'); \
        const int newId = qRegisterNormalizedMetaType< DOUBLE_ARG_TEMPLATE<T, U> >(\
                        typeName, \
                        reinterpret_cast< DOUBLE_ARG_TEMPLATE<T, U> *>(quintptr(-1))); \
        metatype_id.storeRelease(newId); \
        return newId; \
    } \
};

#define Q_DECLARE_SMART_POINTER_METATYPE(SMART_POINTER) \
template <typename T, bool = QtPrivate::IsPointerToTypeDerivedFromQObject<T*>::Value> \
struct QMetaTypeId_ ## SMART_POINTER ## _QObjectStar \
{ \
    enum { \
        Defined = 0 \
    }; \
};\
 \
template <typename T> \
struct QMetaTypeId_ ## SMART_POINTER ## _QObjectStar<T, true> \
{ \
    enum { \
        Defined = 1 \
    }; \
    static int qt_metatype_id() \
    { \
        static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0); \
        if (const int id = metatype_id.loadAcquire()) \
            return id; \
        const char * const cName = T::staticMetaObject.className(); \
        QByteArray typeName; \
        typeName.reserve(int(sizeof(#SMART_POINTER) + 1 + strlen(cName) + 1)); \
        typeName.append(#SMART_POINTER, int(sizeof(#SMART_POINTER)) - 1) \
            .append('<').append(cName).append('>'); \
        const int newId = qRegisterNormalizedMetaType< SMART_POINTER<T> >( \
                        typeName, \
                        reinterpret_cast< SMART_POINTER<T> *>(quintptr(-1))); \
        metatype_id.storeRelease(newId); \
        return newId; \
    } \
}; \
\
template <typename T> \
struct QMetaTypeId< SMART_POINTER<T> > : public QMetaTypeId_ ## SMART_POINTER ## _QObjectStar<T> \
{ \
};

#define QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(F) \
    F(QList) \
    F(QVector) \
    F(QQueue) \
    F(QStack) \
    F(QSet) \
    F(QLinkedList)

#define QT_FOR_EACH_AUTOMATIC_TEMPLATE_2ARG(F) \
    F(QHash, class) \
    F(QMap, class) \
    F(QPair, struct)

#define QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(F) \
    F(QSharedPointer) \
    F(QWeakPointer) \
    F(QPointer)

#define Q_DECLARE_METATYPE_TEMPLATE_1ARG_ITER(TEMPLATENAME) \
    template <class T> class TEMPLATENAME; \
    Q_DECLARE_METATYPE_TEMPLATE_1ARG(TEMPLATENAME)

QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(Q_DECLARE_METATYPE_TEMPLATE_1ARG_ITER)

#undef Q_DECLARE_METATYPE_TEMPLATE_1ARG_ITER

#define Q_DECLARE_METATYPE_TEMPLATE_2ARG_ITER(TEMPLATENAME, CPPTYPE) \
    template <class T1, class T2> CPPTYPE TEMPLATENAME; \
    Q_DECLARE_METATYPE_TEMPLATE_2ARG(TEMPLATENAME)

QT_FOR_EACH_AUTOMATIC_TEMPLATE_2ARG(Q_DECLARE_METATYPE_TEMPLATE_2ARG_ITER)

#undef Q_DECLARE_METATYPE_TEMPLATE_2ARG_ITER

#define Q_DECLARE_METATYPE_TEMPLATE_SMART_POINTER_ITER(TEMPLATENAME) \
    Q_DECLARE_SMART_POINTER_METATYPE(TEMPLATENAME)

QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(Q_DECLARE_METATYPE_TEMPLATE_SMART_POINTER_ITER)

#undef Q_DECLARE_METATYPE_TEMPLATE_SMART_POINTER_ITER

inline QMetaType::QMetaType(const ExtensionFlag extensionFlags, const QMetaTypeInterface *info,
                            Creator creator,
                            Deleter deleter,
                            SaveOperator saveOp,
                            LoadOperator loadOp,
                            Constructor constructor,
                            Destructor destructor,
                            uint size,
                            uint theTypeFlags,
                            int typeId,
                            const QMetaObject *_metaObject)
    : m_creator(creator)
    , m_deleter(deleter)
    , m_saveOp(saveOp)
    , m_loadOp(loadOp)
    , m_constructor(constructor)
    , m_destructor(destructor)
    , m_extension(0)
    , m_size(size)
    , m_typeFlags(theTypeFlags)
    , m_extensionFlags(extensionFlags)
    , m_typeId(typeId)
    , m_metaObject(_metaObject)
{
    if (Q_UNLIKELY(isExtended(CtorEx) || typeId == QMetaType::Void))
        ctor(info);
}

inline QMetaType::~QMetaType()
{
    if (Q_UNLIKELY(isExtended(DtorEx)))
        dtor();
}

inline bool QMetaType::isValid() const
{
    return m_typeId != UnknownType;
}

inline bool QMetaType::isRegistered() const
{
    return isValid();
}

inline void *QMetaType::create(const void *copy) const
{
    if (Q_UNLIKELY(isExtended(CreateEx)))
        return createExtended(copy);
    return m_creator(copy);
}

inline void QMetaType::destroy(void *data) const
{
    if (Q_UNLIKELY(isExtended(DestroyEx)))
        return destroyExtended(data);
    m_deleter(data);
}

inline void *QMetaType::construct(void *where, const void *copy) const
{
    if (Q_UNLIKELY(isExtended(ConstructEx)))
        return constructExtended(where, copy);
    return m_constructor(where, copy);
}

inline void QMetaType::destruct(void *data) const
{
    if (Q_UNLIKELY(isExtended(DestructEx)))
        return destructExtended(data);
    if (Q_UNLIKELY(!data))
        return;
    m_destructor(data);
}

inline int QMetaType::sizeOf() const
{
    if (Q_UNLIKELY(isExtended(SizeEx)))
        return sizeExtended();
    return m_size;
}

inline QMetaType::TypeFlags QMetaType::flags() const
{
    if (Q_UNLIKELY(isExtended(FlagsEx)))
        return flagsExtended();
    return QMetaType::TypeFlags(m_typeFlags);
}

inline const QMetaObject *QMetaType::metaObject() const
{
    if (Q_UNLIKELY(isExtended(MetaObjectEx)))
        return metaObjectExtended();
    return m_metaObject;
}

QT_END_NAMESPACE


QT_FOR_EACH_STATIC_TYPE(Q_DECLARE_BUILTIN_METATYPE)


#endif // QMETATYPE_H
