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

namespace QModulesPrivate {
enum Names { Core, Gui, Widgets, Unknown, ModulesCount /* ModulesCount has to be at the end */ };

static inline int moduleForType(const uint typeId)
{
    if (typeId <= QMetaType::LastCoreType)
        return Core;
    if (typeId >= QMetaType::FirstGuiType && typeId <= QMetaType::LastGuiType)
        return Gui;
    if (typeId >= QMetaType::FirstWidgetsType && typeId <= QMetaType::LastWidgetsType)
        return Widgets;
    return Unknown;
}

template <typename T>
class QTypeModuleInfo
{
public:
    enum Module : bool {
        IsCore = false,
        IsWidget = false,
        IsGui = false,
        IsUnknown = true
    };
};

#define QT_ASSIGN_TYPE_TO_MODULE(TYPE, MODULE) \
template<> \
class QTypeModuleInfo<TYPE > \
{ \
public: \
    enum Module : bool { \
        IsCore = (((MODULE) == (QModulesPrivate::Core))), \
        IsWidget = (((MODULE) == (QModulesPrivate::Widgets))), \
        IsGui = (((MODULE) == (QModulesPrivate::Gui))), \
        IsUnknown = !(IsCore || IsWidget || IsGui) \
    }; \
    static inline int module() { return MODULE; } \
    static_assert((IsUnknown && !(IsCore || IsWidget || IsGui)) \
                 || (IsCore && !(IsUnknown || IsWidget || IsGui)) \
                 || (IsWidget && !(IsUnknown || IsCore || IsGui)) \
                 || (IsGui && !(IsUnknown || IsCore || IsWidget))); \
};


#define QT_DECLARE_CORE_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Core);
#define QT_DECLARE_GUI_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Gui);
#define QT_DECLARE_WIDGETS_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, QModulesPrivate::Widgets);

QT_WARNING_PUSH
#if defined(Q_CC_CLANG) && Q_CC_CLANG >= 900
QT_WARNING_DISABLE_CLANG("-Wconstant-logical-operand")
#endif

QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_CORE_CLASS(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_CORE_POINTER(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_GUI_CLASS(QT_DECLARE_GUI_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_DECLARE_WIDGETS_MODULE_TYPES_ITER)

QT_WARNING_POP
} // namespace QModulesPrivate

#undef QT_DECLARE_CORE_MODULE_TYPES_ITER
#undef QT_DECLARE_GUI_MODULE_TYPES_ITER
#undef QT_DECLARE_WIDGETS_MODULE_TYPES_ITER

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
public:
    static constexpr auto makePair(int from, int to) -> quint64
    {
        return (quint64(from) << 32) + quint64(to);
    };

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

} //namespace QtMetaTypePrivate

QT_END_NAMESPACE

#endif // QMETATYPE_P_H
