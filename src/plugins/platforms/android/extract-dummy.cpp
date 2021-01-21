/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include <jni.h>
#include <extract.h>

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractNativeChunkInfo20(JNIEnv *, jobject, long)
{
    return 0;
}

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractChunkInfo20(JNIEnv *, jobject, jbyteArray)
{
    return 0;
}
