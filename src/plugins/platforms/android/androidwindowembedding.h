// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTANDROIDWINDOWEMBEDDING_H
#define QTANDROIDWINDOWEMBEDDING_H

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

#include <QtCore/qjnienvironment.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(View, "android/view/View");

namespace QtAndroidWindowEmbedding
{
    bool registerNatives(QJniEnvironment& env);
    void createRootWindow(JNIEnv *, jclass, QtJniTypes::View rootView,
                          jint x, jint y,jint width, jint height);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(createRootWindow)
    void deleteWindow(JNIEnv *, jclass, jlong window);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(deleteWindow)
    void setWindowVisible(JNIEnv *, jclass, jlong window, jboolean visible);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(setWindowVisible)
    void resizeWindow(JNIEnv *, jclass, jlong windowRef, jint x, jint y, jint width, jint height);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(resizeWindow)
};

QT_END_NAMESPACE

#endif // QTANDROIDWINDOWEMBEDDING_H
