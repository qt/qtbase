/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
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

#include "qvariant.h"
#include "qbitarray.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qmap.h"
#include "qdatetime.h"
#include "qeasingcurve.h"
#include "qlist.h"
#if QT_CONFIG(regularexpression)
#include "qregularexpression.h"
#endif
#include "qstring.h"
#include "qstringlist.h"
#include "qurl.h"
#include "qlocale.h"
#include "quuid.h"
#if QT_CONFIG(itemmodel)
#include "qabstractitemmodel.h"
#endif
#ifndef QT_BOOTSTRAPPED
#include "qjsonvalue.h"
#include "qjsonobject.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "qbytearraylist.h"
#endif
#include "private/qvariant_p.h"
#include "private/qlocale_p.h"
#include "qmetatype_p.h"
#include <qmetaobject.h>

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

#include <cmath>
#include <float.h>
#include <cstring>

QT_BEGIN_NAMESPACE

namespace {
class HandlersManager
{
    static const QVariant::Handler *Handlers[QModulesPrivate::ModulesCount];
public:
    const QVariant::Handler *operator[] (const uint typeId) const
    {
        return Handlers[QModulesPrivate::moduleForType(typeId)];
    }

    void registerHandler(const QModulesPrivate::Names name, const QVariant::Handler *handler)
    {
        Handlers[name] = handler;
    }
};
}  // namespace

namespace {
struct CoreTypesFilter {
    template<typename T>
    struct Acceptor {
        static const bool IsAccepted = QModulesPrivate::QTypeModuleInfo<T>::IsCore && QtMetaTypePrivate::TypeDefinition<T>::IsAvailable;
    };
};
} // annonymous

