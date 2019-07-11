/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Used by both tst_qmetatype and tst_qsettings

#ifndef TST_QMETATYPE_H
#define TST_QMETATYPE_H

#include <qmetatype.h>
#include <float.h>

#define FOR_EACH_PRIMITIVE_METATYPE(F) \
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F) \
    QT_FOR_EACH_STATIC_CORE_POINTER(F) \

#define FOR_EACH_COMPLEX_CORE_METATYPE(F) \
    QT_FOR_EACH_STATIC_CORE_CLASS(F) \
    QT_FOR_EACH_STATIC_CORE_TEMPLATE(F)

#define FOR_EACH_CORE_METATYPE(F) \
    FOR_EACH_PRIMITIVE_METATYPE(F) \
    FOR_EACH_COMPLEX_CORE_METATYPE(F) \

template <int ID>
struct MetaEnumToType {};

#define DEFINE_META_ENUM_TO_TYPE(MetaTypeName, MetaTypeId, RealType) \
template<> \
struct MetaEnumToType<QMetaType::MetaTypeName> { \
    typedef RealType Type; \
};
FOR_EACH_CORE_METATYPE(DEFINE_META_ENUM_TO_TYPE)
#undef DEFINE_META_ENUM_TO_TYPE

template <int ID>
struct DefaultValueFactory
{
    typedef typename MetaEnumToType<ID>::Type Type;
    static Type *create() { return new Type; }
};

template <>
struct DefaultValueFactory<QMetaType::Void>
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    static Type *create() { return 0; }
};

template <int ID>
struct DefaultValueTraits
{
    // By default we assume that a default-constructed value (new T) is
    // initialized; e.g. QCOMPARE(*(new T), *(new T)) should succeed
    enum { IsInitialized = true };
};

#define DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS(MetaTypeName, MetaTypeId, RealType) \
template<> struct DefaultValueTraits<QMetaType::MetaTypeName> { \
    enum { IsInitialized = false }; \
};
// Primitive types (int et al) aren't initialized
FOR_EACH_PRIMITIVE_METATYPE(DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS)
#undef DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS

template <int ID>
struct TestValueFactory {};

template<> struct TestValueFactory<QMetaType::Void> {
    static void *create() { return 0; }
};

