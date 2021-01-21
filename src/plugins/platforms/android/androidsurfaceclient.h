/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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