namespace { // annonymous used to hide QVariant handlers

static void construct(QVariant::Private *x, const void *copy)
{
    QVariantConstructor<CoreTypesFilter> constructor(x, copy);
    QMetaTypeSwitcher::switcher<void>(constructor, x->type, 0);
}

static void clear(QVariant::Private *d)
{
    QVariantDestructor<CoreTypesFilter> cleaner(d);
    QMetaTypeSwitcher::switcher<void>(cleaner, d->type, 0);
}

static bool isNull(const QVariant::Private *d)
{
    QVariantIsNull<CoreTypesFilter> isNull(d);
    return QMetaTypeSwitcher::switcher<bool>(isNull, d->type, 0);
}

/*!
  \internal

  Compares \a a to \a b. The caller guarantees that \a a and \a b
  are of the same type.
 */
static bool compare(const QVariant::Private *a, const QVariant::Private *b)
{
    QVariantComparator<CoreTypesFilter> comparator(a, b);
    return QMetaTypeSwitcher::switcher<bool>(comparator, a->type, 0);
}

/*!
  \internal
 */
static qlonglong qMetaTypeNumber(const QVariant::Private *d)
{
    switch (d->type) {
    case QMetaType::Int:
        return d->data.i;
    case QMetaType::LongLong:
        return d->data.ll;
    case QMetaType::Char:
        return qlonglong(d->data.c);
    case QMetaType::SChar:
        return qlonglong(d->data.sc);
    case QMetaType::Short:
        return qlonglong(d->data.s);
    case QMetaType::Long:
        return qlonglong(d->data.l);
    case QMetaType::Float:
        return qRound64(d->data.f);
    case QVariant::Double:
        return qRound64(d->data.d);
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        return v_cast<QJsonValue>(d)->toDouble();
#endif
    }
    Q_ASSERT(false);
    return 0;
}

static qulonglong qMetaTypeUNumber(const QVariant::Private *d)
{
    switch (d->type) {
    case QVariant::UInt:
        return d->data.u;
    case QVariant::ULongLong:
        return d->data.ull;
    case QMetaType::UChar:
        return d->data.uc;
    case QMetaType::UShort:
        return d->data.us;
    case QMetaType::ULong:
        return d->data.ul;
    }
    Q_ASSERT(false);
    return 0;
}

static qlonglong qConvertToNumber(const QVariant::Private *d, bool *ok)
{
    *ok = true;

    switch (uint(d->type)) {
    case QVariant::String:
        return v_cast<QString>(d)->toLongLong(ok);
    case QVariant::Char:
        return v_cast<QChar>(d)->unicode();
    case QVariant::ByteArray:
        return v_cast<QByteArray>(d)->toLongLong(ok);
    case QVariant::Bool:
        return qlonglong(d->data.b);
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        if (!v_cast<QJsonValue>(d)->isDouble())
            break;
        Q_FALLTHROUGH();
#endif
    case QVariant::Double:
    case QVariant::Int:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
        return qMetaTypeNumber(d);
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:

        return qlonglong(qMetaTypeUNumber(d));
    }

    QMetaType typeInfo(d->type);
    if (typeInfo.flags() & QMetaType::IsEnumeration) {
        switch (typeInfo.sizeOf()) {
        case 1:
            return d->is_shared ? *reinterpret_cast<signed char *>(d->data.shared->ptr) : d->data.sc;
        case 2:
            return d->is_shared ? *reinterpret_cast<qint16 *>(d->data.shared->ptr) : d->data.s;
        case 4:
            return d->is_shared ? *reinterpret_cast<qint32 *>(d->data.shared->ptr) : d->data.i;
        case 8:
            return d->is_shared ? *reinterpret_cast<qint64 *>(d->data.shared->ptr) : d->data.ll;
        }
    }

    *ok = false;
    return Q_INT64_C(0);
}

static qreal qConvertToRealNumber(const QVariant::Private *d, bool *ok)
{
    *ok = true;
    switch (uint(d->type)) {
    case QVariant::Double:
        return qreal(d->data.d);
    case QMetaType::Float:
        return qreal(d->data.f);
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qreal(qMetaTypeUNumber(d));
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        return v_cast<QJsonValue>(d)->toDouble();
#endif
    default:
        // includes enum conversion as well as invalid types
        return qreal(qConvertToNumber(d, ok));
    }
}

static qulonglong qConvertToUnsignedNumber(const QVariant::Private *d, bool *ok)
{
    *ok = true;

    switch (uint(d->type)) {
    case QVariant::String:
        return v_cast<QString>(d)->toULongLong(ok);
    case QVariant::Char:
        return v_cast<QChar>(d)->unicode();
    case QVariant::ByteArray:
        return v_cast<QByteArray>(d)->toULongLong(ok);
    case QVariant::Bool:
        return qulonglong(d->data.b);
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        if (!v_cast<QJsonValue>(d)->isDouble())
            break;
        Q_FALLTHROUGH();
#endif
    case QVariant::Double:
    case QVariant::Int:
    case QMetaType::Char:
    case QMetaType::SChar:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::Float:
    case QMetaType::LongLong:
        return qulonglong(qMetaTypeNumber(d));
    case QVariant::ULongLong:
    case QVariant::UInt:
    case QMetaType::UChar:
    case QMetaType::UShort:
    case QMetaType::ULong:
        return qMetaTypeUNumber(d);
    }

    QMetaType typeInfo(d->type);
    if (typeInfo.flags() & QMetaType::IsEnumeration) {
        switch (typeInfo.sizeOf()) {
        case 1:
            return d->is_shared ? *reinterpret_cast<uchar *>(d->data.shared->ptr) : d->data.uc;
        case 2:
            return d->is_shared ? *reinterpret_cast<quint16 *>(d->data.shared->ptr) : d->data.us;
        case 4:
            return d->is_shared ? *reinterpret_cast<quint32 *>(d->data.shared->ptr) : d->data.u;
        case 8:
            return d->is_shared ? *reinterpret_cast<qint64 *>(d->data.shared->ptr) : d->data.ull;
        }
    }

    *ok = false;
    return Q_UINT64_C(0);
}

template<typename TInput, typename LiteralWrapper>
inline bool qt_convertToBool(const QVariant::Private *const d)
{
    TInput str = v_cast<TInput>(d)->toLower();
    return !(str.isEmpty() || str == LiteralWrapper("0") || str == LiteralWrapper("false"));
}

/*!
 \internal
 Returns the internal data pointer from \a d.
 */

static const void *constData(const QVariant::Private &d)
{
    return d.is_shared ? d.data.shared->ptr : reinterpret_cast<const void *>(&d.data.c);
}

#ifndef QT_NO_QOBJECT
/*!
  \internal
  returns a QMetaEnum for a given meta tape type id if possible
*/
static QMetaEnum metaEnumFromType(int type)
{
    QMetaType t(type);
    if (t.flags() & QMetaType::IsEnumeration) {
        if (const QMetaObject *metaObject = t.metaObject()) {
            const char *enumName = QMetaType::typeName(type);
            const char *lastColon = std::strrchr(enumName, ':');
            if (lastColon)
                enumName = lastColon + 1;
            return metaObject->enumerator(metaObject->indexOfEnumerator(enumName));
        }
    }
    return QMetaEnum();
}
#endif

/*!
 \internal

 Converts \a d to type \a t, which is placed in \a result.
 */
static bool convert(const QVariant::Private *d, int t, void *result, bool *ok)
{
    Q_ASSERT(d->type != uint(t));
    Q_ASSERT(result);

    if (d->type >= QMetaType::User || t >= QMetaType::User) {
        const bool isOk = QMetaType::convert(constData(*d), d->type, result, t);
        if (ok)
            *ok = isOk;
        if (isOk)
            return true;
    }

    bool dummy;
    if (!ok)
        ok = &dummy;

    switch (uint(t)) {
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        switch (d->type) {
        case QVariant::String:
            *static_cast<QUrl *>(result) = QUrl(*v_cast<QString>(d));
            break;
        default:
            return false;
        }
        break;
#endif // QT_BOOTSTRAPPED
#if QT_CONFIG(itemmodel)
    case QVariant::ModelIndex:
        switch (d->type) {
        case QVariant::PersistentModelIndex:
            *static_cast<QModelIndex *>(result) = QModelIndex(*v_cast<QPersistentModelIndex>(d));
            break;
        default:
            return false;
        }
        break;
    case QVariant::PersistentModelIndex:
        switch (d->type) {
        case QVariant::ModelIndex:
            *static_cast<QPersistentModelIndex *>(result) = QPersistentModelIndex(*v_cast<QModelIndex>(d));
            break;
        default:
            return false;
        }
        break;
#endif // QT_CONFIG(itemmodel)
    case QVariant::String: {
        QString *str = static_cast<QString *>(result);
        switch (d->type) {
        case QVariant::Char:
            *str = *v_cast<QChar>(d);
            break;
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
            *str = QChar::fromLatin1(d->data.c);
            break;
        case QMetaType::Short:
        case QMetaType::Long:
        case QVariant::Int:
        case QVariant::LongLong:
            *str = QString::number(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *str = QString::number(qMetaTypeUNumber(d));
            break;
        case QMetaType::Float:
            *str = QString::number(d->data.f, 'g', QLocale::FloatingPointShortest);
            break;
        case QVariant::Double:
            *str = QString::number(d->data.d, 'g', QLocale::FloatingPointShortest);
            break;
#if QT_CONFIG(datestring)
        case QVariant::Date:
            *str = v_cast<QDate>(d)->toString(Qt::ISODate);
            break;
        case QVariant::Time:
            *str = v_cast<QTime>(d)->toString(Qt::ISODateWithMs);
            break;
        case QVariant::DateTime:
            *str = v_cast<QDateTime>(d)->toString(Qt::ISODateWithMs);
            break;
#endif
        case QVariant::Bool:
            *str = d->data.b ? QStringLiteral("true") : QStringLiteral("false");
            break;
        case QVariant::ByteArray:
            *str = QString::fromUtf8(v_cast<QByteArray>(d)->constData());
            break;
        case QVariant::StringList:
            if (v_cast<QStringList>(d)->count() == 1)
                *str = v_cast<QStringList>(d)->at(0);
            break;
#ifndef QT_BOOTSTRAPPED
        case QVariant::Url:
            *str = v_cast<QUrl>(d)->toString();
            break;
        case QMetaType::QJsonValue:
            if (v_cast<QJsonValue>(d)->isString())
                *str = v_cast<QJsonValue>(d)->toString();
            else if (!v_cast<QJsonValue>(d)->isNull())
                return false;
            break;
#endif
        case QVariant::Uuid:
            *str = v_cast<QUuid>(d)->toString();
            break;
        case QMetaType::Nullptr:
            *str = QString();
            break;
        default:
#ifndef QT_NO_QOBJECT
            {
                QMetaEnum en = metaEnumFromType(d->type);
                if (en.isValid()) {
                    *str = QString::fromUtf8(en.valueToKey(qConvertToNumber(d, ok)));
                    return *ok;
                }
            }
#endif
            return false;
        }
        break;
    }
    case QVariant::Char: {
        QChar *c = static_cast<QChar *>(result);
        switch (d->type) {
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::Long:
        case QMetaType::Float:
            *c = QChar(ushort(qMetaTypeNumber(d)));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *c = QChar(ushort(qMetaTypeUNumber(d)));
            break;
        default:
            return false;
        }
        break;
    }
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Size: {
        QSize *s = static_cast<QSize *>(result);
        switch (d->type) {
        case QVariant::SizeF:
            *s = v_cast<QSizeF>(d)->toSize();
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::SizeF: {
        QSizeF *s = static_cast<QSizeF *>(result);
        switch (d->type) {
        case QVariant::Size:
            *s = QSizeF(*(v_cast<QSize>(d)));
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::Line: {
        QLine *s = static_cast<QLine *>(result);
        switch (d->type) {
        case QVariant::LineF:
            *s = v_cast<QLineF>(d)->toLine();
            break;
        default:
            return false;
        }
        break;
    }

    case QVariant::LineF: {
        QLineF *s = static_cast<QLineF *>(result);
        switch (d->type) {
        case QVariant::Line:
            *s = QLineF(*(v_cast<QLine>(d)));
            break;
        default:
            return false;
        }
        break;
    }
#endif
    case QVariant::StringList:
        if (d->type == QVariant::List) {
            QStringList *slst = static_cast<QStringList *>(result);
            const QVariantList *list = v_cast<QVariantList >(d);
            const int size = list->size();
            slst->reserve(size);
            for (int i = 0; i < size; ++i)
                slst->append(list->at(i).toString());
        } else if (d->type == QVariant::String) {
            QStringList *slst = static_cast<QStringList *>(result);
            *slst = QStringList(*v_cast<QString>(d));
        } else {
            return false;
        }
        break;
    case QVariant::Date: {
        QDate *dt = static_cast<QDate *>(result);
        if (d->type == QVariant::DateTime)
            *dt = v_cast<QDateTime>(d)->date();
#if QT_CONFIG(datestring)
        else if (d->type == QVariant::String)
            *dt = QDate::fromString(*v_cast<QString>(d), Qt::ISODate);
#endif
        else
            return false;

        return dt->isValid();
    }
    case QVariant::Time: {
        QTime *t = static_cast<QTime *>(result);
        switch (d->type) {
        case QVariant::DateTime:
            *t = v_cast<QDateTime>(d)->time();
            break;
#if QT_CONFIG(datestring)
        case QVariant::String:
            *t = QTime::fromString(*v_cast<QString>(d), Qt::ISODate);
            break;
#endif
        default:
            return false;
        }
        return t->isValid();
    }
    case QVariant::DateTime: {
        QDateTime *dt = static_cast<QDateTime *>(result);
        switch (d->type) {
#if QT_CONFIG(datestring)
        case QVariant::String:
            *dt = QDateTime::fromString(*v_cast<QString>(d), Qt::ISODate);
            break;
#endif
        case QVariant::Date:
            *dt = QDateTime(*v_cast<QDate>(d));
            break;
        default:
            return false;
        }
        return dt->isValid();
    }
    case QVariant::ByteArray: {
        QByteArray *ba = static_cast<QByteArray *>(result);
        switch (d->type) {
        case QVariant::String:
            *ba = v_cast<QString>(d)->toUtf8();
            break;
        case QVariant::Double:
            *ba = QByteArray::number(d->data.d, 'g', QLocale::FloatingPointShortest);
            break;
        case QMetaType::Float:
            *ba = QByteArray::number(d->data.f, 'g', QLocale::FloatingPointShortest);
            break;
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
            *ba = QByteArray(1, d->data.c);
            break;
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Short:
        case QMetaType::Long:
            *ba = QByteArray::number(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *ba = QByteArray::number(qMetaTypeUNumber(d));
            break;
        case QVariant::Bool:
            *ba = QByteArray(d->data.b ? "true" : "false");
            break;
        case QVariant::Uuid:
            *ba = v_cast<QUuid>(d)->toByteArray();
            break;
        case QMetaType::Nullptr:
            *ba = QByteArray();
            break;
        default:
#ifndef QT_NO_QOBJECT
            {
                QMetaEnum en = metaEnumFromType(d->type);
                if (en.isValid()) {
                    *ba = en.valueToKey(qConvertToNumber(d, ok));
                    return *ok;
                }
            }
#endif
            return false;
        }
    }
    break;
    case QMetaType::Short:
        *static_cast<short *>(result) = short(qConvertToNumber(d, ok));
        return *ok;
    case QMetaType::Long:
        *static_cast<long *>(result) = long(qConvertToNumber(d, ok));
        return *ok;
    case QMetaType::UShort:
        *static_cast<ushort *>(result) = ushort(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QMetaType::ULong:
        *static_cast<ulong *>(result) = ulong(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QVariant::Int:
        *static_cast<int *>(result) = int(qConvertToNumber(d, ok));
        return *ok;
    case QVariant::UInt:
        *static_cast<uint *>(result) = uint(qConvertToUnsignedNumber(d, ok));
        return *ok;
    case QVariant::LongLong:
        *static_cast<qlonglong *>(result) = qConvertToNumber(d, ok);
        return *ok;
    case QVariant::ULongLong: {
        *static_cast<qulonglong *>(result) = qConvertToUnsignedNumber(d, ok);
        return *ok;
    }
    case QMetaType::SChar: {
        signed char s = qConvertToNumber(d, ok);
        *static_cast<signed char*>(result) = s;
        return *ok;
    }
    case QMetaType::UChar: {
        *static_cast<uchar *>(result) = qConvertToUnsignedNumber(d, ok);
        return *ok;
    }
    case QVariant::Bool: {
        bool *b = static_cast<bool *>(result);
        switch(d->type) {
        case QVariant::ByteArray:
            *b = qt_convertToBool<QByteArray, const char*>(d);
            break;
        case QVariant::String:
            *b = qt_convertToBool<QString, QLatin1String>(d);
            break;
        case QVariant::Char:
            *b = !v_cast<QChar>(d)->isNull();
            break;
        case QVariant::Double:
        case QVariant::Int:
        case QVariant::LongLong:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::Long:
        case QMetaType::Float:
            *b = qMetaTypeNumber(d) != Q_INT64_C(0);
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *b = qMetaTypeUNumber(d) != Q_UINT64_C(0);
            break;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *b = v_cast<QJsonValue>(d)->toBool(false);
            if (!v_cast<QJsonValue>(d)->isBool())
                return false;
            break;
#endif
        default:
            *b = false;
            return false;
        }
        break;
    }
    case QVariant::Double: {
        double *f = static_cast<double *>(result);
        switch (d->type) {
        case QVariant::String:
            *f = v_cast<QString>(d)->toDouble(ok);
            break;
        case QVariant::ByteArray:
            *f = v_cast<QByteArray>(d)->toDouble(ok);
            break;
        case QVariant::Bool:
            *f = double(d->data.b);
            break;
        case QMetaType::Float:
            *f = double(d->data.f);
            break;
        case QVariant::LongLong:
        case QVariant::Int:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::Long:
            *f = double(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *f = double(qMetaTypeUNumber(d));
            break;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *f = v_cast<QJsonValue>(d)->toDouble(0.0);
            if (!v_cast<QJsonValue>(d)->isDouble())
                return false;
            break;
#endif
        default:
            *f = 0.0;
            return false;
        }
        break;
    }
    case QMetaType::Float: {
        float *f = static_cast<float *>(result);
        switch (d->type) {
        case QVariant::String:
            *f = v_cast<QString>(d)->toFloat(ok);
            break;
        case QVariant::ByteArray:
            *f = v_cast<QByteArray>(d)->toFloat(ok);
            break;
        case QVariant::Bool:
            *f = float(d->data.b);
            break;
        case QVariant::Double:
            *f = float(d->data.d);
            break;
        case QVariant::LongLong:
        case QVariant::Int:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::Long:
            *f = float(qMetaTypeNumber(d));
            break;
        case QVariant::UInt:
        case QVariant::ULongLong:
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::ULong:
            *f = float(qMetaTypeUNumber(d));
            break;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QJsonValue:
            *f = v_cast<QJsonValue>(d)->toDouble(0.0);
            if (!v_cast<QJsonValue>(d)->isDouble())
                return false;
            break;
#endif
        default:
            *f = 0.0f;
            return false;
        }
        break;
    }
    case QVariant::List:
        if (d->type == QVariant::StringList) {
            QVariantList *lst = static_cast<QVariantList *>(result);
            const QStringList *slist = v_cast<QStringList>(d);
            const int size = slist->size();
            lst->reserve(size);
            for (int i = 0; i < size; ++i)
                lst->append(QVariant(slist->at(i)));
        } else if (qstrcmp(QMetaType::typeName(d->type), "QList<QVariant>") == 0) {
            *static_cast<QVariantList *>(result) =
                *static_cast<QList<QVariant> *>(d->data.shared->ptr);
#ifndef QT_BOOTSTRAPPED
        } else if (d->type == QMetaType::QJsonValue) {
            if (!v_cast<QJsonValue>(d)->isArray())
                return false;
            *static_cast<QVariantList *>(result) = v_cast<QJsonValue>(d)->toArray().toVariantList();
        } else if (d->type == QMetaType::QJsonArray) {
            *static_cast<QVariantList *>(result) = v_cast<QJsonArray>(d)->toVariantList();
#endif
        } else {
            return false;
        }
        break;
    case QVariant::Map:
        if (qstrcmp(QMetaType::typeName(d->type), "QMap<QString, QVariant>") == 0) {
            *static_cast<QVariantMap *>(result) =
                *static_cast<QMap<QString, QVariant> *>(d->data.shared->ptr);
        } else if (d->type == QVariant::Hash) {
            QVariantMap *map = static_cast<QVariantMap *>(result);
            const QVariantHash *hash = v_cast<QVariantHash>(d);
            const auto end = hash->end();
            for (auto it = hash->begin(); it != end; ++it)
                map->insertMulti(it.key(), it.value());
#ifndef QT_BOOTSTRAPPED
        } else if (d->type == QMetaType::QJsonValue) {
            if (!v_cast<QJsonValue>(d)->isObject())
                return false;
            *static_cast<QVariantMap *>(result) = v_cast<QJsonValue>(d)->toObject().toVariantMap();
        } else if (d->type == QMetaType::QJsonObject) {
            *static_cast<QVariantMap *>(result) = v_cast<QJsonObject>(d)->toVariantMap();
#endif
        } else {
            return false;
        }
        break;
    case QVariant::Hash:
        if (qstrcmp(QMetaType::typeName(d->type), "QHash<QString, QVariant>") == 0) {
            *static_cast<QVariantHash *>(result) =
                *static_cast<QHash<QString, QVariant> *>(d->data.shared->ptr);
        } else if (d->type == QVariant::Map) {
            QVariantHash *hash = static_cast<QVariantHash *>(result);
            const QVariantMap *map = v_cast<QVariantMap>(d);
            const auto end = map->end();
            for (auto it = map->begin(); it != end; ++it)
                hash->insertMulti(it.key(), it.value());
#ifndef QT_BOOTSTRAPPED
        } else if (d->type == QMetaType::QJsonValue) {
            if (!v_cast<QJsonValue>(d)->isObject())
                return false;
            *static_cast<QVariantHash *>(result) = v_cast<QJsonValue>(d)->toObject().toVariantHash();
        } else if (d->type == QMetaType::QJsonObject) {
            *static_cast<QVariantHash *>(result) = v_cast<QJsonObject>(d)->toVariantHash();
#endif
        } else {
            return false;
        }
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Rect:
        if (d->type == QVariant::RectF)
            *static_cast<QRect *>(result) = (v_cast<QRectF>(d))->toRect();
        else
            return false;
        break;
    case QVariant::RectF:
        if (d->type == QVariant::Rect)
            *static_cast<QRectF *>(result) = *v_cast<QRect>(d);
        else
            return false;
        break;
    case QVariant::PointF:
        if (d->type == QVariant::Point)
            *static_cast<QPointF *>(result) = *v_cast<QPoint>(d);
        else
            return false;
        break;
    case QVariant::Point:
        if (d->type == QVariant::PointF)
            *static_cast<QPoint *>(result) = (v_cast<QPointF>(d))->toPoint();
        else
            return false;
        break;
    case QMetaType::Char:
    {
        *static_cast<qint8 *>(result) = qint8(qConvertToNumber(d, ok));
        return *ok;
    }
#endif
    case QVariant::Uuid:
        switch (d->type) {
        case QVariant::String:
            *static_cast<QUuid *>(result) = QUuid(*v_cast<QString>(d));
            break;
        case QVariant::ByteArray:
            *static_cast<QUuid *>(result) = QUuid(*v_cast<QByteArray>(d));
            break;
        default:
            return false;
        }
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QJsonValue:
        switch (d->type) {
        case QMetaType::Nullptr:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonValue::Null);
            break;
        case QVariant::Bool:
            *static_cast<QJsonValue *>(result) = QJsonValue(d->data.b);
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Double:
        case QMetaType::Float:
        case QMetaType::ULong:
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::UShort:
        case QMetaType::UChar:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
            *static_cast<QJsonValue *>(result) = QJsonValue(qConvertToRealNumber(d, ok));
            Q_ASSERT(ok);
            break;
        case QVariant::String:
            *static_cast<QJsonValue *>(result) = QJsonValue(*v_cast<QString>(d));
            break;
        case QVariant::StringList:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonArray::fromStringList(*v_cast<QStringList>(d)));
            break;
        case QVariant::List:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonArray::fromVariantList(*v_cast<QVariantList>(d)));
            break;
        case QVariant::Map:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonObject::fromVariantMap(*v_cast<QVariantMap>(d)));
            break;
        case QVariant::Hash:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonObject::fromVariantHash(*v_cast<QVariantHash>(d)));
            break;
        case QMetaType::QJsonObject:
            *static_cast<QJsonValue *>(result) = *v_cast<QJsonObject>(d);
            break;
        case QMetaType::QJsonArray:
            *static_cast<QJsonValue *>(result) = *v_cast<QJsonArray>(d);
            break;
        case QMetaType::QJsonDocument: {
            QJsonDocument doc = *v_cast<QJsonDocument>(d);
            *static_cast<QJsonValue *>(result) = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
            break;
        }
        default:
            *static_cast<QJsonValue *>(result) = QJsonValue(QJsonValue::Undefined);
            return false;
        }
        break;
    case QMetaType::QJsonArray:
        switch (d->type) {
        case QVariant::StringList:
            *static_cast<QJsonArray *>(result) = QJsonArray::fromStringList(*v_cast<QStringList>(d));
            break;
        case QVariant::List:
            *static_cast<QJsonArray *>(result) = QJsonArray::fromVariantList(*v_cast<QVariantList>(d));
            break;
        case QMetaType::QJsonValue:
            if (!v_cast<QJsonValue>(d)->isArray())
                return false;
            *static_cast<QJsonArray *>(result) = v_cast<QJsonValue>(d)->toArray();
            break;
        case QMetaType::QJsonDocument:
            if (!v_cast<QJsonDocument>(d)->isArray())
                return false;
            *static_cast<QJsonArray *>(result) = v_cast<QJsonDocument>(d)->array();
            break;
        default:
            return false;
        }
        break;
    case QMetaType::QJsonObject:
        switch (d->type) {
        case QVariant::Map:
            *static_cast<QJsonObject *>(result) = QJsonObject::fromVariantMap(*v_cast<QVariantMap>(d));
            break;
        case QVariant::Hash:
            *static_cast<QJsonObject *>(result) = QJsonObject::fromVariantHash(*v_cast<QVariantHash>(d));
            break;
        case QMetaType::QJsonValue:
            if (!v_cast<QJsonValue>(d)->isObject())
                return false;
            *static_cast<QJsonObject *>(result) = v_cast<QJsonValue>(d)->toObject();
            break;
        case QMetaType::QJsonDocument:
            if (v_cast<QJsonDocument>(d)->isArray())
                return false;
            *static_cast<QJsonObject *>(result) = v_cast<QJsonDocument>(d)->object();
            break;
        default:
            return false;
        }
        break;
#endif
    default:
#ifndef QT_NO_QOBJECT
        if (d->type == QVariant::String || d->type == QVariant::ByteArray) {
            QMetaEnum en = metaEnumFromType(t);
            if (en.isValid()) {
                QByteArray keys = (d->type == QVariant::String) ? v_cast<QString>(d)->toUtf8() : *v_cast<QByteArray>(d);
                int value = en.keysToValue(keys.constData(), ok);
                if (*ok) {
                    switch (QMetaType::sizeOf(t)) {
                    case 1:
                        *static_cast<signed char *>(result) = value;
                        return true;
                    case 2:
                        *static_cast<qint16 *>(result) = value;
                        return true;
                    case 4:
                        *static_cast<qint32 *>(result) = value;
                        return true;
                    case 8:
                        *static_cast<qint64 *>(result) = value;
                        return true;
                    }
                }
            }
        }
#endif
        if (QMetaType::typeFlags(t) & QMetaType::IsEnumeration) {
            qlonglong value = qConvertToNumber(d, ok);
            if (*ok) {
                switch (QMetaType::sizeOf(t)) {
                case 1:
                    *static_cast<signed char *>(result) = value;
                    return true;
                case 2:
                    *static_cast<qint16 *>(result) = value;
                    return true;
                case 4:
                    *static_cast<qint32 *>(result) = value;
                    return true;
                case 8:
                    *static_cast<qint64 *>(result) = value;
                    return true;
                }
            }
            return *ok;
        }
        return false;
    }
    return true;
}

#if !defined(QT_NO_DEBUG_STREAM)
static void streamDebug(QDebug dbg, const QVariant &v)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(&v.data_ptr());
    QVariantDebugStream<CoreTypesFilter> stream(dbg, d);
    QMetaTypeSwitcher::switcher<void>(stream, d->type, 0);
}
#endif

const QVariant::Handler qt_kernel_variant_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    compare,
    convert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    streamDebug
#else
    0
#endif
};

static void dummyConstruct(QVariant::Private *, const void *) { Q_ASSERT_X(false, "QVariant", "Trying to construct an unknown type"); }
static void dummyClear(QVariant::Private *) { Q_ASSERT_X(false, "QVariant", "Trying to clear an unknown type"); }
static bool dummyIsNull(const QVariant::Private *d) { Q_ASSERT_X(false, "QVariant::isNull", "Trying to call isNull on an unknown type"); return d->is_null; }
static bool dummyCompare(const QVariant::Private *, const QVariant::Private *) { Q_ASSERT_X(false, "QVariant", "Trying to compare an unknown types"); return false; }
static bool dummyConvert(const QVariant::Private *, int, void *, bool *) { Q_ASSERT_X(false, "QVariant", "Trying to convert an unknown type"); return false; }
#if !defined(QT_NO_DEBUG_STREAM)
static void dummyStreamDebug(QDebug, const QVariant &) { Q_ASSERT_X(false, "QVariant", "Trying to convert an unknown type"); }
#endif
const QVariant::Handler qt_dummy_variant_handler = {
    dummyConstruct,
    dummyClear,
    dummyIsNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    dummyCompare,
    dummyConvert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    dummyStreamDebug
#else
    0
#endif
};

static void customConstruct(QVariant::Private *d, const void *copy)
{
    const QMetaType type(d->type);
    const uint size = type.sizeOf();
    if (!size) {
        qWarning("Trying to construct an instance of an invalid type, type id: %i", d->type);
        d->type = QVariant::Invalid;
        return;
    }

    // this logic should match with QVariantIntegrator::CanUseInternalSpace
    if (size <= sizeof(QVariant::Private::Data)
            && (type.flags() & (QMetaType::MovableType | QMetaType::IsEnumeration))) {
        type.construct(&d->data.ptr, copy);
        d->is_shared = false;
    } else {
        void *ptr = type.create(copy);
        d->is_shared = true;
        d->data.shared = new QVariant::PrivateShared(ptr);
    }
}

static void customClear(QVariant::Private *d)
{
    if (!d->is_shared) {
        QMetaType::destruct(d->type, &d->data.ptr);
    } else {
        QMetaType::destroy(d->type, d->data.shared->ptr);
        delete d->data.shared;
    }
}

static bool customIsNull(const QVariant::Private *d)
{
    if (d->is_null)
        return true;
    const char *const typeName = QMetaType::typeName(d->type);
    if (Q_UNLIKELY(!typeName) && Q_LIKELY(!QMetaType::isRegistered(d->type)))
        qFatal("QVariant::isNull: type %d unknown to QVariant.", d->type);
    uint typeNameLen = qstrlen(typeName);
    if (typeNameLen > 0 && typeName[typeNameLen - 1] == '*') {
        const void *d_ptr = d->is_shared ? d->data.shared->ptr : &(d->data.ptr);
        return *static_cast<void *const *>(d_ptr) == nullptr;
    }
    return false;
}

static bool customCompare(const QVariant::Private *a, const QVariant::Private *b)
{
    const char *const typeName = QMetaType::typeName(a->type);
    if (Q_UNLIKELY(!typeName) && Q_LIKELY(!QMetaType::isRegistered(a->type)))
        qFatal("QVariant::compare: type %d unknown to QVariant.", a->type);

    const void *a_ptr = a->is_shared ? a->data.shared->ptr : &(a->data.ptr);
    const void *b_ptr = b->is_shared ? b->data.shared->ptr : &(b->data.ptr);

    uint typeNameLen = qstrlen(typeName);
    if (typeNameLen > 0 && typeName[typeNameLen - 1] == '*')
        return *static_cast<void *const *>(a_ptr) == *static_cast<void *const *>(b_ptr);

    if (a->is_null && b->is_null)
        return true;

    return !memcmp(a_ptr, b_ptr, QMetaType::sizeOf(a->type));
}

static bool customConvert(const QVariant::Private *d, int t, void *result, bool *ok)
{
    if (d->type >= QMetaType::User || t >= QMetaType::User) {
        if (QMetaType::convert(constData(*d), d->type, result, t)) {
            if (ok)
                *ok = true;
            return true;
        }
    }
    return convert(d, t, result, ok);
}

#if !defined(QT_NO_DEBUG_STREAM)
static void customStreamDebug(QDebug dbg, const QVariant &variant) {
#ifndef QT_BOOTSTRAPPED
    QMetaType::TypeFlags flags = QMetaType::typeFlags(variant.userType());
    if (flags & QMetaType::PointerToQObject)
        dbg.nospace() << variant.value<QObject*>();
#else
    Q_UNUSED(dbg);
    Q_UNUSED(variant);
#endif
}
#endif

const QVariant::Handler qt_custom_variant_handler = {
    customConstruct,
    customClear,
    customIsNull,
#ifndef QT_NO_DATASTREAM
    0,
    0,
#endif
    customCompare,
    customConvert,
    0,
#if !defined(QT_NO_DEBUG_STREAM)
    customStreamDebug
#else
    0
#endif
};

} // annonymous used to hide QVariant handlers

static HandlersManager handlerManager;
Q_STATIC_ASSERT_X(!QModulesPrivate::Core, "Initialization assumes that ModulesNames::Core is 0");
const QVariant::Handler *HandlersManager::Handlers[QModulesPrivate::ModulesCount]
                                        = { &qt_kernel_variant_handler, &qt_dummy_variant_handler,
                                            &qt_dummy_variant_handler, &qt_custom_variant_handler };

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler()
{
    return &qt_kernel_variant_handler;
}

Q_CORE_EXPORT void QVariantPrivate::registerHandler(const int /* Modules::Names */name, const QVariant::Handler *handler)
{
    handlerManager.registerHandler(static_cast<QModulesPrivate::Names>(name), handler);
}

/*!
    \class QVariant
    \inmodule QtCore
    \brief The QVariant class acts like a union for the most common Qt data types.

    \ingroup objectmodel
    \ingroup shared


    Because C++ forbids unions from including types that have
    non-default constructors or destructors, most interesting Qt
    classes cannot be used in unions. Without QVariant, this would be
    a problem for QObject::property() and for database work, etc.

    A QVariant object holds a single value of a single type() at a
    time. (Some type()s are multi-valued, for example a string list.)
    You can find out what type, T, the variant holds, convert it to a
    different type using convert(), get its value using one of the
    toT() functions (e.g., toSize()) and check whether the type can
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

    QVariant also supports the notion of null values, where you can
    have a defined type with no value set. However, note that QVariant
    types can only be cast when they have had a value set.

    \snippet code/src_corelib_kernel_qvariant.cpp 1

    QVariant can be extended to support other types than those
    mentioned in the \l Type enum. See \l{Creating Custom Qt Types}{Creating Custom Qt Types}
    for details.

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

    For example, canConvert(Int) would return true when called on a variant
    containing a string because, in principle, QVariant is able to convert
    strings of numbers to integers.
    However, if the string contains non-numeric characters, it cannot be
    converted to an integer, and any attempt to convert it will fail.
    Hence, it is important to have both functions return true for a
    successful conversion.

    \sa QMetaType
*/

/*!
    \obsolete Use QMetaType::Type instead
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
    \value PersistentModelIndex a QPersistentModelIndex (since 5.5)
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
    \value Matrix  a QMatrix
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
    \value RegExp  a QRegExp
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
    \fn QVariant::QVariant(int typeId, const void *copy)

    Constructs variant of type \a typeId, and initializes with
    \a copy if \a copy is not 0.

    Note that you have to pass the address of the variable you want stored.

    Usually, you never have to use this constructor, use QVariant::fromValue()
    instead to construct variants from the pointer types represented by
    \c QMetaType::VoidStar, and \c QMetaType::QObjectStar.

    \sa QVariant::fromValue(), QMetaType::Type
*/

/*!
    \fn QVariant::QVariant(Type type)

    Constructs an uninitialized variant of type \a type. This will create a
    variant in a special null state that if accessed will return a default
    constructed value of the \a type.

    \sa isNull()
*/



/*!
    \fn QVariant::create(int type, const void *copy)

    \internal

    Constructs a variant private of type \a type, and initializes with \a copy if
    \a copy is not 0.
*/

void QVariant::create(int type, const void *copy)
{
    d.type = type;
    handlerManager[type]->construct(&d, copy);
}

/*!
    \fn QVariant::~QVariant()

    Destroys the QVariant and the contained object.

    Note that subclasses that reimplement clear() should reimplement
    the destructor to call clear(). This destructor calls clear(), but
    because it is the destructor, QVariant::clear() is called rather
    than a subclass's clear().
*/

QVariant::~QVariant()
{
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type > Char))
        handlerManager[d.type]->clear(&d);
}

