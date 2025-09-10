// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDTYPECONVERTER_P_H
#define QANDROIDTYPECONVERTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/private/qandroidtypes_p.h>
#include <QtCore/private/qandroiditemmodelproxy_p.h>

#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

namespace QAndroidTypeConverter
{
    [[maybe_unused]] static QVariant toQVariant(const QJniObject &object)
    {
        using namespace QtJniTypes;
        if (!object.isValid())
            return QVariant{};
        const QByteArray classname(object.className());

        if (classname == Traits<String>::className())
            return object.toString();
        else if (classname == Traits<Integer>::className())
            return object.callMethod<jint>("intValue");
        else if (classname == Traits<Long>::className())
            return QVariant::fromValue<long>(object.callMethod<jlong>("longValue"));
        else if (classname == Traits<Double>::className())
            return object.callMethod<jdouble>("doubleValue");
        else if (classname == Traits<Float>::className())
            return object.callMethod<jfloat>("floatValue");
        else if (classname == Traits<Boolean>::className())
            return QVariant::fromValue<bool>(object.callMethod<jboolean>("booleanValue"));
        else {
            QJniEnvironment env;
            const jclass className = env.findClass(Traits<JQtAbstractItemModel>::className());
            if (env->IsInstanceOf(object.object(), className))
                return QVariant::fromValue(QAndroidItemModelProxy::createNativeProxy(object));
        }

        return QVariant{};
    }

    [[maybe_unused]] Q_REQUIRED_RESULT static jobject toJavaObject(const QVariant &var, JNIEnv *env)
    {
        Q_ASSERT(env);
        switch (var.typeId()) {
        case QMetaType::Type::Int:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Integer>(
                                            get<int>(var))
                                            .object());
        case QMetaType::Type::Long:
        case QMetaType::Type::LongLong:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Long>(
                                            get<jlong>(var))
                                            .object());
        case QMetaType::Type::Double:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Double>(
                                            get<double>(var))
                                            .object());
        case QMetaType::Type::Float:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Float>(
                                            get<float>(var))
                                            .object());
        case QMetaType::Type::Bool:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Boolean>(
                                            get<bool>(var))
                                            .object());
        case QMetaType::Type::VoidStar:
            return env->NewLocalRef(QJniObject::construct<QtJniTypes::Void>().object());
        case QMetaType::Type::QString:
            return env->NewLocalRef(
                    QJniObject::fromString(get<QString>(var)).object());
        default:
            if (var.canConvert<QAbstractItemModel *>()) {
                return env->NewLocalRef(
                        QAndroidItemModelProxy::createProxy(var.value<QAbstractItemModel *>())
                                .object());
            } else
                return nullptr;
        }
        return nullptr;
    }
};

QT_END_NAMESPACE

#endif // QANDROIDTYPECONVERTER_P_H
