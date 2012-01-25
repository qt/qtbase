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

#ifndef QMETATYPE_H
#define QMETATYPE_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>

#include <new>

#ifdef Bool
#error qmetatype.h must be included before any header file that defines Bool
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


// F is a tuple: (QMetaType::TypeName, QMetaType::TypeNameID, RealType)
#define QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F)\
    F(Void, 0, void) \
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

#define QT_FOR_EACH_STATIC_CORE_POINTER(F)\
    F(QObjectStar, 39, QObject*) \
    F(QWidgetStar, 40, QWidget*) \

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
    F(QImage, 69, QImage) \
    F(QPolygon, 70, QPolygon) \
    F(QRegion, 71, QRegion) \
    F(QBitmap, 72, QBitmap) \
    F(QCursor, 73, QCursor) \
    F(QKeySequence, 74, QKeySequence) \
    F(QPen, 75, QPen) \
    F(QTextLength, 76, QTextLength) \
    F(QTextFormat, 77, QTextFormat) \
    F(QMatrix, 78, QMatrix) \
    F(QTransform, 79, QTransform) \
    F(QMatrix4x4, 80, QMatrix4x4) \
    F(QVector2D, 81, QVector2D) \
    F(QVector3D, 82, QVector3D) \
    F(QVector4D, 83, QVector4D) \
    F(QQuaternion, 84, QQuaternion) \
    F(QPolygonF, 85, QPolygonF) \

#define QT_FOR_EACH_STATIC_WIDGETS_CLASS(F)\
    F(QIcon, 120, QIcon) \
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
    F(Char, -1, char, "qint8") \
    F(Char, -1, char, "signed char") \
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

class Q_CORE_EXPORT QMetaType {
public:
    enum Type {
        // these are merged with QVariant
        QT_FOR_EACH_STATIC_TYPE(QT_DEFINE_METATYPE_ID)

        LastCoreType = QModelIndex,
        FirstGuiType = QFont,
        LastGuiType = QPolygonF,
        FirstWidgetsType = QIcon,
        LastWidgetsType = QSizePolicy,
        HighestInternalId = LastWidgetsType,

// This logic must match the one in qglobal.h
#if defined(QT_COORD_TYPE)
        QReal = 0,
#elif defined(QT_NO_FPU) || defined(QT_ARCH_ARM) || defined(QT_ARCH_WINDOWSCE)
        QReal = Float,
#else
        QReal = Double,
#endif

        User = 256
    };

    enum TypeFlag {
        NeedsConstruction = 0x1,
        NeedsDestruction = 0x2,
        MovableType = 0x4,
        PointerToQObject = 0x8
    };
    Q_DECLARE_FLAGS(TypeFlags, TypeFlag)

    typedef void (*Deleter)(void *);
    typedef void *(*Creator)(const void *);

    typedef void (*Destructor)(void *);
    typedef void *(*Constructor)(void *, const void *);

#ifndef QT_NO_DATASTREAM
    typedef void (*SaveOperator)(QDataStream &, const void *);
    typedef void (*LoadOperator)(QDataStream &, void *);
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
                            QMetaType::TypeFlags flags);
    static int registerTypedef(const char *typeName, int aliasId);
    static int type(const char *typeName);
    static const char *typeName(int type);
    static int sizeOf(int type);
    static TypeFlags typeFlags(int type);
    static bool isRegistered(int type);
    static void *create(int type, const void *copy = 0);
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED static void *construct(int type, const void *copy = 0)
    { return create(type, copy); }
#endif
    static void destroy(int type, void *data);
    static void *construct(int type, void *where, const void *copy);
    static void destruct(int type, void *where);
    static void unregisterType(const char *typeName);

#ifndef QT_NO_DATASTREAM
    static bool save(QDataStream &stream, int type, const void *data);
    static bool load(QDataStream &stream, int type, void *data);
#endif
};

#undef QT_DEFINE_METATYPE_ID

Q_DECLARE_OPERATORS_FOR_FLAGS(QMetaType::TypeFlags)

template <typename T>
void qMetaTypeDeleteHelper(T *t)
{
    delete t;
}

template <typename T>
void *qMetaTypeCreateHelper(const T *t)
{
    if (t)
        return new T(*static_cast<const T*>(t));
    return new T();
}

template <typename T>
void qMetaTypeDestructHelper(T *t)
{
    Q_UNUSED(t) // Silence MSVC that warns for POD types.
    t->~T();
}