/*!
  \fn QVariant::QVariant(const QVariant &p)

    Constructs a copy of the variant, \a p, passed as the argument to
    this constructor.
*/

QVariant::QVariant(const QVariant &p)
    : d(p.d)
{
    if (d.is_shared) {
        d.data.shared->ref.ref();
    } else if (p.d.type > Char) {
        handlerManager[d.type]->construct(&d, p.constData());
        d.is_null = p.d.is_null;
    }
}

#ifndef QT_NO_DATASTREAM
/*!
    Reads the variant from the data stream, \a s.
*/
QVariant::QVariant(QDataStream &s)
{
    d.is_null = true;
    s >> *this;
}
#endif //QT_NO_DATASTREAM

/*!
  \fn QVariant::QVariant(const QString &val)

    Constructs a new variant with a string value, \a val.
*/

/*!
  \fn QVariant::QVariant(QLatin1String val)

    Constructs a new variant with a string value, \a val.
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

#ifndef QT_NO_CAST_FROM_ASCII
QVariant::QVariant(const char *val)
{
    QString s = QString::fromUtf8(val);
    create(String, &s);
}
#endif

/*!
  \fn QVariant::QVariant(const QStringList &val)

    Constructs a new variant with a string list value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QMap<QString, QVariant> &val)

    Constructs a new variant with a map of \l {QVariant}s, \a val.
*/

/*!
  \fn QVariant::QVariant(const QHash<QString, QVariant> &val)

    Constructs a new variant with a hash of \l {QVariant}s, \a val.
*/

/*!
  \fn QVariant::QVariant(const QDate &val)

    Constructs a new variant with a date value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QTime &val)

    Constructs a new variant with a time value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QDateTime &val)

    Constructs a new variant with a date/time value, \a val.
*/

/*!
    \since 4.7
  \fn QVariant::QVariant(const QEasingCurve &val)

    Constructs a new variant with an easing curve value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QUuid &val)

    Constructs a new variant with an uuid value, \a val.
*/

/*!
    \since 5.0
    \fn QVariant::QVariant(const QModelIndex &val)

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
  \fn QVariant::QVariant(const QByteArray &val)

    Constructs a new variant with a bytearray value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QBitArray &val)

    Constructs a new variant with a bitarray value, \a val.
*/

/*!
  \fn QVariant::QVariant(const QPoint &val)

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QPointF &val)

  Constructs a new variant with a point value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QRectF &val)

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QLineF &val)

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QLine &val)

  Constructs a new variant with a line value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QRect &val)

  Constructs a new variant with a rect value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QSize &val)

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QSizeF &val)

  Constructs a new variant with a size value of \a val.
 */

/*!
  \fn QVariant::QVariant(const QUrl &val)

  Constructs a new variant with a url value of \a val.
 */

/*!
  \fn QVariant::QVariant(int val)

    Constructs a new variant with an integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(uint val)

    Constructs a new variant with an unsigned integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qlonglong val)

    Constructs a new variant with a long long integer value, \a val.
*/

/*!
  \fn QVariant::QVariant(qulonglong val)

    Constructs a new variant with an unsigned long long integer value, \a val.
*/


/*!
  \fn QVariant::QVariant(bool val)

    Constructs a new variant with a boolean value, \a val.
*/

/*!
  \fn QVariant::QVariant(double val)

    Constructs a new variant with a floating point value, \a val.
*/

/*!
  \fn QVariant::QVariant(float val)

    Constructs a new variant with a floating point value, \a val.
    \since 4.6
*/

