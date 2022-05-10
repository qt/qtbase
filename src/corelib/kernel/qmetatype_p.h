// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMETATYPE_P_H
#define QMETATYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "qmetatype.h"

QT_BEGIN_NAMESPACE

#define QMETATYPE_CONVERTER(To, From, assign_and_return) \
    case makePair(QMetaType::To, QMetaType::From): \
        if (onlyCheck) \
            return true; \
        { \
            const From &source = *static_cast<const From *>(from); \
            To &result = *static_cast<To *>(to); \
            assign_and_return \
        }
#define QMETATYPE_CONVERTER_ASSIGN(To, From) \
    QMETATYPE_CONVERTER(To, From, result = To(source); return true;)

#define QMETATYPE_CONVERTER_FUNCTION(To, assign_and_return) \
        { \
            To &result = *static_cast<To *>(r); \
            assign_and_return \
        }

class QMetaTypeModuleHelper
{
    Q_DISABLE_COPY_MOVE(QMetaTypeModuleHelper)
protected:
    QMetaTypeModuleHelper() = default;
    ~QMetaTypeModuleHelper() = default;
public:
    static constexpr auto makePair(int from, int to) -> quint64
    {
        return (quint64(from) << 32) + quint64(to);
    }

    virtual const QtPrivate::QMetaTypeInterface *interfaceForType(int) const = 0;
    virtual bool convert(const void *, int, void *, int) const { return false; }
};

extern Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeGuiHelper;
extern Q_CORE_EXPORT const QMetaTypeModuleHelper *qMetaTypeWidgetsHelper;

namespace QtMetaTypePrivate {
template<typename T>
struct TypeDefinition
{
    static const bool IsAvailable = true;
};

// Ignore these types, as incomplete
#ifdef QT_BOOTSTRAPPED
template<> struct TypeDefinition<QBitArray> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QCborArray> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QCborMap> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QCborSimpleType> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QCborValue> { static const bool IsAvailable = false; };
#if QT_CONFIG(easingcurve)
template<> struct TypeDefinition<QEasingCurve> { static const bool IsAvailable = false; };
#endif
template<> struct TypeDefinition<QJsonArray> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonDocument> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonObject> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QJsonValue> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QUrl> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QByteArrayList> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_GEOM_VARIANT
template<> struct TypeDefinition<QRect> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QRectF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QSize> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QSizeF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QLine> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QLineF> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QPoint> { static const bool IsAvailable = false; };
template<> struct TypeDefinition<QPointF> { static const bool IsAvailable = false; };
#endif
#if !QT_CONFIG(regularexpression)
template<> struct TypeDefinition<QRegularExpression> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_CURSOR
template<> struct TypeDefinition<QCursor> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_MATRIX4X4
template<> struct TypeDefinition<QMatrix4x4> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR2D
template<> struct TypeDefinition<QVector2D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR3D
template<> struct TypeDefinition<QVector3D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_VECTOR4D
template<> struct TypeDefinition<QVector4D> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_QUATERNION
template<> struct TypeDefinition<QQuaternion> { static const bool IsAvailable = false; };
#endif
#ifdef QT_NO_ICON
template<> struct TypeDefinition<QIcon> { static const bool IsAvailable = false; };
#endif

template<typename T>
static const QT_PREPEND_NAMESPACE(QtPrivate::QMetaTypeInterface) *getInterfaceFromType()
{
    if constexpr (QtMetaTypePrivate::TypeDefinition<T>::IsAvailable) {
        return &QT_PREPEND_NAMESPACE(QtPrivate::QMetaTypeInterfaceWrapper)<T>::metaType;
    }
    return nullptr;
}

#define QT_METATYPE_CONVERT_ID_TO_TYPE(MetaTypeName, MetaTypeId, RealName)                         \
    case QMetaType::MetaTypeName:                                                                  \
        return QtMetaTypePrivate::getInterfaceFromType<RealName>();

const char *typedefNameForType(const QtPrivate::QMetaTypeInterface *type_d);
} //namespace QtMetaTypePrivate

QT_END_NAMESPACE

#endif // QMETATYPE_P_H