template <typename T>
void *qMetaTypeConstructHelper(void *where, const T *t)
{
    if (t)
        return new (where) T(*static_cast<const T*>(t));
    return new (where) T;
}

#ifndef QT_NO_DATASTREAM
template <typename T>
void qMetaTypeSaveHelper(QDataStream &stream, const T *t)
{
    stream << *t;
}

template <typename T>
void qMetaTypeLoadHelper(QDataStream &stream, T *t)
{
    stream >> *t;
}
#endif // QT_NO_DATASTREAM

template <typename T>
struct QMetaTypeId
{
    enum { Defined = 0 };
};

template <typename T>
struct QMetaTypeId2
{
    enum { Defined = QMetaTypeId<T>::Defined };
    static inline int qt_metatype_id() { return QMetaTypeId<T>::qt_metatype_id(); }
};

class QObject;
class QWidget;

namespace QtPrivate {
    template <typename T, bool Defined = QMetaTypeId2<T>::Defined>
    struct QMetaTypeIdHelper {
        static inline int qt_metatype_id()
        { return QMetaTypeId2<T>::qt_metatype_id(); }
    };
    template <typename T> struct QMetaTypeIdHelper<T, false> {
        static inline int qt_metatype_id()
        { return -1; }
    };

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
    template<>
    struct IsPointerToTypeDerivedFromQObject<QWidget*>
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

    // Function pointers don't derive from QObject
    template <class Result> struct IsPointerToTypeDerivedFromQObject<Result(*)()> { enum { Value = false }; };
    template <class Result, class Arg0> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0)> { enum { Value = false }; };
    template <class Result, class Arg0, class Arg1> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0, Arg1)> { enum { Value = false }; };
    template <class Result, class Arg0, class Arg1, class Arg2> struct IsPointerToTypeDerivedFromQObject<Result(*)(Arg0, Arg1, Arg2)> { enum { Value = false }; };
}

template <typename T>
int qRegisterMetaType(const char *typeName
#ifndef qdoc
    , T * dummy = 0
#endif
)
{
    const int typedefOf = dummy ? -1 : QtPrivate::QMetaTypeIdHelper<T>::qt_metatype_id();
    if (typedefOf != -1)
        return QMetaType::registerTypedef(typeName, typedefOf);

    typedef void*(*CreatePtr)(const T*);
    CreatePtr cptr = qMetaTypeCreateHelper<T>;
    typedef void(*DeletePtr)(T*);
    DeletePtr dptr = qMetaTypeDeleteHelper<T>;
    typedef void*(*ConstructPtr)(void *, const T*);
    ConstructPtr ipcptr = qMetaTypeConstructHelper<T>;
    typedef void(*DestructPtr)(T*);
    DestructPtr ipdptr = qMetaTypeDestructHelper<T>;

    QMetaType::TypeFlags flags;
    if (!QTypeInfo<T>::isStatic)
        flags |= QMetaType::MovableType;
    if (QTypeInfo<T>::isComplex) {
        flags |= QMetaType::NeedsConstruction;
        flags |= QMetaType::NeedsDestruction;
    }
    if (QtPrivate::IsPointerToTypeDerivedFromQObject<T>::Value)
        flags |= QMetaType::PointerToQObject;

    return QMetaType::registerType(typeName, reinterpret_cast<QMetaType::Deleter>(dptr),
                                   reinterpret_cast<QMetaType::Creator>(cptr),
                                   reinterpret_cast<QMetaType::Destructor>(ipdptr),
                                   reinterpret_cast<QMetaType::Constructor>(ipcptr),
                                   sizeof(T),
                                   flags);
}

#ifndef QT_NO_DATASTREAM
template <typename T>
void qRegisterMetaTypeStreamOperators(const char *typeName
#ifndef qdoc
    , T * /* dummy */ = 0
#endif
)
{
    typedef void(*SavePtr)(QDataStream &, const T *);
    typedef void(*LoadPtr)(QDataStream &, T *);
    SavePtr sptr = qMetaTypeSaveHelper<T>;
    LoadPtr lptr = qMetaTypeLoadHelper<T>;

    qRegisterMetaType<T>(typeName);
    QMetaType::registerStreamOperators(typeName, reinterpret_cast<QMetaType::SaveOperator>(sptr),
                                       reinterpret_cast<QMetaType::LoadOperator>(lptr));
}
#endif // QT_NO_DATASTREAM