/*!
    \fn QVariant::QVariant(const QList<QVariant> &val)

    Constructs a new variant with a list value, \a val.
*/

/*!
  \fn QVariant::QVariant(QChar c)

  Constructs a new variant with a char value, \a c.
*/

/*!
  \fn QVariant::QVariant(const QLocale &l)

  Constructs a new variant with a locale value, \a l.
*/

/*!
  \fn QVariant::QVariant(const QRegExp &regExp)

  Constructs a new variant with the regexp value \a regExp.
*/

/*!
  \fn QVariant::QVariant(const QRegularExpression &re)

  \since 5.0

  Constructs a new variant with the regular expression value \a re.
*/

QVariant::QVariant(Type type)
{ create(type, 0); }
QVariant::QVariant(int typeId, const void *copy)
{ create(typeId, copy); d.is_null = false; }

/*!
    \internal
    flags is true if it is a pointer type
 */
QVariant::QVariant(int typeId, const void *copy, uint flags)
{
    if (flags) { //type is a pointer type
        d.type = typeId;
        d.data.ptr = *reinterpret_cast<void *const*>(copy);
    } else {
        create(typeId, copy);
    }
    d.is_null = false;
}

QVariant::QVariant(int val)
    : d(Int)
{ d.data.i = val; }
QVariant::QVariant(uint val)
    : d(UInt)
{ d.data.u = val; }
QVariant::QVariant(qlonglong val)
    : d(LongLong)
{ d.data.ll = val; }
QVariant::QVariant(qulonglong val)
    : d(ULongLong)
{ d.data.ull = val; }
QVariant::QVariant(bool val)
    : d(Bool)
{ d.data.b = val; }
QVariant::QVariant(double val)
    : d(Double)
{ d.data.d = val; }
QVariant::QVariant(float val)
    : d(QMetaType::Float)
{ d.data.f = val; }

QVariant::QVariant(const QByteArray &val)
    : d(ByteArray)
{ v_construct<QByteArray>(&d, val); }
QVariant::QVariant(const QBitArray &val)
    : d(BitArray)
{ v_construct<QBitArray>(&d, val);  }
QVariant::QVariant(const QString &val)
    : d(String)
{ v_construct<QString>(&d, val);  }
QVariant::QVariant(QChar val)
    : d(Char)
{ v_construct<QChar>(&d, val);  }
QVariant::QVariant(QLatin1String val)
    : d(String)
{ v_construct<QString>(&d, val); }
QVariant::QVariant(const QStringList &val)
    : d(StringList)
{ v_construct<QStringList>(&d, val); }

QVariant::QVariant(const QDate &val)
    : d(Date)
{ v_construct<QDate>(&d, val); }
QVariant::QVariant(const QTime &val)
    : d(Time)
{ v_construct<QTime>(&d, val); }
QVariant::QVariant(const QDateTime &val)
    : d(DateTime)
{ v_construct<QDateTime>(&d, val); }
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QEasingCurve &val)
    : d(EasingCurve)
{ v_construct<QEasingCurve>(&d, val); }
#endif
QVariant::QVariant(const QList<QVariant> &list)
    : d(List)
{ v_construct<QVariantList>(&d, list); }
QVariant::QVariant(const QMap<QString, QVariant> &map)
    : d(Map)
{ v_construct<QVariantMap>(&d, map); }
QVariant::QVariant(const QHash<QString, QVariant> &hash)
    : d(Hash)
{ v_construct<QVariantHash>(&d, hash); }
#ifndef QT_NO_GEOM_VARIANT
QVariant::QVariant(const QPoint &pt)
    : d(Point)
{ v_construct<QPoint>(&d, pt); }
QVariant::QVariant(const QPointF &pt)
    : d(PointF)
{ v_construct<QPointF>(&d, pt); }
QVariant::QVariant(const QRectF &r)
    : d(RectF)
{ v_construct<QRectF>(&d, r); }
QVariant::QVariant(const QLineF &l)
    : d(LineF)
{ v_construct<QLineF>(&d, l); }
QVariant::QVariant(const QLine &l)
    : d(Line)
{ v_construct<QLine>(&d, l); }
QVariant::QVariant(const QRect &r)
    : d(Rect)
{ v_construct<QRect>(&d, r); }
QVariant::QVariant(const QSize &s)
    : d(Size)
{ v_construct<QSize>(&d, s); }
QVariant::QVariant(const QSizeF &s)
    : d(SizeF)
{ v_construct<QSizeF>(&d, s); }
#endif
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QUrl &u)
    : d(Url)
{ v_construct<QUrl>(&d, u); }
#endif
QVariant::QVariant(const QLocale &l)
    : d(Locale)
{ v_construct<QLocale>(&d, l); }
#ifndef QT_NO_REGEXP
QVariant::QVariant(const QRegExp &regExp)
    : d(RegExp)
{ v_construct<QRegExp>(&d, regExp); }
#endif // QT_NO_REGEXP
#if QT_CONFIG(regularexpression)
QVariant::QVariant(const QRegularExpression &re)
    : d(RegularExpression)
{ v_construct<QRegularExpression>(&d, re); }
#endif // QT_CONFIG(regularexpression)
#ifndef QT_BOOTSTRAPPED
QVariant::QVariant(const QUuid &uuid)
    : d(Uuid)
{ v_construct<QUuid>(&d, uuid); }
QVariant::QVariant(const QJsonValue &jsonValue)
    : d(QMetaType::QJsonValue)
{ v_construct<QJsonValue>(&d, jsonValue); }
QVariant::QVariant(const QJsonObject &jsonObject)
    : d(QMetaType::QJsonObject)
{ v_construct<QJsonObject>(&d, jsonObject); }
QVariant::QVariant(const QJsonArray &jsonArray)
    : d(QMetaType::QJsonArray)
{ v_construct<QJsonArray>(&d, jsonArray); }
QVariant::QVariant(const QJsonDocument &jsonDocument)
    : d(QMetaType::QJsonDocument)
{ v_construct<QJsonDocument>(&d, jsonDocument); }
#endif // QT_BOOTSTRAPPED
#if QT_CONFIG(itemmodel)
QVariant::QVariant(const QModelIndex &modelIndex)
    : d(ModelIndex)
{ v_construct<QModelIndex>(&d, modelIndex); }
QVariant::QVariant(const QPersistentModelIndex &modelIndex)
    : d(PersistentModelIndex)
{ v_construct<QPersistentModelIndex>(&d, modelIndex); }
#endif

/*!
    Returns the storage type of the value stored in the variant.
    Although this function is declared as returning QVariant::Type,
    the return value should be interpreted as QMetaType::Type. In
    particular, QVariant::UserType is returned here only if the value
    is equal or greater than QMetaType::User.

    Note that return values in the ranges QVariant::Char through
    QVariant::RegExp and QVariant::Font through QVariant::Transform
    correspond to the values in the ranges QMetaType::QChar through
    QMetaType::QRegExp and QMetaType::QFont through QMetaType::QQuaternion.

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
*/

QVariant::Type QVariant::type() const
{
    return d.type >= QMetaType::User ? UserType : static_cast<Type>(d.type);
}

/*!
    Returns the storage type of the value stored in the variant. For
    non-user types, this is the same as type().

    \sa type()
*/

int QVariant::userType() const
{
    return d.type;
}

/*!
    Assigns the value of the variant \a variant to this variant.
*/
QVariant& QVariant::operator=(const QVariant &variant)
{
    if (this == &variant)
        return *this;

    clear();
    if (variant.d.is_shared) {
        variant.d.data.shared->ref.ref();
        d = variant.d;
    } else if (variant.d.type > Char) {
        d.type = variant.d.type;
        handlerManager[d.type]->construct(&d, variant.constData());
        d.is_null = variant.d.is_null;
    } else {
        d = variant.d;
    }

    return *this;
}

/*!
    \fn void QVariant::swap(QVariant &other)
    \since 4.8

    Swaps variant \a other with this variant. This operation is very
    fast and never fails.
*/

/*!
    \fn void QVariant::detach()

    \internal
*/

void QVariant::detach()
{
    if (!d.is_shared || d.data.shared->ref.load() == 1)
        return;

    Private dd;
    dd.type = d.type;
    handlerManager[d.type]->construct(&dd, constData());
    if (!d.data.shared->ref.deref())
        handlerManager[d.type]->clear(&d);
    d.data.shared = dd.data.shared;
}

/*!
    \fn bool QVariant::isDetached() const

    \internal
*/

/*!
    Returns the name of the type stored in the variant. The returned
    strings describe the C++ datatype used to store the data: for
    example, "QFont", "QString", or "QVariantList". An Invalid
    variant returns 0.
*/
const char *QVariant::typeName() const
{
    return QMetaType::typeName(d.type);
}

/*!
    Convert this variant to type QMetaType::UnknownType and free up any resources
    used.
*/
void QVariant::clear()
{
    if ((d.is_shared && !d.data.shared->ref.deref()) || (!d.is_shared && d.type > Char))
        handlerManager[d.type]->clear(&d);
    d.type = Invalid;
    d.is_null = true;
    d.is_shared = false;
}

/*!
    Converts the int representation of the storage type, \a typeId, to
    its string representation.

    Returns a null pointer if the type is QMetaType::UnknownType or doesn't exist.
*/
const char *QVariant::typeToName(int typeId)
{
    return QMetaType::typeName(typeId);
}


/*!
    Converts the string representation of the storage type given in \a
    name, to its enum representation.

    If the string representation cannot be converted to any enum
    representation, the variant is set to \c Invalid.
*/
QVariant::Type QVariant::nameToType(const char *name)
{
    int metaType = QMetaType::type(name);
    return metaType <= int(UserType) ? QVariant::Type(metaType) : UserType;
}

#ifndef QT_NO_DATASTREAM
enum { MapFromThreeCount = 36 };
static const ushort mapIdFromQt3ToCurrent[MapFromThreeCount] =
{
    QVariant::Invalid,
    QVariant::Map,
    QVariant::List,
    QVariant::String,
    QVariant::StringList,
    QVariant::Font,
    QVariant::Pixmap,
    QVariant::Brush,
    QVariant::Rect,
    QVariant::Size,
    QVariant::Color,
    QVariant::Palette,
    0, // ColorGroup
    QVariant::Icon,
    QVariant::Point,
    QVariant::Image,
    QVariant::Int,
    QVariant::UInt,
    QVariant::Bool,
    QVariant::Double,
    0, // Buggy ByteArray, QByteArray never had id == 20
    QVariant::Polygon,
    QVariant::Region,
    QVariant::Bitmap,
    QVariant::Cursor,
    QVariant::SizePolicy,
    QVariant::Date,
    QVariant::Time,
    QVariant::DateTime,
    QVariant::ByteArray,
    QVariant::BitArray,
    QVariant::KeySequence,
    QVariant::Pen,
    QVariant::LongLong,
    QVariant::ULongLong,
    QVariant::EasingCurve
};

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
        if (typeId >= MapFromThreeCount)
            return;
        typeId = mapIdFromQt3ToCurrent[typeId];
    } else if (s.version() < QDataStream::Qt_5_0) {
        if (typeId == 127 /* QVariant::UserType */) {
            typeId = QMetaType::User;
        } else if (typeId >= 128 && typeId != QVariant::UserType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId -= 97;
        } else if (typeId == 75 /* QSizePolicy */) {
            typeId = QMetaType::QSizePolicy;
        } else if (typeId > 75 && typeId <= 86) {
            // and as a result these types received lower ids too
            // QKeySequence QPen QTextLength QTextFormat QMatrix QTransform QMatrix4x4 QVector2D QVector3D QVector4D QQuaternion
            typeId -=1;
        }
    }

    qint8 is_null = false;
    if (s.version() >= QDataStream::Qt_4_2)
        s >> is_null;
    if (typeId == QVariant::UserType) {
        QByteArray name;
        s >> name;
        typeId = QMetaType::type(name.constData());
        if (typeId == QMetaType::UnknownType) {
            s.setStatus(QDataStream::ReadCorruptData);
            qWarning("QVariant::load: unknown user type with name %s.", name.constData());
            return;
        }
    }
    create(typeId, 0);
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
    if (!QMetaType::load(s, d.type, const_cast<void *>(constData()))) {
        s.setStatus(QDataStream::ReadCorruptData);
        qWarning("QVariant::load: unable to load type %d.", d.type);
    }
}

/*!
    Internal function for saving a variant to the stream \a s. Use the
    stream operators instead.

    \internal
*/
void QVariant::save(QDataStream &s) const
{
    quint32 typeId = type();
    bool fakeUserType = false;
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
        if (typeId == QMetaType::User) {
            typeId = 127; // QVariant::UserType had this value in Qt4
        } else if (typeId >= 128 - 97 && typeId <= LastCoreType) {
            // In Qt4 id == 128 was FirstExtCoreType. In Qt5 ExtCoreTypes set was merged to CoreTypes
            // by moving all ids down by 97.
            typeId += 97;
        } else if (typeId == QMetaType::QSizePolicy) {
            typeId = 75;
        } else if (typeId >= QMetaType::QKeySequence && typeId <= QMetaType::QQuaternion) {
            // and as a result these types received lower ids too
            typeId +=1;
        } else if (typeId == QMetaType::QPolygonF) {
            // This existed in Qt 4 only as a custom type
            typeId = 127;
            fakeUserType = true;
        }
    }
    s << typeId;
    if (s.version() >= QDataStream::Qt_4_2)
        s << qint8(d.is_null);
    if (d.type >= QVariant::UserType || fakeUserType) {
        s << QMetaType::typeName(userType());
    }

    if (!isValid()) {
        if (s.version() < QDataStream::Qt_5_0)
            s << QString();
        return;
    }

    if (!QMetaType::save(s, d.type, constData())) {
        qWarning("QVariant::save: unable to save type '%s' (type id: %d).\n", QMetaType::typeName(d.type), d.type);
        Q_ASSERT_X(false, "QVariant::save", "Invalid type to save");
    }
}

