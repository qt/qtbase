/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
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

#include "qjnihelpers_p.h"

#include <qpa/qplatformnativeinterface.h>
#include <qguiapplication.h>

QT_BEGIN_NAMESPACE

QString qt_convertJString(jstring string)
{
    QAttachedJNIEnv env;
    int strLength = env->GetStringLength(string);
    QString res;
    res.resize(strLength);
    env->GetStringRegion(string, 0, strLength, (jchar*)res.utf16());
    return res;
}

QJNILocalRef<jstring> qt_toJString(const QString &string)
{
    QAttachedJNIEnv env;
    return QJNILocalRef<jstring>(env->NewString(reinterpret_cast<const jchar*>(string.constData()),
                                                string.length()));
}


static JavaVM *g_javaVM = 0;

static JavaVM *getJavaVM()
{
    if (!g_javaVM){
        QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
        g_javaVM = static_cast<JavaVM*>(nativeInterface->nativeResourceForIntegration("JavaVM"));
    }
    return g_javaVM;
}

QThreadStorage<int> QAttachedJNIEnv::m_refCount;

QAttachedJNIEnv::QAttachedJNIEnv()
{
    JavaVM *vm = javaVM();
    if (vm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&jniEnv, 0) < 0) {
            jniEnv = 0;
            return;
        }
    }

    if (!m_refCount.hasLocalData())
        m_refCount.setLocalData(1);
    else
        m_refCount.setLocalData(m_refCount.localData() + 1);
}

QAttachedJNIEnv::~QAttachedJNIEnv()
{
    if (!jniEnv)
        return;

    int newRef = m_refCount.localData() - 1;
    m_refCount.setLocalData(newRef);

    if (newRef == 0)
        javaVM()->DetachCurrentThread();

    jniEnv = 0;
}

JavaVM *QAttachedJNIEnv::javaVM()
{
    return getJavaVM();
}

QT_END_NAMESPACE
