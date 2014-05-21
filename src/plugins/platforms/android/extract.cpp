/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/



#include <jni.h>
#include <android/log.h>
#include <extract.h>
#include <alloca.h>

#define LOG_TAG    "extractSyleInfo"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractNativeChunkInfo(JNIEnv * env, jobject, Res_png_9patch* chunk)
{
        Res_png_9patch::deserialize(chunk);
        //printChunkInformation(chunk);
        jintArray result;
        size_t size = 3+chunk->numXDivs+chunk->numYDivs+chunk->numColors;
        result = env->NewIntArray(size);
        if (!result)
            return 0;

        jint *data = (jint*)malloc(sizeof(jint)*size);
        size_t pos = 0;
        data[pos++]=chunk->numXDivs;
        data[pos++]=chunk->numYDivs;
        data[pos++]=chunk->numColors;
        for (int x = 0; x <chunk->numXDivs; x ++)
            data[pos++]=chunk->xDivs[x];
        for (int y = 0; y <chunk->numYDivs; y ++)
            data[pos++]=chunk->yDivs[y];
        for (int c = 0; c <chunk->numColors; c ++)
            data[pos++]=chunk->colors[c];
        env->SetIntArrayRegion(result, 0, size, data);
        free(data);
        return result;
}

extern "C" JNIEXPORT jintArray JNICALL Java_org_qtproject_qt5_android_ExtractStyle_extractChunkInfo(JNIEnv * env, jobject  obj, jbyteArray chunkObj)
{
        size_t chunkSize = env->GetArrayLength(chunkObj);
        void* storage = alloca(chunkSize);
        env->GetByteArrayRegion(chunkObj, 0, chunkSize,
                                reinterpret_cast<jbyte*>(storage));

        if (!env->ExceptionCheck())
            return Java_org_qtproject_qt5_android_ExtractStyle_extractNativeChunkInfo(env, obj, static_cast<Res_png_9patch*>(storage));
        else
            env->ExceptionClear();
        return 0;
}

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
