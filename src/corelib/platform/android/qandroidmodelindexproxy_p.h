// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDMODELINDEXPROXY_P_H
#define QANDROIDMODELINDEXPROXY_P_H

#include <QtCore/private/qandroidtypes_p.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>

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

QT_BEGIN_NAMESPACE

using namespace QtJniTypes;

class QAndroidItemModelProxy;

class Q_CORE_EXPORT QAndroidModelIndexProxy
{
public:
    static JQtModelIndex jInstance(QModelIndex modelIndex);
    static QModelIndex qInstance(JQtModelIndex jModelIndex);

    static jobject data(JNIEnv *env, jobject object, int role);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(data)

    static jlong internalId(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(internalId)

    static jboolean isValid(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(isValid)

    static JQtModelIndex parent(JNIEnv *env, jobject object);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(parent)

    static bool registerNatives(QJniEnvironment &env);
};

QT_END_NAMESPACE

#endif // QANDROIDMODELINDEXPROXY_P_H
