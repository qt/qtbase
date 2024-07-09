// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDTYPES_P_H
#define QANDROIDTYPES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qjniobject.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

#ifndef QT_DECLARE_JNI_CLASS_STANDARD_TYPES
Q_DECLARE_JNI_CLASS(Void, "java/lang/Void");
Q_DECLARE_JNI_CLASS(Integer, "java/lang/Integer");
Q_DECLARE_JNI_CLASS(Long, "java/lang/Long");
Q_DECLARE_JNI_CLASS(Double, "java/lang/Double");
Q_DECLARE_JNI_CLASS(Float, "java/lang/Float");
Q_DECLARE_JNI_CLASS(Boolean, "java/lang/Boolean");
Q_DECLARE_JNI_CLASS(String, "java/lang/String");
Q_DECLARE_JNI_CLASS(Class, "java/lang/Class");

Q_DECLARE_JNI_CLASS(HashMap, "java/util/HashMap")
Q_DECLARE_JNI_CLASS(Set, "java/util/Set")
#endif

Q_DECLARE_JNI_CLASS(JQtAbstractItemModel, "org/qtproject/qt/android/QtAbstractItemModel")
Q_DECLARE_JNI_CLASS(JQtAndroidItemModelProxy, "org/qtproject/qt/android/QtAndroidItemModelProxy")
Q_DECLARE_JNI_CLASS(JQtModelIndex, "org/qtproject/qt/android/QtModelIndex")

QT_END_NAMESPACE

#endif // QANDROIDTYPES_P_H