/*!
    \since 4.4

    Reads a variant \a p from the stream \a s.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream& operator>>(QDataStream &s, QVariant &p)
{
    p.load(s);
    return s;
}

/*!
    Writes a variant \a p to the stream \a s.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream& operator<<(QDataStream &s, const QVariant &p)
{
    p.save(s);
    return s;
}

/*!
    Reads a variant type \a p in enum representation from the stream \a s.
*/
QDataStream& operator>>(QDataStream &s, QVariant::Type &p)
{
    quint32 u;
    s >> u;
    p = (QVariant::Type)u;

    return s;
}

/*!
    Writes a variant type \a p to the stream \a s.
*/
QDataStream& operator<<(QDataStream &s, const QVariant::Type p)
{
    s << static_cast<quint32>(p);

    return s;
}

#endif //QT_NO_DATASTREAM

/*!
    \fn bool QVariant::isValid() const

    Returns \c true if the storage type of this variant is not
    QMetaType::UnknownType; otherwise returns \c false.
*/

template <typename T>
inline T qVariantToHelper(const QVariant::Private &d, const HandlersManager &handlerManager)
{
    const uint targetType = qMetaTypeId<T>();
    if (d.type == targetType)
        return *v_cast<T>(&d);

    T ret;
    if (d.type >= QMetaType::User || targetType >= QMetaType::User) {
        const void * const from = constData(d);
        if (QMetaType::convert(from, d.type, &ret, targetType))
            return ret;
    }

    handlerManager[d.type]->convert(&d, targetType, &ret, 0);
    return ret;
}