template<> struct TestValueFactory<QMetaType::QString> {
    static QString *create() { return new QString(QString::fromLatin1("QString")); }
};
template<> struct TestValueFactory<QMetaType::Int> {
    static int *create() { return new int(INT_MIN); }
};
template<> struct TestValueFactory<QMetaType::UInt> {
    static uint *create() { return new uint(UINT_MAX); }
};
template<> struct TestValueFactory<QMetaType::Bool> {
    static bool *create() { return new bool(true); }
};
template<> struct TestValueFactory<QMetaType::Double> {
    static double *create() { return new double(DBL_MIN); }
};
template<> struct TestValueFactory<QMetaType::QByteArray> {
    static QByteArray *create() { return new QByteArray(QByteArray("QByteArray")); }
};
template<> struct TestValueFactory<QMetaType::QByteArrayList> {
    static QByteArrayList *create() { return new QByteArrayList(QByteArrayList() << "Q" << "Byte" << "Array" << "List"); }
};
template<> struct TestValueFactory<QMetaType::QVariantMap> {
    static QVariantMap *create() { return new QVariantMap(); }
};
template<> struct TestValueFactory<QMetaType::QVariantHash> {
    static QVariantHash *create() { return new QVariantHash(); }
};
template<> struct TestValueFactory<QMetaType::QVariantList> {
    static QVariantList *create() { return new QVariantList(QVariantList() << 123 << "Q" << "Variant" << "List"); }
};
template<> struct TestValueFactory<QMetaType::QChar> {
    static QChar *create() { return new QChar(QChar('q')); }
};
template<> struct TestValueFactory<QMetaType::Long> {
    static long *create() { return new long(LONG_MIN); }
};
template<> struct TestValueFactory<QMetaType::Short> {
    static short *create() { return new short(SHRT_MIN); }
};
template<> struct TestValueFactory<QMetaType::Char> {
    static char *create() { return new char('c'); }
};
template<> struct TestValueFactory<QMetaType::ULong> {
    static ulong *create() { return new ulong(ULONG_MAX); }
};
template<> struct TestValueFactory<QMetaType::UShort> {
    static ushort *create() { return new ushort(USHRT_MAX); }
};
template<> struct TestValueFactory<QMetaType::SChar> {
    static signed char *create() { return new signed char(CHAR_MIN); }
};
template<> struct TestValueFactory<QMetaType::UChar> {
    static uchar *create() { return new uchar(UCHAR_MAX); }
};
template<> struct TestValueFactory<QMetaType::Float> {
    static float *create() { return new float(FLT_MIN); }
};
template<> struct TestValueFactory<QMetaType::QObjectStar> {
    static QObject * *create() { return new QObject *(0); }
};
template<> struct TestValueFactory<QMetaType::VoidStar> {
    static void * *create() { return new void *(0); }
};
template<> struct TestValueFactory<QMetaType::LongLong> {
    static qlonglong *create() { return new qlonglong(LLONG_MIN); }
};
template<> struct TestValueFactory<QMetaType::ULongLong> {
    static qulonglong *create() { return new qulonglong(ULLONG_MAX); }
};
template<> struct TestValueFactory<QMetaType::QStringList> {
    static QStringList *create() { return new QStringList(QStringList() << "Q" << "t"); }
};
template<> struct TestValueFactory<QMetaType::QBitArray> {
    static QBitArray *create() { return new QBitArray(QBitArray(256, true)); }
};
template<> struct TestValueFactory<QMetaType::QDate> {
    static QDate *create() { return new QDate(QDate::currentDate()); }
};
template<> struct TestValueFactory<QMetaType::QTime> {
    static QTime *create() { return new QTime(QTime::currentTime()); }
};
template<> struct TestValueFactory<QMetaType::QDateTime> {
    static QDateTime *create() { return new QDateTime(QDateTime::currentDateTime()); }
};
template<> struct TestValueFactory<QMetaType::QUrl> {
    static QUrl *create() { return new QUrl("http://www.example.org"); }
};
template<> struct TestValueFactory<QMetaType::QLocale> {
    static QLocale *create() { return new QLocale(QLocale::c()); }
};
template<> struct TestValueFactory<QMetaType::QRect> {
    static QRect *create() { return new QRect(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QRectF> {
    static QRectF *create() { return new QRectF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QSize> {
    static QSize *create() { return new QSize(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QSizeF> {
    static QSizeF *create() { return new QSizeF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QLine> {
    static QLine *create() { return new QLine(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QLineF> {
    static QLineF *create() { return new QLineF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QPoint> {
    static QPoint *create() { return new QPoint(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QPointF> {
    static QPointF *create() { return new QPointF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QEasingCurve> {
    static QEasingCurve *create() { return new QEasingCurve(QEasingCurve::InOutElastic); }
};
template<> struct TestValueFactory<QMetaType::QUuid> {
    static QUuid *create() { return new QUuid(); }
};
template<> struct TestValueFactory<QMetaType::QModelIndex> {
    static QModelIndex *create() { return new QModelIndex(); }
};
template<> struct TestValueFactory<QMetaType::QPersistentModelIndex> {
    static QPersistentModelIndex *create() { return new QPersistentModelIndex(); }
};
template<> struct TestValueFactory<QMetaType::Nullptr> {
    static std::nullptr_t *create() { return new std::nullptr_t; }
};
template<> struct TestValueFactory<QMetaType::QRegExp> {
    static QRegExp *create()
    {
#ifndef QT_NO_REGEXP
        return new QRegExp("A*");
#else
        return 0;
#endif
    }
};
template<> struct TestValueFactory<QMetaType::QRegularExpression> {
    static QRegularExpression *create()
    {
#if QT_CONFIG(regularexpression)
        return new QRegularExpression("abc.*def");
#else
        return 0;
#endif
    }
};
template<> struct TestValueFactory<QMetaType::QJsonValue> {
    static QJsonValue *create() { return new QJsonValue(123.); }
};
template<> struct TestValueFactory<QMetaType::QJsonObject> {
    static QJsonObject *create() {
        QJsonObject *o = new QJsonObject();
        o->insert("a", 123.);
        o->insert("b", true);
        o->insert("c", QJsonValue::Null);
        o->insert("d", QLatin1String("ciao"));
        return o;
    }
};
template<> struct TestValueFactory<QMetaType::QJsonArray> {
    static QJsonArray *create() {
        QJsonArray *a = new QJsonArray();
        a->append(123.);
        a->append(true);
        a->append(QJsonValue::Null);
        a->append(QLatin1String("ciao"));
        return a;
    }
};
template<> struct TestValueFactory<QMetaType::QJsonDocument> {
    static QJsonDocument *create() {
        return new QJsonDocument(
            QJsonDocument::fromJson("{ 'foo': 123, 'bar': [true, null, 'ciao'] }")
        );
    }
};

template<> struct TestValueFactory<QMetaType::QCborSimpleType> {
    static QCborSimpleType *create() { return new QCborSimpleType(QCborSimpleType::True); }
};
template<> struct TestValueFactory<QMetaType::QCborValue> {
    static QCborValue *create() { return new QCborValue(123.); }
};
template<> struct TestValueFactory<QMetaType::QCborMap> {
    static QCborMap *create() {
        return new QCborMap{{0, 0}, {"Hello", 1}, {1, nullptr}};
    }
};
template<> struct TestValueFactory<QMetaType::QCborArray> {
    static QCborArray *create() {
        return new QCborArray{0, 1, -2, 2.5, false, nullptr, "Hello", QByteArray("World") };
    }
};

template<> struct TestValueFactory<QMetaType::QVariant> {
    static QVariant *create() { return new QVariant(QStringList(QStringList() << "Q" << "t")); }
};

#endif // TST_QMETATYPE_H
