/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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



#include <jni.h>
#include <android/log.h>
#include <extract.h>
#include <alloca.h>
#include <stdlib.h>

#define LOG_TAG    "extractSyleInfo"
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// The following part was shamelessly stolen from ResourceTypes.cpp from Android's sources
/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

static void deserializeInternal(const void* inData, Res_png_9patch* outData) {
    char* patch = (char*) inData;
    if (inData != outData) {
        memmove(&outData->wasDeserialized, patch, 4);     // copy  wasDeserialized, numXDivs, numYDivs, numColors
        memmove(&outData->paddingLeft, patch + 12, 4);     // copy  wasDeserialized, numXDivs, numYDivs, numColors
    }
    outData->wasDeserialized = true;
    char* data = (char*)outData;
    data +=  sizeof(Res_png_9patch);
    outData->xDivs = (int32_t*) data;
    data +=  outData->numXDivs * sizeof(int32_t);
    outData->yDivs = (int32_t*) data;
    data +=  outData->numYDivs * sizeof(int32_t);
    outData->colors = (uint32_t*) data;
}

Res_png_9patch* Res_png_9patch::deserialize(const void* inData)
{
    if (sizeof(void*) != sizeof(int32_t)) {
        LOGE("Cannot deserialize on non 32-bit system\n");
        return NULL;
    }
    deserializeInternal(inData, (Res_png_9patch*) inData);
    return (Res_png_9patch*) inData;
}

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractNativeChunkInfo20(JNIEnv * env, jobject, long addr)
{
    Res_png_9patch20* chunk = reinterpret_cast<Res_png_9patch20*>(addr);
    Res_png_9patch20::deserialize(chunk);
    //printChunkInformation(chunk);
    jintArray result;
    size_t size = 3+chunk->numXDivs+chunk->numYDivs+chunk->numColors;
    result = env->NewIntArray(size);
    if (!result)
        return 0;

    jint *data = (jint*)malloc(sizeof(jint)*size);
    size_t pos = 0;
    data[pos++] = chunk->numXDivs;
    data[pos++] = chunk->numYDivs;
    data[pos++] = chunk->numColors;

    int32_t* xDivs = chunk->getXDivs();
    int32_t* yDivs = chunk->getYDivs();
    uint32_t* colors = chunk->getColors();

    for (int x = 0; x <chunk->numXDivs; x ++)
        data[pos++]=xDivs[x];
    for (int y = 0; y <chunk->numYDivs; y ++)
        data[pos++] = yDivs[y];
    for (int c = 0; c <chunk->numColors; c ++)
        data[pos++] = colors[c];
    env->SetIntArrayRegion(result, 0, size, data);
    free(data);
    return result;
}

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractChunkInfo20(JNIEnv * env, jobject  obj, jbyteArray chunkObj)
{
    size_t chunkSize = env->GetArrayLength(chunkObj);
    void* storage = alloca(chunkSize);
    env->GetByteArrayRegion(chunkObj, 0, chunkSize,
                            reinterpret_cast<jbyte*>(storage));

    if (!env->ExceptionCheck())
        return Java_org_qtproject_qt5_android_ExtractStyle_extractNativeChunkInfo20(env, obj, long(storage));
    else
        env->ExceptionClear();
    return 0;
}

static inline void fill9patchOffsets(Res_png_9patch20* patch) {
    patch->xDivsOffset = sizeof(Res_png_9patch20);
    patch->yDivsOffset = patch->xDivsOffset + (patch->numXDivs * sizeof(int32_t));
    patch->colorsOffset = patch->yDivsOffset + (patch->numYDivs * sizeof(int32_t));
}

Res_png_9patch20* Res_png_9patch20::deserialize(void* inData)
{
    Res_png_9patch20* patch = reinterpret_cast<Res_png_9patch20*>(inData);
    patch->wasDeserialized = true;
    fill9patchOffsets(patch);
    return patch;
}
