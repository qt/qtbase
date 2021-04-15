/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QJNI_ENVIRONMENT_H
#define QJNI_ENVIRONMENT_H

#include <QtCore/QScopedPointer>

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#include <jni.h>

QT_BEGIN_NAMESPACE

class QJniEnvironmentPrivate;

class Q_CORE_EXPORT QJniEnvironment
{
public:
    QJniEnvironment();
    ~QJniEnvironment();
    JNIEnv *operator->() const;
    JNIEnv &operator*() const;
    JNIEnv *jniEnv() const;
    jclass findClass(const char *className);
    static JavaVM *javaVM();
    bool registerNativeMethods(const char *className, JNINativeMethod methods[], int size);

    enum class OutputMode {
        Silent,
        Verbose
    };

    bool checkAndClearExceptions(OutputMode outputMode = OutputMode::Verbose);
    static bool checkAndClearExceptions(JNIEnv *env, OutputMode outputMode = OutputMode::Verbose);

private:
    Q_DISABLE_COPY_MOVE(QJniEnvironment)
    QScopedPointer<QJniEnvironmentPrivate> d;
};

QT_END_NAMESPACE

#endif

#endif // QJNI_ENVIRONMENT_H
