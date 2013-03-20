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

#ifndef QJNIOBJECT_H
#define QJNIOBJECT_H

#include <qglobal.h>
#include <jni.h>

QT_BEGIN_NAMESPACE

template <typename T>
class QJNILocalRef;

/**
 * Allows to wrap any Java class and partially hide some of the jni calls.
 *
 * Usage example:
 *
 *    QJNIObject javaString("java/lang/String");
 *    jchar char = javaString.callMethod<jchar>("charAt", "(I)C", 0);
 *
 *    ----
 *
 *    jstring string = QJNIObject::callStaticMethod<jstring>("java/lang/String",
 *                                                           "valueOf",
 *                                                           "(I)Ljava/lang/String;", 2);
 *
 *    ----
 *
 *    // Constructor with argument
 *    jstring someString;
 *    QJNIObject someObject("java/some/Class", "(Ljava/lang/String;)V", someString);
 *    someObject.setField<jint>("fieldName", 10);
 *    someObject.callMethod<void>("doStuff");
 */
class QJNIObject
{
public:
    QJNIObject(const char *className);
    QJNIObject(const char *className, const char *sig, ...);
    QJNIObject(jclass clazz);
    QJNIObject(jclass clazz, const char *sig, ...);
    QJNIObject(jobject obj);
    virtual ~QJNIObject();

    static bool isClassAvailable(const char *className);

    bool isValid() const { return m_jobject != 0; }
    jobject object() const { return m_jobject; }

    template <typename T>
    T callMethod(const char *methodName);
    template <typename T>
    T callMethod(const char *methodName, const char *sig, ...);
    template <typename T>
    QJNILocalRef<T> callObjectMethod(const char *methodName);
    template <typename T>
    QJNILocalRef<T> callObjectMethod(const char *methodName, const char *sig, ...);

    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName);
    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName, const char *sig, ...);
    template <typename T>
    static QJNILocalRef<T> callStaticObjectMethod(const char *className, const char *methodName);
    template <typename T>
    static QJNILocalRef<T> callStaticObjectMethod(const char *className,
                                               const char *methodName,
                                               const char *sig, ...);
    template <typename T>
    static T callStaticMethod(jclass clazz, const char *methodName);
    template <typename T>
    static T callStaticMethod(jclass clazz, const char *methodName, const char *sig, ...);
    template <typename T>
    static QJNILocalRef<T> callStaticObjectMethod(jclass clazz, const char *methodName);
    template <typename T>
    static QJNILocalRef<T> callStaticObjectMethod(jclass clazz,
                                               const char *methodName,
                                               const char *sig, ...);

    template <typename T>
    T getField(const char *fieldName);
    template <typename T>
    T getField(const char *fieldName, const char *sig);
    template <typename T>
    QJNILocalRef<T> getObjectField(const char *fieldName);
    template <typename T>
    QJNILocalRef<T> getObjectField(const char *fieldName, const char *sig);

    template <typename T>
    void setField(const char *fieldName, T value);
    template <typename T>
    void setField(const char *fieldName, const char *sig, T value);

    template <typename T>
    static QJNILocalRef<T> getStaticObjectField(const char *className, const char *fieldName);
    template <typename T>
    static QJNILocalRef<T> getStaticObjectField(const char *className,
                                             const char *fieldName,
                                             const char *sig);
    template <typename T>
    static T getStaticField(const char *className, const char *fieldName);
    template <typename T>
    static QJNILocalRef<T> getStaticObjectField(jclass clazz, const char *fieldName);
    template <typename T>
    static QJNILocalRef<T> getStaticObjectField(jclass clazz, const char *fieldName, const char *sig);
    template <typename T>
    static T getStaticField(jclass clazz, const char *fieldName);

    template <typename T>
    static void setStaticField(const char *className,
                               const char *fieldName,
                               const char *sig,
                               T value);
    template <typename T>
    static void setStaticField(const char *className, const char *fieldName, T value);
    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName, const char *sig, T value);
    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName, T value);

protected:
    jobject m_jobject;
    jclass m_jclass;
    bool m_own_jclass;
};

QT_END_NAMESPACE

#endif // QJNIOBJECT_H