template <typename T>
inline int qMetaTypeId(
#ifndef qdoc
    T * /* dummy */ = 0
#endif
)
{
    return QMetaTypeId2<T>::qt_metatype_id();
}

template <typename T>
inline int qRegisterMetaType(
#if !defined(qdoc) && !defined(Q_CC_SUN)
    T * dummy = 0
#endif
)
{
#ifdef Q_CC_SUN
    return qMetaTypeId(static_cast<T *>(0));
#else
    return qMetaTypeId(dummy);
#endif
}

#ifndef QT_NO_DATASTREAM
template <typename T>
inline int qRegisterMetaTypeStreamOperators()
{
    typedef void(*SavePtr)(QDataStream &, const T *);
    typedef void(*LoadPtr)(QDataStream &, T *);
    SavePtr sptr = qMetaTypeSaveHelper<T>;
    LoadPtr lptr = qMetaTypeLoadHelper<T>;

    register int id = qMetaTypeId<T>();
    QMetaType::registerStreamOperators(id,
                                       reinterpret_cast<QMetaType::SaveOperator>(sptr),
                                       reinterpret_cast<QMetaType::LoadOperator>(lptr));

    return id;
}
#endif

#define Q_DECLARE_METATYPE(TYPE)                                        \
    QT_BEGIN_NAMESPACE                                                  \
    template <>                                                         \
    struct QMetaTypeId< TYPE >                                          \
    {                                                                   \
        enum { Defined = 1 };                                           \
        static int qt_metatype_id()                                     \
            {                                                           \
                static QBasicAtomicInt metatype_id = Q_BASIC_ATOMIC_INITIALIZER(0); \
                if (!metatype_id.load())                                \
                    metatype_id.storeRelease(qRegisterMetaType< TYPE >(#TYPE, \
                               reinterpret_cast< TYPE *>(quintptr(-1)))); \
                return metatype_id.loadAcquire();                       \
            }                                                           \
    };                                                                  \
    QT_END_NAMESPACE

#define Q_DECLARE_BUILTIN_METATYPE(TYPE, NAME) \
    QT_BEGIN_NAMESPACE \
    template<> struct QMetaTypeId2<TYPE> \
    { \
        enum { Defined = 1, MetaType = QMetaType::NAME }; \
        static inline int qt_metatype_id() { return QMetaType::NAME; } \
    }; \
    QT_END_NAMESPACE

#define QT_FORWARD_DECLARE_STATIC_TYPES_ITER(TypeName, TypeId, Name) \
    class Name;

QT_FOR_EACH_STATIC_CORE_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)
QT_FOR_EACH_STATIC_GUI_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_FORWARD_DECLARE_STATIC_TYPES_ITER)

#undef QT_FORWARD_DECLARE_STATIC_TYPES_ITER

template <class T> class QList;
template <class T> class QLinkedList;
template <class T> class QVector;
template <class T> class QQueue;
template <class T> class QStack;
template <class T> class QSet;
template <class T> class QSharedPointer;
template <class T1, class T2> class QMap;
template <class T1, class T2> class QHash;
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
        if (!metatype_id.load()) \
            metatype_id.storeRelease(qRegisterMetaType< SINGLE_ARG_TEMPLATE<T> >( QByteArray(QByteArray(#SINGLE_ARG_TEMPLATE "<") + QMetaType::typeName(qMetaTypeId<T>()) + ">"), \
                        reinterpret_cast< SINGLE_ARG_TEMPLATE<T> *>(quintptr(-1)))); \
        return metatype_id.loadAcquire(); \
    } \
};

Q_DECLARE_METATYPE_TEMPLATE_1ARG(QList)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QVector)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QQueue)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QStack)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QSet)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QSharedPointer)
Q_DECLARE_METATYPE_TEMPLATE_1ARG(QLinkedList)

QT_END_NAMESPACE


#define QT_DECLARE_BUILTIN_METATYPE_ITER(MetaTypeName, MetaTypeId, Name) \
    Q_DECLARE_BUILTIN_METATYPE(Name, MetaTypeName)

QT_FOR_EACH_STATIC_TYPE(QT_DECLARE_BUILTIN_METATYPE_ITER)
Q_DECLARE_BUILTIN_METATYPE(signed char, Char)

#undef QT_DECLARE_BUILTIN_METATYPE_ITER


QT_END_HEADER

#endif // QMETATYPE_H