/*!
    \fn QStringList QVariant::toStringList() const

    Returns the variant as a QStringList if the variant has userType()
    \l QMetaType::QStringList, \l QMetaType::QString, or
    \l QMetaType::QVariantList of a type that can be converted to QString;
    otherwise returns an empty list.

    \sa canConvert(int targetTypeId), convert()
*/
QStringList QVariant::toStringList() const
{
    return qVariantToHelper<QStringList>(d, handlerManager);
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

    \sa canConvert(int targetTypeId), convert()
*/
QString QVariant::toString() const
{
    return qVariantToHelper<QString>(d, handlerManager);
}

/*!
    Returns the variant as a QMap<QString, QVariant> if the variant
    has type() \l QMetaType::QVariantMap; otherwise returns an empty map.

    \sa canConvert(int targetTypeId), convert()
*/
QVariantMap QVariant::toMap() const
{
    return qVariantToHelper<QVariantMap>(d, handlerManager);
}

/*!
    Returns the variant as a QHash<QString, QVariant> if the variant
    has type() \l QMetaType::QVariantHash; otherwise returns an empty map.

    \sa canConvert(int targetTypeId), convert()
*/
QVariantHash QVariant::toHash() const
{
    return qVariantToHelper<QVariantHash>(d, handlerManager);
}

/*!
    \fn QDate QVariant::toDate() const

    Returns the variant as a QDate if the variant has userType()
    \l QMetaType::QDate, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid date.

    If the type() is \l QMetaType::QString, an invalid date will be returned if
    the string cannot be parsed as a Qt::ISODate format date.

    \sa canConvert(int targetTypeId), convert()
*/
QDate QVariant::toDate() const
{
    return qVariantToHelper<QDate>(d, handlerManager);
}

/*!
    \fn QTime QVariant::toTime() const

    Returns the variant as a QTime if the variant has userType()
    \l QMetaType::QTime, \l QMetaType::QDateTime, or \l QMetaType::QString;
    otherwise returns an invalid time.

    If the type() is \l QMetaType::QString, an invalid time will be returned if
    the string cannot be parsed as a Qt::ISODate format time.

    \sa canConvert(int targetTypeId), convert()
*/
QTime QVariant::toTime() const
{
    return qVariantToHelper<QTime>(d, handlerManager);
}

/*!
    \fn QDateTime QVariant::toDateTime() const

    Returns the variant as a QDateTime if the variant has userType()
    \l QMetaType::QDateTime, \l QMetaType::QDate, or \l QMetaType::QString;
    otherwise returns an invalid date/time.

    If the type() is \l QMetaType::QString, an invalid date/time will be
    returned if the string cannot be parsed as a Qt::ISODate format date/time.

    \sa canConvert(int targetTypeId), convert()
*/
QDateTime QVariant::toDateTime() const
{
    return qVariantToHelper<QDateTime>(d, handlerManager);
}

/*!
    \since 4.7
    \fn QEasingCurve QVariant::toEasingCurve() const

    Returns the variant as a QEasingCurve if the variant has userType()
    \l QMetaType::QEasingCurve; otherwise returns a default easing curve.

    \sa canConvert(int targetTypeId), convert()
*/
#ifndef QT_BOOTSTRAPPED
QEasingCurve QVariant::toEasingCurve() const
{
    return qVariantToHelper<QEasingCurve>(d, handlerManager);
}
#endif

/*!
    \fn QByteArray QVariant::toByteArray() const

    Returns the variant as a QByteArray if the variant has userType()
    \l QMetaType::QByteArray or \l QMetaType::QString (converted using
    QString::fromUtf8()); otherwise returns an empty byte array.

    \sa canConvert(int targetTypeId), convert()
*/
QByteArray QVariant::toByteArray() const
{
    return qVariantToHelper<QByteArray>(d, handlerManager);
}

#ifndef QT_NO_GEOM_VARIANT
/*!
    \fn QPoint QVariant::toPoint() const

    Returns the variant as a QPoint if the variant has userType()
    \l QMetaType::QPoint or \l QMetaType::QPointF; otherwise returns a null
    QPoint.

    \sa canConvert(int targetTypeId), convert()
*/
QPoint QVariant::toPoint() const
{
    return qVariantToHelper<QPoint>(d, handlerManager);
}

/*!
    \fn QRect QVariant::toRect() const

    Returns the variant as a QRect if the variant has userType()
    \l QMetaType::QRect; otherwise returns an invalid QRect.

    \sa canConvert(int targetTypeId), convert()
*/
QRect QVariant::toRect() const
{
    return qVariantToHelper<QRect>(d, handlerManager);
}

/*!
    \fn QSize QVariant::toSize() const

    Returns the variant as a QSize if the variant has userType()
    \l QMetaType::QSize; otherwise returns an invalid QSize.

    \sa canConvert(int targetTypeId), convert()
*/
QSize QVariant::toSize() const
{
    return qVariantToHelper<QSize>(d, handlerManager);
}

/*!
    \fn QSizeF QVariant::toSizeF() const

    Returns the variant as a QSizeF if the variant has userType() \l
    QMetaType::QSizeF; otherwise returns an invalid QSizeF.

    \sa canConvert(int targetTypeId), convert()
*/
QSizeF QVariant::toSizeF() const
{
    return qVariantToHelper<QSizeF>(d, handlerManager);
}

/*!
    \fn QRectF QVariant::toRectF() const

    Returns the variant as a QRectF if the variant has userType()
    \l QMetaType::QRect or \l QMetaType::QRectF; otherwise returns an invalid
    QRectF.

    \sa canConvert(int targetTypeId), convert()
*/
QRectF QVariant::toRectF() const
{
    return qVariantToHelper<QRectF>(d, handlerManager);
}

/*!
    \fn QLineF QVariant::toLineF() const

    Returns the variant as a QLineF if the variant has userType()
    \l QMetaType::QLineF; otherwise returns an invalid QLineF.

    \sa canConvert(int targetTypeId), convert()
*/
QLineF QVariant::toLineF() const
{
    return qVariantToHelper<QLineF>(d, handlerManager);
}

/*!
    \fn QLine QVariant::toLine() const

    Returns the variant as a QLine if the variant has userType()
    \l QMetaType::QLine; otherwise returns an invalid QLine.

    \sa canConvert(int targetTypeId), convert()
*/
QLine QVariant::toLine() const
{
    return qVariantToHelper<QLine>(d, handlerManager);
}

/*!
    \fn QPointF QVariant::toPointF() const

    Returns the variant as a QPointF if the variant has userType() \l
    QMetaType::QPoint or \l QMetaType::QPointF; otherwise returns a null
    QPointF.

    \sa canConvert(int targetTypeId), convert()
*/
QPointF QVariant::toPointF() const
{
    return qVariantToHelper<QPointF>(d, handlerManager);
}

#endif // QT_NO_GEOM_VARIANT

#ifndef QT_BOOTSTRAPPED
/*!
    \fn QUrl QVariant::toUrl() const

    Returns the variant as a QUrl if the variant has userType()
    \l QMetaType::QUrl; otherwise returns an invalid QUrl.

    \sa canConvert(int targetTypeId), convert()
*/
QUrl QVariant::toUrl() const
{
    return qVariantToHelper<QUrl>(d, handlerManager);
}
#endif

/*!
    \fn QLocale QVariant::toLocale() const

    Returns the variant as a QLocale if the variant has userType()
    \l QMetaType::QLocale; otherwise returns an invalid QLocale.

    \sa canConvert(int targetTypeId), convert()
*/
QLocale QVariant::toLocale() const
{
    return qVariantToHelper<QLocale>(d, handlerManager);
}

/*!
    \fn QRegExp QVariant::toRegExp() const
    \since 4.1

    Returns the variant as a QRegExp if the variant has userType()
    \l QMetaType::QRegExp; otherwise returns an empty QRegExp.

    \sa canConvert(int targetTypeId), convert()
*/
#ifndef QT_NO_REGEXP
QRegExp QVariant::toRegExp() const
{
    return qVariantToHelper<QRegExp>(d, handlerManager);
}
#endif

#if QT_CONFIG(regularexpression)
/*!
    \fn QRegularExpression QVariant::toRegularExpression() const
    \since 5.0

    Returns the variant as a QRegularExpression if the variant has userType() \l
    QRegularExpression; otherwise returns an empty QRegularExpression.

    \sa canConvert(int targetTypeId), convert()
*/
QRegularExpression QVariant::toRegularExpression() const
{
    return qVariantToHelper<QRegularExpression>(d, handlerManager);
}
#endif // QT_CONFIG(regularexpression)

#if QT_CONFIG(itemmodel)
/*!
    \since 5.0

    Returns the variant as a QModelIndex if the variant has userType() \l
    QModelIndex; otherwise returns a default constructed QModelIndex.

    \sa canConvert(int targetTypeId), convert(), toPersistentModelIndex()
*/
QModelIndex QVariant::toModelIndex() const
{
    return qVariantToHelper<QModelIndex>(d, handlerManager);
}

/*!
    \since 5.5

    Returns the variant as a QPersistentModelIndex if the variant has userType() \l
    QPersistentModelIndex; otherwise returns a default constructed QPersistentModelIndex.

    \sa canConvert(int targetTypeId), convert(), toModelIndex()
*/
QPersistentModelIndex QVariant::toPersistentModelIndex() const
{
    return qVariantToHelper<QPersistentModelIndex>(d, handlerManager);
}
#endif // QT_CONFIG(itemmodel)

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.0

    Returns the variant as a QUuid if the variant has type()
    \l QMetaType::QUuid, \l QMetaType::QByteArray or \l QMetaType::QString;
    otherwise returns a default-constructed QUuid.

    \sa canConvert(int targetTypeId), convert()
*/
QUuid QVariant::toUuid() const
{
    return qVariantToHelper<QUuid>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonValue if the variant has userType() \l
    QJsonValue; otherwise returns a default constructed QJsonValue.

    \sa canConvert(int targetTypeId), convert()
*/
QJsonValue QVariant::toJsonValue() const
{
    return qVariantToHelper<QJsonValue>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonObject if the variant has userType() \l
    QJsonObject; otherwise returns a default constructed QJsonObject.

    \sa canConvert(int targetTypeId), convert()
*/
QJsonObject QVariant::toJsonObject() const
{
    return qVariantToHelper<QJsonObject>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonArray if the variant has userType() \l
    QJsonArray; otherwise returns a default constructed QJsonArray.

    \sa canConvert(int targetTypeId), convert()
*/
QJsonArray QVariant::toJsonArray() const
{
    return qVariantToHelper<QJsonArray>(d, handlerManager);
}

/*!
    \since 5.0

    Returns the variant as a QJsonDocument if the variant has userType() \l
    QJsonDocument; otherwise returns a default constructed QJsonDocument.

    \sa canConvert(int targetTypeId), convert()
*/
QJsonDocument QVariant::toJsonDocument() const
{
    return qVariantToHelper<QJsonDocument>(d, handlerManager);
}
#endif // QT_BOOTSTRAPPED

/*!
    \fn QChar QVariant::toChar() const

    Returns the variant as a QChar if the variant has userType()
    \l QMetaType::QChar, \l QMetaType::Int, or \l QMetaType::UInt; otherwise
    returns an invalid QChar.

    \sa canConvert(int targetTypeId), convert()
*/
QChar QVariant::toChar() const
{
    return qVariantToHelper<QChar>(d, handlerManager);
}

/*!
    Returns the variant as a QBitArray if the variant has userType()
    \l QMetaType::QBitArray; otherwise returns an empty bit array.

    \sa canConvert(int targetTypeId), convert()
*/
QBitArray QVariant::toBitArray() const
{
    return qVariantToHelper<QBitArray>(d, handlerManager);
}

template <typename T>
inline T qNumVariantToHelper(const QVariant::Private &d,
                             const HandlersManager &handlerManager, bool *ok, const T& val)
{
    const uint t = qMetaTypeId<T>();
    if (ok)
        *ok = true;

    if (d.type == t)
        return val;

    T ret = 0;
    if ((d.type >= QMetaType::User || t >= QMetaType::User)
        && QMetaType::convert(constData(d), d.type, &ret, t))
        return ret;

    if (!handlerManager[d.type]->convert(&d, t, &ret, ok) && ok)
        *ok = false;
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

    \sa canConvert(int targetTypeId), convert()
*/
int QVariant::toInt(bool *ok) const
{
    return qNumVariantToHelper<int>(d, handlerManager, ok, d.data.i);
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

    \sa canConvert(int targetTypeId), convert()
*/
uint QVariant::toUInt(bool *ok) const
{
    return qNumVariantToHelper<uint>(d, handlerManager, ok, d.data.u);
}

/*!
    Returns the variant as a long long int if the variant has userType()
    \l QMetaType::LongLong, \l QMetaType::Bool, \l QMetaType::QByteArray,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::QString, \l QMetaType::UInt, or \l QMetaType::ULongLong;
    otherwise returns 0.

    If \a ok is non-null: \c{*}\c{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\c{ok} is set to false.

    \sa canConvert(int targetTypeId), convert()
*/
qlonglong QVariant::toLongLong(bool *ok) const
{
    return qNumVariantToHelper<qlonglong>(d, handlerManager, ok, d.data.ll);
}

/*!
    Returns the variant as an unsigned long long int if the
    variant has type() \l QMetaType::ULongLong, \l QMetaType::Bool,
    \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
    \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString, or
    \l QMetaType::UInt; otherwise returns 0.

    If \a ok is non-null: \c{*}\a{ok} is set to true if the value could be
    converted to an int; otherwise \c{*}\a{ok} is set to false.

    \sa canConvert(int targetTypeId), convert()
*/
qulonglong QVariant::toULongLong(bool *ok) const
{
    return qNumVariantToHelper<qulonglong>(d, handlerManager, ok, d.data.ull);
}

/*!
    Returns the variant as a bool if the variant has userType() Bool.

    Returns \c true if the variant has userType() \l QMetaType::Bool,
    \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
    \l QMetaType::LongLong, \l QMetaType::UInt, or \l QMetaType::ULongLong and
    the value is non-zero, or if the variant has type \l QMetaType::QString or
    \l QMetaType::QByteArray and its lower-case content is not one of the
    following: empty, "0" or "false"; otherwise returns \c false.

    \sa canConvert(int targetTypeId), convert()
*/
bool QVariant::toBool() const
{
    if (d.type == Bool)
        return d.data.b;

    bool res = false;
    handlerManager[d.type]->convert(&d, Bool, &res, 0);

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

    \sa canConvert(int targetTypeId), convert()
*/
double QVariant::toDouble(bool *ok) const
{
    return qNumVariantToHelper<double>(d, handlerManager, ok, d.data.d);
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

    \sa canConvert(int targetTypeId), convert()
*/
float QVariant::toFloat(bool *ok) const
{
    return qNumVariantToHelper<float>(d, handlerManager, ok, d.data.f);
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

    \sa canConvert(int targetTypeId), convert()
*/
qreal QVariant::toReal(bool *ok) const
{
    return qNumVariantToHelper<qreal>(d, handlerManager, ok, d.data.real);
}

/*!
    Returns the variant as a QVariantList if the variant has userType()
    \l QMetaType::QVariantList or \l QMetaType::QStringList; otherwise returns
    an empty list.

    \sa canConvert(int targetTypeId), convert()
*/
QVariantList QVariant::toList() const
{
    return qVariantToHelper<QVariantList>(d, handlerManager);
}


static const quint32 qCanConvertMatrix[QVariant::LastCoreType + 1] =
{
/*Invalid*/     0,

/*Bool*/          1 << QVariant::Double     | 1 << QVariant::Int        | 1 << QVariant::UInt
                | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong  | 1 << QVariant::ByteArray
                | 1 << QVariant::String     | 1 << QVariant::Char,

/*Int*/           1 << QVariant::UInt       | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray  | 1 << QVariant::Int,

/*UInt*/          1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*LLong*/         1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::ULongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*ULlong*/        1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::Double
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::Char       | 1 << QVariant::ByteArray,

/*double*/        1 << QVariant::Int        | 1 << QVariant::String     | 1 << QVariant::ULongLong
                | 1 << QVariant::Bool       | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::ByteArray,

/*QChar*/         1 << QVariant::Int        | 1 << QVariant::UInt       | 1 << QVariant::LongLong
                | 1 << QVariant::ULongLong,

/*QMap*/          0,

/*QList*/         1 << QVariant::StringList,

/*QString*/       1 << QVariant::StringList | 1 << QVariant::ByteArray  | 1 << QVariant::Int
                | 1 << QVariant::UInt       | 1 << QVariant::Bool       | 1 << QVariant::Double
                | 1 << QVariant::Date       | 1 << QVariant::Time       | 1 << QVariant::DateTime
                | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong  | 1 << QVariant::Char
                | 1 << QVariant::Url        | 1 << QVariant::Uuid,

/*QStringList*/   1 << QVariant::List       | 1 << QVariant::String,

/*QByteArray*/    1 << QVariant::String     | 1 << QVariant::Int        | 1 << QVariant::UInt | 1 << QVariant::Bool
                | 1 << QVariant::Double     | 1 << QVariant::LongLong   | 1 << QVariant::ULongLong
                | 1 << QVariant::Uuid,

/*QBitArray*/     0,

/*QDate*/         1 << QVariant::String     | 1 << QVariant::DateTime,

/*QTime*/         1 << QVariant::String     | 1 << QVariant::DateTime,

/*QDateTime*/     1 << QVariant::String     | 1 << QVariant::Date,

/*QUrl*/          1 << QVariant::String,

/*QLocale*/       0,

/*QRect*/         1 << QVariant::RectF,

/*QRectF*/        1 << QVariant::Rect,

/*QSize*/         1 << QVariant::SizeF,

/*QSizeF*/        1 << QVariant::Size,

/*QLine*/         1 << QVariant::LineF,

/*QLineF*/        1 << QVariant::Line,

/*QPoint*/        1 << QVariant::PointF,

/*QPointF*/       1 << QVariant::Point,

/*QRegExp*/       0,

/*QHash*/         0,

/*QEasingCurve*/  0,

/*QUuid*/         1 << QVariant::String     | 1 << QVariant::ByteArray,
};
static const size_t qCanConvertMatrixMaximumTargetType = 8 * sizeof(*qCanConvertMatrix);

#ifndef QT_BOOTSTRAPPED
/*
    Returns \c true if from inherits to.
*/
static bool canConvertMetaObject(const QMetaObject *from, const QMetaObject *to)
{
    if (from && to == &QObject::staticMetaObject)
        return true;

    while (from) {
        if (from == to)
            return true;
        from = from->superClass();
    }

    return false;
}
#endif

static bool canConvertMetaObject(int fromId, int toId, QObject *fromObject)
{
#ifndef QT_BOOTSTRAPPED
    QMetaType toType(toId);
    if ((QMetaType::typeFlags(fromId) & QMetaType::PointerToQObject) && (toType.flags() & QMetaType::PointerToQObject)) {
        if (!fromObject)
            return true;
        return canConvertMetaObject(fromObject->metaObject(), toType.metaObject());
    }
#else
    Q_UNUSED(fromId);
    Q_UNUSED(toId);
    Q_UNUSED(fromObject);
#endif
    return false;
}


/*!
    Returns \c true if the variant's type can be cast to the requested
    type, \a targetTypeId. Such casting is done automatically when calling the
    toInt(), toBool(), ... methods.

    The following casts are done automatically:

    \table
    \header \li Type \li Automatically Cast To
    \row \li \l QMetaType::Bool \li \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QByteArray \li \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong, \l QMetaType::QUuid
    \row \li \l QMetaType::QChar \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::UInt, \l QMetaType::LongLong, \l QMetaType::ULongLong
    \row \li \l QMetaType::QColor \li \l QMetaType::QString
    \row \li \l QMetaType::QDate \li \l QMetaType::QDateTime,
        \l QMetaType::QString
    \row \li \l QMetaType::QDateTime \li \l QMetaType::QDate,
        \l QMetaType::QString, \l QMetaType::QTime
    \row \li \l QMetaType::Double \li \l QMetaType::Bool, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QFont \li \l QMetaType::QString
    \row \li \l QMetaType::Int \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::LongLong, \l QMetaType::QString,
        \l QMetaType::UInt, \l QMetaType::ULongLong
    \row \li \l QMetaType::QKeySequence \li \l QMetaType::Int,
        \l QMetaType::QString
    \row \li \l QMetaType::QVariantList \li \l QMetaType::QStringList (if the
        list's items can be converted to QStrings)
    \row \li \l QMetaType::LongLong \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::Double,
        \l QMetaType::Int, \l QMetaType::QString, \l QMetaType::UInt,
        \l QMetaType::ULongLong
    \row \li \l QMetaType::QPoint \li QMetaType::QPointF
    \row \li \l QMetaType::QRect \li QMetaType::QRectF
    \row \li \l QMetaType::QString \li \l QMetaType::Bool,
        \l QMetaType::QByteArray, \l QMetaType::QChar, \l QMetaType::QColor,
        \l QMetaType::QDate, \l QMetaType::QDateTime, \l QMetaType::Double,
        \l QMetaType::QFont, \l QMetaType::Int, \l QMetaType::QKeySequence,
        \l QMetaType::LongLong, \l QMetaType::QStringList, \l QMetaType::QTime,
        \l QMetaType::UInt, \l QMetaType::ULongLong, \l QMetaType::QUuid
    \row \li \l QMetaType::QStringList \li \l QMetaType::QVariantList,
        \l QMetaType::QString (if the list contains exactly one item)
    \row \li \l QMetaType::QTime \li \l QMetaType::QString
    \row \li \l QMetaType::UInt \li \l QMetaType::Bool, \l QMetaType::QChar,
        \l QMetaType::Double, \l QMetaType::Int, \l QMetaType::LongLong,
        \l QMetaType::QString, \l QMetaType::ULongLong
    \row \li \l QMetaType::ULongLong \li \l QMetaType::Bool,
        \l QMetaType::QChar, \l QMetaType::Double, \l QMetaType::Int,
        \l QMetaType::LongLong, \l QMetaType::QString, \l QMetaType::UInt
    \row \li \l QMetaType::QUuid \li \l QMetaType::QByteArray, \l QMetaType::QString
    \endtable

    A QVariant containing a pointer to a type derived from QObject will also return true for this
    function if a qobject_cast to the type described by \a targetTypeId would succeed. Note that
    this only works for QObject subclasses which use the Q_OBJECT macro.

    A QVariant containing a sequential container will also return true for this
    function if the \a targetTypeId is QVariantList. It is possible to iterate over
    the contents of the container without extracting it as a (copied) QVariantList:

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    This requires that the value_type of the container is itself a metatype.

    Similarly, a QVariant containing a sequential container will also return true for this
    function the \a targetTypeId is QVariantHash or QVariantMap. It is possible to iterate over
    the contents of the container without extracting it as a (copied) QVariantHash or QVariantMap:

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa convert(), QSequentialIterable, Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(), QAssociativeIterable,
        Q_DECLARE_ASSOCIATIVE_CONTAINER_METATYPE()
*/
bool QVariant::canConvert(int targetTypeId) const
{
    if (d.type == targetTypeId)
        return true;

#if QT_CONFIG(itemmodel)
    if ((targetTypeId == QMetaType::QModelIndex && d.type == QMetaType::QPersistentModelIndex)
        || (targetTypeId == QMetaType::QPersistentModelIndex && d.type == QMetaType::QModelIndex))
        return true;
#endif

    if (targetTypeId == QMetaType::QVariantList
            && (d.type == QMetaType::QVariantList
              || d.type == QMetaType::QStringList
              || d.type == QMetaType::QByteArrayList
              || QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QSequentialIterableImpl>()))) {
        return true;
    }

    if ((targetTypeId == QMetaType::QVariantHash || targetTypeId == QMetaType::QVariantMap)
            && (d.type == QMetaType::QVariantMap
              || d.type == QMetaType::QVariantHash
              || QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QAssociativeIterableImpl>()))) {
        return true;
    }

    if (targetTypeId == qMetaTypeId<QPair<QVariant, QVariant> >() &&
              QMetaType::hasRegisteredConverterFunction(d.type,
                    qMetaTypeId<QtMetaTypePrivate::QPairVariantInterfaceImpl>())) {
        return true;
    }

    if ((d.type >= QMetaType::User || targetTypeId >= QMetaType::User)
        && QMetaType::hasRegisteredConverterFunction(d.type, targetTypeId)) {
        return true;
    }

    // TODO Reimplement this function, currently it works but it is a historical mess.
    uint currentType = ((d.type == QMetaType::Float) ? QVariant::Double : d.type);
    if (currentType == QMetaType::SChar || currentType == QMetaType::Char)
        currentType = QMetaType::UInt;
    if (targetTypeId == QMetaType::SChar || currentType == QMetaType::Char)
        targetTypeId = QMetaType::UInt;
    if (uint(targetTypeId) == uint(QMetaType::Float)) targetTypeId = QVariant::Double;


    if (currentType == uint(targetTypeId))
        return true;

    if (targetTypeId < 0)
        return false;
    if (targetTypeId >= QMetaType::User) {
        if (QMetaType::typeFlags(targetTypeId) & QMetaType::IsEnumeration) {
            targetTypeId = QMetaType::Int;
        } else {
            return canConvertMetaObject(currentType, targetTypeId, d.data.o);
        }
    }

    if (currentType == QMetaType::QJsonValue || targetTypeId == QMetaType::QJsonValue) {
        switch (currentType == QMetaType::QJsonValue ? targetTypeId : currentType) {
        case QMetaType::Nullptr:
        case QMetaType::QString:
        case QMetaType::Bool:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Double:
        case QMetaType::Float:
        case QMetaType::ULong:
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::UShort:
        case QMetaType::UChar:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::QVariantList:
        case QMetaType::QVariantMap:
        case QMetaType::QVariantHash:
            return true;
        default:
            return false;
        }
    }
    if (currentType == QMetaType::QJsonArray)
        return targetTypeId == QMetaType::QVariantList;
    if (currentType == QMetaType::QJsonObject)
        return targetTypeId == QMetaType::QVariantMap || targetTypeId == QMetaType::QVariantHash;

    // FIXME It should be LastCoreType intead of Uuid
    if (currentType > int(QMetaType::QUuid) || targetTypeId > int(QMetaType::QUuid)) {
        switch (uint(targetTypeId)) {
        case QVariant::Int:
            if (currentType == QVariant::KeySequence)
                return true;
            Q_FALLTHROUGH();
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
               return currentType == QMetaType::ULong
                   || currentType == QMetaType::Long
                   || currentType == QMetaType::UShort
                   || currentType == QMetaType::UChar
                   || currentType == QMetaType::Char
                   || currentType == QMetaType::SChar
                   || currentType == QMetaType::Short
                   || QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration;
        case QVariant::Image:
            return currentType == QVariant::Pixmap || currentType == QVariant::Bitmap;
        case QVariant::Pixmap:
            return currentType == QVariant::Image || currentType == QVariant::Bitmap
                              || currentType == QVariant::Brush;
        case QVariant::Bitmap:
            return currentType == QVariant::Pixmap || currentType == QVariant::Image;
        case QVariant::ByteArray:
            return currentType == QVariant::Color || currentType == QMetaType::Nullptr
                              || ((QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration) && QMetaType::metaObjectForType(currentType));
        case QVariant::String:
            return currentType == QVariant::KeySequence || currentType == QVariant::Font
                              || currentType == QVariant::Color || currentType == QMetaType::Nullptr
                              || ((QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration) && QMetaType::metaObjectForType(currentType));
        case QVariant::KeySequence:
            return currentType == QVariant::String || currentType == QVariant::Int;
        case QVariant::Font:
            return currentType == QVariant::String;
        case QVariant::Color:
            return currentType == QVariant::String || currentType == QVariant::ByteArray
                              || currentType == QVariant::Brush;
        case QVariant::Brush:
            return currentType == QVariant::Color || currentType == QVariant::Pixmap;
        case QMetaType::Long:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
        case QMetaType::ULong:
        case QMetaType::Short:
        case QMetaType::UShort:
            return currentType == QVariant::Int
                || (currentType < qCanConvertMatrixMaximumTargetType
                    && qCanConvertMatrix[QVariant::Int] & (1U << currentType))
                || QMetaType::typeFlags(currentType) & QMetaType::IsEnumeration;
        case QMetaType::QObjectStar:
            return canConvertMetaObject(currentType, targetTypeId, d.data.o);
        default:
            return false;
        }
    }

    if (targetTypeId == String && currentType == StringList)
        return v_cast<QStringList>(&d)->count() == 1;
    return currentType < qCanConvertMatrixMaximumTargetType
        && qCanConvertMatrix[targetTypeId] & (1U << currentType);
}

/*!
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

    \sa canConvert(int targetTypeId), clear()
*/

bool QVariant::convert(int targetTypeId)
{
    if (d.type == uint(targetTypeId))
        return true;

    QVariant oldValue = *this;

    clear();
    if (!oldValue.canConvert(targetTypeId))
        return false;

    create(targetTypeId, 0);
    // Fail if the value is not initialized or was forced null by a previous failed convert.
    if (oldValue.d.is_null)
        return false;

    if ((QMetaType::typeFlags(oldValue.userType()) & QMetaType::PointerToQObject) && (QMetaType::typeFlags(targetTypeId) & QMetaType::PointerToQObject)) {
        create(targetTypeId, &oldValue.d.data.o);
        return true;
    }

    bool isOk = true;
    int converterType = std::max(oldValue.userType(), targetTypeId);
    if (!handlerManager[converterType]->convert(&oldValue.d, targetTypeId, data(), &isOk))
        isOk = false;
    d.is_null = !isOk;
    return isOk;
}

/*!
  \fn bool QVariant::convert(const int type, void *ptr) const
  \internal
  Created for qvariant_cast() usage
*/
bool QVariant::convert(const int type, void *ptr) const
{
    return handlerManager[type]->convert(&d, type, ptr, 0);
}


/*!
    \fn bool operator==(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns \c true if \a v1 and \a v2 are equal; otherwise returns \c false.

    If \a v1 and \a v2 have the same \l{QVariant::}{type()}, the
    type's equality operator is used for comparison. If not, it is
    attempted to \l{QVariant::}{convert()} \a v2 to the same type as
    \a v1. See \l{QVariant::}{canConvert()} for a list of possible
    conversions.

    The result of the function is not affected by the result of QVariant::isNull,
    which means that two values can be equal even if one of them is null and
    another is not.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/
/*!
    \fn bool operator!=(const QVariant &v1, const QVariant &v2)

    \relates QVariant

    Returns \c false if \a v1 and \a v2 are equal; otherwise returns \c true.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*! \fn bool QVariant::operator==(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if they are
    equal; otherwise returns \c false.

    QVariant uses the equality operator of the type() it contains to
    check for equality. QVariant will try to convert() \a v if its
    type is not the same as this variant's type. See canConvert() for
    a list of possible conversions.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator!=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if they are not
    equal; otherwise returns \c false.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator<(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is less than \a v.

    \note Comparability might not be availabe for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator<=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is less or equal than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator>(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is larger than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

/*!
    \fn bool QVariant::operator>=(const QVariant &v) const

    Compares this QVariant with \a v and returns \c true if this is larger or equal than \a v.

    \note Comparability might not be available for the type stored in this QVariant
    or in \a v.

    \warning To make this function work with a custom type registered with
    qRegisterMetaType(), its comparison operator must be registered using
    QMetaType::registerComparators().
*/

static bool qIsNumericType(uint tp)
{
    static const qulonglong numericTypeBits =
            Q_UINT64_C(1) << QMetaType::Bool |
            Q_UINT64_C(1) << QMetaType::Double |
            Q_UINT64_C(1) << QMetaType::Float |
            Q_UINT64_C(1) << QMetaType::Char |
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
    return tp == QVariant::Double || tp == QMetaType::Float;
}

static int normalizeLowerRanks(uint tp)
{
    static const qulonglong numericTypeBits =
            Q_UINT64_C(1) << QMetaType::Bool |
            Q_UINT64_C(1) << QMetaType::Char |
            Q_UINT64_C(1) << QMetaType::SChar |
            Q_UINT64_C(1) << QMetaType::UChar |
            Q_UINT64_C(1) << QMetaType::Short |
            Q_UINT64_C(1) << QMetaType::UShort;
    return numericTypeBits & (Q_UINT64_C(1) << tp) ? QVariant::Int : tp;
}

static int normalizeLong(uint tp)
{
    const uint IntType = sizeof(long) == sizeof(int) ? QVariant::Int : QVariant::LongLong;
    const uint UIntType = sizeof(ulong) == sizeof(uint) ? QVariant::UInt : QVariant::ULongLong;
    return tp == QMetaType::Long ? IntType :
           tp == QMetaType::ULong ? UIntType : tp;
}

static int numericTypePromotion(uint t1, uint t2)
{
    Q_ASSERT(qIsNumericType(t1));
    Q_ASSERT(qIsNumericType(t2));

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

    // integral rules:
    // for all platforms we support, int can always hold the values of lower-ranked types
    t1 = normalizeLowerRanks(t1);
    t2 = normalizeLowerRanks(t2);

    // normalize long / ulong: in all platforms we run, they're either the same as int or as long long
    t1 = normalizeLong(t1);
    t2 = normalizeLong(t2);

    // implement the other rules
    // the four possibilities are Int, UInt, LongLong and ULongLong
    // if any of the two is ULongLong, then it wins (highest rank, unsigned)
    // otherwise, if one of the two is LongLong, then the other is either LongLong too or lower-ranked
    // otherwise, if one of the two is UInt, then the other is either UInt too or Int
    if (t1 == QVariant::ULongLong || t2 == QVariant::ULongLong)
        return QVariant::ULongLong;
    if (t1 == QVariant::LongLong || t2 == QVariant::LongLong)
        return QVariant::LongLong;
    if (t1 == QVariant::UInt || t2 == QVariant::UInt)
        return QVariant::UInt;
    return QVariant::Int;
}

static int integralCompare(uint promotedType, const QVariant::Private *d1, const QVariant::Private *d2)
{
    // use toLongLong to retrieve the data, it gets us all the bits
    bool ok;
    qlonglong l1 = qConvertToNumber(d1, &ok);
    Q_ASSERT(ok);

    qlonglong l2 = qConvertToNumber(d2, &ok);
    Q_ASSERT(ok);

    if (promotedType == QVariant::Int)
        return int(l1) < int(l2) ? -1 : int(l1) == int(l2) ? 0 : 1;
    if (promotedType == QVariant::UInt)
        return uint(l1) < uint(l2) ? -1 : uint(l1) == uint(l2) ? 0 : 1;
    if (promotedType == QVariant::LongLong)
        return l1 < l2 ? -1 : l1 == l2 ? 0 : 1;
    if (promotedType == QVariant::ULongLong)
        return qulonglong(l1) < qulonglong(l2) ? -1 : qulonglong(l1) == qulonglong(l2) ? 0 : 1;

    Q_UNREACHABLE();
    return 0;
}

static int numericCompare(const QVariant::Private *d1, const QVariant::Private *d2)
{
    uint promotedType = numericTypePromotion(d1->type, d2->type);
    if (promotedType != QMetaType::QReal)
        return integralCompare(promotedType, d1, d2);

    // qreal comparisons
    bool ok;
    qreal r1 = qConvertToRealNumber(d1, &ok);
    Q_ASSERT(ok);
    qreal r2 = qConvertToRealNumber(d2, &ok);
    Q_ASSERT(ok);
    if (r1 == r2)
        return 0;

    // only do fuzzy comparisons for finite, non-zero numbers
    int c1 = std::fpclassify(r1);
    int c2 = std::fpclassify(r2);
    if ((c1 == FP_NORMAL || c1 == FP_SUBNORMAL) && (c2 == FP_NORMAL || c2 == FP_SUBNORMAL)) {
        if (qFuzzyCompare(r1, r2))
            return 0;
    }

    return r1 < r2 ? -1 : 1;
}

/*!
    \internal
 */
bool QVariant::cmp(const QVariant &v) const
{
    auto cmp_helper = [] (const QVariant::Private &d1, const QVariant::Private &d2)
    {
        Q_ASSERT(d1.type == d2.type);
        if (d1.type >= QMetaType::User) {
            int result;
            if (QMetaType::equals(QT_PREPEND_NAMESPACE(constData(d1)), QT_PREPEND_NAMESPACE(constData(d2)), d1.type, &result))
                return result == 0;
        }
        return handlerManager[d1.type]->compare(&d1, &d2);
    };

    // try numerics first, with C++ type promotion rules (no conversion)
    if (qIsNumericType(d.type) && qIsNumericType(v.d.type))
        return numericCompare(&d, &v.d) == 0;

    if (d.type == v.d.type)
        return cmp_helper(d, v.d);

    QVariant v1 = *this;
    QVariant v2 = v;
    if (v2.canConvert(v1.d.type)) {
        if (!v2.convert(v1.d.type))
            return false;
    } else {
        // try the opposite conversion, it might work
        qSwap(v1, v2);
        if (!v2.convert(v1.d.type))
            return false;
    }
    return cmp_helper(v1.d, v2.d);
}

/*!
    \internal
 */
int QVariant::compare(const QVariant &v) const
{
    // try numerics first, with C++ type promotion rules (no conversion)
    if (qIsNumericType(d.type) && qIsNumericType(v.d.type))
        return numericCompare(&d, &v.d);

    // check for equality next, as more types implement operator== than operator<
    if (cmp(v))
        return 0;

    const QVariant *v1 = this;
    const QVariant *v2 = &v;
    QVariant converted1;
    QVariant converted2;

    if (d.type != v.d.type) {
        // if both types differ, try to convert
        if (v2->canConvert(v1->d.type)) {
            converted2 = *v2;
            if (converted2.convert(v1->d.type))
                v2 = &converted2;
        }
        if (v1->d.type != v2->d.type && v1->canConvert(v2->d.type)) {
            converted1 = *v1;
            if (converted1.convert(v2->d.type))
                v1 = &converted1;
        }
        if (v1->d.type != v2->d.type) {
            // if conversion fails, default to toString
            int r = v1->toString().compare(v2->toString(), Qt::CaseInsensitive);
            if (r == 0) {
                // cmp(v) returned false, so we should try to agree with it.
                return (v1->d.type < v2->d.type) ? -1 : 1;
            }
            return r;
        }

        // did we end up with two numerics? If so, restart
        if (qIsNumericType(v1->d.type) && qIsNumericType(v2->d.type))
            return v1->compare(*v2);
    }
    if (v1->d.type >= QMetaType::User) {
        int result;
        if (QMetaType::compare(QT_PREPEND_NAMESPACE(constData(d)), QT_PREPEND_NAMESPACE(constData(v2->d)), d.type, &result))
            return result;
    }
    switch (v1->d.type) {
    case QVariant::Date:
        return v1->toDate() < v2->toDate() ? -1 : 1;
    case QVariant::Time:
        return v1->toTime() < v2->toTime() ? -1 : 1;
    case QVariant::DateTime:
        return v1->toDateTime() < v2->toDateTime() ? -1 : 1;
    case QVariant::StringList:
        return v1->toStringList() < v2->toStringList() ? -1 : 1;
    }
    int r = v1->toString().compare(v2->toString(), Qt::CaseInsensitive);
    if (r == 0) {
        // cmp(v) returned false, so we should try to agree with it.
        return (d.type < v.d.type) ? -1 : 1;
    }
    return r;
}

/*!
    \internal
 */

const void *QVariant::constData() const
{
    return d.is_shared ? d.data.shared->ptr : reinterpret_cast<const void *>(&d.data.ptr);
}

/*!
    \fn const void* QVariant::data() const

    \internal
*/

/*!
    \internal
*/
void* QVariant::data()
{
    detach();
    return const_cast<void *>(constData());
}


/*!
    Returns \c true if this is a null variant, false otherwise. A variant is
    considered null if it contains no initialized value, or the contained value
    is a null pointer or is an instance of a built-in type that has an isNull
    method, in which case the result would be the same as calling isNull on the
    wrapped object.

    \warning Null variants is not a single state and two null variants may easily
    return \c false on the == operator if they do not contain similar null values.

    \sa convert(int)
*/
bool QVariant::isNull() const
{
    return handlerManager[d.type]->isNull(&d);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QVariant &v)
{
    QDebugStateSaver saver(dbg);
    const uint typeId = v.d.type;
    dbg.nospace() << "QVariant(";
    if (typeId != QMetaType::UnknownType) {
        dbg << QMetaType::typeName(typeId) << ", ";
        bool userStream = false;
        bool canConvertToString = false;
        if (typeId >= QMetaType::User) {
            userStream = QMetaType::debugStream(dbg, constData(v.d), typeId);
            canConvertToString = v.canConvert<QString>();
        }
        if (!userStream && canConvertToString)
            dbg << v.toString();
        else if (!userStream)
            handlerManager[typeId]->debugStream(dbg, v);
    } else {
        dbg << "Invalid";
    }
    dbg << ')';
    return dbg;
}

QDebug operator<<(QDebug dbg, const QVariant::Type p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QVariant::"
                  << (int(p) != int(QMetaType::UnknownType)
                     ? QMetaType::typeName(p)
                     : "Invalid");
    return dbg;
}
#endif


/*! \fn template<typename T> void QVariant::setValue(const T &value)

    Stores a copy of \a value. If \c{T} is a type that QVariant
    doesn't support, QMetaType is used to store the value. A compile
    error will occur if QMetaType doesn't handle the type.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 4

    \sa value(), fromValue(), canConvert()
 */

/*! \fn template<typename T> T QVariant::value() const

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
    qobject_cast to T, then that result is returned. Otherwise a null pointer is
    returned. Note that this only works for QObject subclasses which use the
    Q_OBJECT macro.

    If the QVariant contains a sequential container and \c{T} is QVariantList, the
    elements of the container will be converted into \l {QVariant}s and returned as a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa setValue(), fromValue(), canConvert(), Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE()
*/

/*! \fn bool QVariant::canConvert() const

    Returns \c true if the variant can be converted to the template type \c{T},
    otherwise false.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 6

    A QVariant containing a pointer to a type derived from QObject will also return true for this
    function if a qobject_cast to the template type \c{T} would succeed. Note that this only works
    for QObject subclasses which use the Q_OBJECT macro.

    \sa convert()
*/

/*! \fn template<typename T> static QVariant QVariant::fromValue(const T &value)

    Returns a QVariant containing a copy of \a value. Behaves
    exactly like setValue() otherwise.

    Example:

    \snippet code/src_corelib_kernel_qvariant.cpp 7

    \note If you are working with custom types, you should use
    the Q_DECLARE_METATYPE() macro to register your custom type.

    \sa setValue(), value()
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
    \fn template<typename T> QVariant qVariantFromValue(const T &value)
    \relates QVariant
    \obsolete

    Returns a variant containing a copy of the given \a value
    with template type \c{T}.

    This function is equivalent to QVariant::fromValue(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    For example, a QObject pointer can be stored in a variant with the
    following code:

    \snippet code/src_corelib_kernel_qvariant.cpp 8

    \sa QVariant::fromValue()
*/

/*! \fn template<typename T> void qVariantSetValue(QVariant &variant, const T &value)
    \relates QVariant
    \obsolete

    Sets the contents of the given \a variant to a copy of the
    \a value with the specified template type \c{T}.

    This function is equivalent to QVariant::setValue(\a value).

    \note This function was provided as a workaround for MSVC 6
    which did not support member template functions. It is advised
    to use the other form in new code.

    \sa QVariant::setValue()
*/

/*!
    \fn template<typename T> T qvariant_cast(const QVariant &value)
    \relates QVariant

    Returns the given \a value converted to the template type \c{T}.

    This function is equivalent to QVariant::value().

    \sa QVariant::value()
*/

/*! \fn template<typename T> T qVariantValue(const QVariant &value)
    \relates QVariant
    \obsolete

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
    \obsolete

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
    \class QSequentialIterable
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable class is an iterable interface for a container in a QVariant.

    This class allows several methods of accessing the elements of a container held within
    a QVariant. An instance of QSequentialIterable can be extracted from a QVariant if it can
    be converted to a QVariantList.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \internal
*/
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QSequentialIterable::QSequentialIterable(QtMetaTypePrivate::QSequentialIterableImpl impl)
#else
QSequentialIterable::QSequentialIterable(const QtMetaTypePrivate::QSequentialIterableImpl &impl)
#endif
  : m_impl(impl)
{
}

QSequentialIterable::const_iterator::const_iterator(const QSequentialIterable &iter, QAtomicInt *ref_)
  : m_impl(iter.m_impl), ref(ref_)
{
    ref->ref();
}

QSequentialIterable::const_iterator::const_iterator(const QtMetaTypePrivate::QSequentialIterableImpl &impl, QAtomicInt *ref_)
  : m_impl(impl), ref(ref_)
{
    ref->ref();
}

void QSequentialIterable::const_iterator::begin()
{
    m_impl.moveToBegin();
}

void QSequentialIterable::const_iterator::end()
{
    m_impl.moveToEnd();
}

/*! \fn QSequentialIterable::const_iterator QSequentialIterable::begin() const

    Returns a QSequentialIterable::const_iterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/
QSequentialIterable::const_iterator QSequentialIterable::begin() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.begin();
    return it;
}

/*!
    Returns a QSequentialIterable::const_iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/
QSequentialIterable::const_iterator QSequentialIterable::end() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.end();
    return it;
}

/*!
    Returns the element at position \a idx in the container.
*/
QVariant QSequentialIterable::at(int idx) const
{
    const QtMetaTypePrivate::VariantData d = m_impl.at(idx);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return QVariant(d.metaTypeId, d.data, d.flags);
}

/*!
    Returns the number of elements in the container.
*/
int QSequentialIterable::size() const
{
    return m_impl.size();
}

/*!
    Returns whether it is possible to iterate over the container in reverse. This
    corresponds to the std::bidirectional_iterator_tag iterator trait of the
    const_iterator of the container.
*/
bool QSequentialIterable::canReverseIterate() const
{
    return m_impl._iteratorCapabilities & QtMetaTypePrivate::BiDirectionalCapability;
}

/*!
    \class QSequentialIterable::const_iterator
    \since 5.2
    \inmodule QtCore
    \brief The QSequentialIterable::const_iterator allows iteration over a container in a QVariant.

    A QSequentialIterable::const_iterator can only be created by a QSequentialIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 9

    \sa QSequentialIterable
*/


/*!
    Destroys the QSequentialIterable::const_iterator.
*/
QSequentialIterable::const_iterator::~const_iterator() {
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
}

/*!
    Creates a copy of \a other.
*/
QSequentialIterable::const_iterator::const_iterator(const const_iterator &other)
  : m_impl(other.m_impl), ref(other.ref)
{
    ref->ref();
}

/*!
    Assigns \a other to this.
*/
QSequentialIterable::const_iterator&
QSequentialIterable::const_iterator::operator=(const const_iterator &other)
{
    other.ref->ref();
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
    m_impl = other.m_impl;
    ref = other.ref;
    return *this;
}

/*!
    Returns the current item, converted to a QVariant.
*/
const QVariant QSequentialIterable::const_iterator::operator*() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrent();
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return QVariant(d.metaTypeId, d.data, d.flags);
}

/*!
    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/
bool QSequentialIterable::const_iterator::operator==(const const_iterator &other) const
{
    return m_impl.equal(other.m_impl);
}

/*!
    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/
bool QSequentialIterable::const_iterator::operator!=(const const_iterator &other) const
{
    return !m_impl.equal(other.m_impl);
}

/*!
    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QSequentialIterable::end() leads to undefined results.

    \sa operator--()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator++()
{
    m_impl.advance(1);
    return *this;
}

/*!
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator++(int)
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QSequentialIterable::begin() leads to undefined results.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator++(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator--()
{
    m_impl.advance(-1);
    return *this;
}

/*!
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator--(int)
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(-1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator+=(int j)
{
    m_impl.advance(j);
    return *this;
}

/*!
    Makes the iterator go back by \a j items.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+=(), operator-(), canReverseIterate()
*/
QSequentialIterable::const_iterator &QSequentialIterable::const_iterator::operator-=(int j)
{
    m_impl.advance(-j);
    return *this;
}

/*!
    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator+(int j) const
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Returns an iterator to the item at \a j positions backward from
    this iterator.

    If the container in the QVariant does not support bi-directional iteration, calling this function
    leads to undefined results.

    \sa operator+(), operator-=(), canReverseIterate()
*/
QSequentialIterable::const_iterator QSequentialIterable::const_iterator::operator-(int j) const
{
    QtMetaTypePrivate::QSequentialIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(-j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    \class QAssociativeIterable
    \since 5.2
    \inmodule QtCore
    \brief The QAssociativeIterable class is an iterable interface for an associative container in a QVariant.

    This class allows several methods of accessing the elements of an associative container held within
    a QVariant. An instance of QAssociativeIterable can be extracted from a QVariant if it can
    be converted to a QVariantHash or QVariantMap.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    The container itself is not copied before iterating over it.

    \sa QVariant
*/

/*!
    \internal
*/
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QAssociativeIterable::QAssociativeIterable(QtMetaTypePrivate::QAssociativeIterableImpl impl)
#else
QAssociativeIterable::QAssociativeIterable(const QtMetaTypePrivate::QAssociativeIterableImpl &impl)
#endif
  : m_impl(impl)
{
}

QAssociativeIterable::const_iterator::const_iterator(const QAssociativeIterable &iter, QAtomicInt *ref_)
  : m_impl(iter.m_impl), ref(ref_)
{
    ref->ref();
}

QAssociativeIterable::const_iterator::const_iterator(const QtMetaTypePrivate::QAssociativeIterableImpl &impl, QAtomicInt *ref_)
  : m_impl(impl), ref(ref_)
{
    ref->ref();
}

void QAssociativeIterable::const_iterator::begin()
{
    m_impl.begin();
}

void QAssociativeIterable::const_iterator::end()
{
    m_impl.end();
}

void QAssociativeIterable::const_iterator::find(const QVariant &key)
{
    Q_ASSERT(key.userType() == m_impl._metaType_id_key);
    const QtMetaTypePrivate::VariantData dkey(key.userType(), key.constData(), 0 /*key.flags()*/);
    m_impl.find(dkey);
}

/*!
    Returns a QAssociativeIterable::const_iterator for the beginning of the container. This
    can be used in stl-style iteration.

    \sa end()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::begin() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.begin();
    return it;
}

/*!
    Returns a QAssociativeIterable::const_iterator for the end of the container. This
    can be used in stl-style iteration.

    \sa begin()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::end() const
{
    const_iterator it(*this, new QAtomicInt(0));
    it.end();
    return it;
}

/*!
    \since 5.5

    Returns a QAssociativeIterable::const_iterator for the given key \a key
    in the container, if the types are convertible.

    If the key is not found, returns end().

    This can be used in stl-style iteration.

    \sa begin(), end(), value()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::find(const QVariant &key) const
{
    const_iterator it(*this, new QAtomicInt(0));
    QVariant key_ = key;
    if (key_.canConvert(m_impl._metaType_id_key) && key_.convert(m_impl._metaType_id_key))
        it.find(key_);
    else
        it.end();
    return it;
}

/*!
    Returns the value for the given \a key in the container, if the types are convertible.

    \sa find()
*/
QVariant QAssociativeIterable::value(const QVariant &key) const
{
    const const_iterator it = find(key);
    if (it == end())
        return QVariant();
    return *it;
}

/*!
    Returns the number of elements in the container.
*/
int QAssociativeIterable::size() const
{
    return m_impl.size();
}

/*!
    \class QAssociativeIterable::const_iterator
    \since 5.2
    \inmodule QtCore
    \brief The QAssociativeIterable::const_iterator allows iteration over a container in a QVariant.

    A QAssociativeIterable::const_iterator can only be created by a QAssociativeIterable instance,
    and can be used in a way similar to other stl-style iterators.

    \snippet code/src_corelib_kernel_qvariant.cpp 10

    \sa QAssociativeIterable
*/


/*!
    Destroys the QAssociativeIterable::const_iterator.
*/
QAssociativeIterable::const_iterator::~const_iterator()
{
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
}

/*!
    Creates a copy of \a other.
*/
QAssociativeIterable::const_iterator::const_iterator(const const_iterator &other)
  : m_impl(other.m_impl), ref(other.ref)
{
    ref->ref();
}

/*!
    Assigns \a other to this.
*/
QAssociativeIterable::const_iterator&
QAssociativeIterable::const_iterator::operator=(const const_iterator &other)
{
    other.ref->ref();
    if (!ref->deref()) {
        m_impl.destroyIter();
        delete ref;
    }
    m_impl = other.m_impl;
    ref = other.ref;
    return *this;
}

/*!
    Returns the current value, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::operator*() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentValue();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns the current key, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::key() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentKey();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns the current value, converted to a QVariant.
*/
const QVariant QAssociativeIterable::const_iterator::value() const
{
    const QtMetaTypePrivate::VariantData d = m_impl.getCurrentValue();
    QVariant v(d.metaTypeId, d.data, d.flags);
    if (d.metaTypeId == qMetaTypeId<QVariant>())
        return *reinterpret_cast<const QVariant*>(d.data);
    return v;
}

/*!
    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/
bool QAssociativeIterable::const_iterator::operator==(const const_iterator &other) const
{
    return m_impl.equal(other.m_impl);
}

/*!
    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/
bool QAssociativeIterable::const_iterator::operator!=(const const_iterator &other) const
{
    return !m_impl.equal(other.m_impl);
}

/*!
    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the container and returns an iterator to the new current
    item.

    Calling this function on QAssociativeIterable::end() leads to undefined results.

    \sa operator--()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator++()
{
    m_impl.advance(1);
    return *this;
}

/*!
    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the container and returns an iterator to the previously
    current item.
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator++(int)
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QAssociativeIterable::begin() leads to undefined results.

    \sa operator++()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator--()
{
    m_impl.advance(-1);
    return *this;
}

/*!
    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator--(int)
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    m_impl.advance(-1);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Advances the iterator by \a j items.

    \sa operator-=(), operator+()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator+=(int j)
{
    m_impl.advance(j);
    return *this;
}

/*!
    Makes the iterator go back by \a j items.

    \sa operator+=(), operator-()
*/
QAssociativeIterable::const_iterator &QAssociativeIterable::const_iterator::operator-=(int j)
{
    m_impl.advance(-j);
    return *this;
}

/*!
    Returns an iterator to the item at \a j positions forward from
    this iterator.

    \sa operator-(), operator+=()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator+(int j) const
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(j);
    return const_iterator(impl, new QAtomicInt(0));
}

/*!
    Returns an iterator to the item at \a j positions backward from
    this iterator.

    \sa operator+(), operator-=()
*/
QAssociativeIterable::const_iterator QAssociativeIterable::const_iterator::operator-(int j) const
{
    QtMetaTypePrivate::QAssociativeIterableImpl impl;
    impl.copy(m_impl);
    impl.advance(-j);
    return const_iterator(impl, new QAtomicInt(0));
}

QT_END_NAMESPACE
