// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDSURFACECLIENT_H
#define ANDROIDSURFACECLIENT_H
#include <QMutex>
#include <jni.h>

QT_BEGIN_NAMESPACE

class AndroidSurfaceClient
{
public:
    virtual void surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h) = 0;
    void lockSurface() { m_surfaceMutex.lock(); }
    void unlockSurface() { m_surfaceMutex.unlock(); }

protected:
    QMutex m_surfaceMutex;
};

QT_END_NAMESPACE

#endif // ANDROIDSURFACECLIENT_H
