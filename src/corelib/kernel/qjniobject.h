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

#ifndef QJNIOBJECT_H
#define QJNIOBJECT_H

#include <QtCore/QSharedPointer>

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#include <jni.h>
#else
class jclass;
class jobject;
#endif

QT_BEGIN_NAMESPACE

class QJniObjectPrivate;

class Q_CORE_EXPORT QJniObject
{
public:
    QJniObject();
    explicit QJniObject(const char *className);
    explicit QJniObject(const char *className, const char *signature, ...);
    explicit QJniObject(jclass clazz);
    explicit QJniObject(jclass clazz, const char *signature, ...);
    QJniObject(jobject globalRef);
    ~QJniObject();

    template <typename T>
    T object() const;
    jobject object() const;

    template <typename T>
    T callMethod(const char *methodName, const char *signature, ...) const;
    template <typename T>
    T callMethod(const char *methodName) const;
    template <typename T>
    QJniObject callObjectMethod(const char *methodName) const;
    QJniObject callObjectMethod(const char *methodName, const char *signature, ...) const;

    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName,
                              const char *signature, ...);
    template <typename T>
    static T callStaticMethod(const char *className, const char *methodName);
    template <typename T>
    static T callStaticMethod(jclass clazz, const char *methodName, const char *signature, ...);
    template <typename T>
    static T callStaticMethod(jclass clazz, const char *methodName);

    template <typename T>
    static QJniObject callStaticObjectMethod(const char *className, const char *methodName);
    static QJniObject callStaticObjectMethod(const char *className,
                                             const char *methodName,
                                             const char *signature, ...);

    template <typename T>
    static QJniObject callStaticObjectMethod(jclass clazz, const char *methodName);
    static QJniObject callStaticObjectMethod(jclass clazz,
                                             const char *methodName,
                                             const char *signature, ...);

    template <typename T>
    T getField(const char *fieldName) const;

    template <typename T>
    static T getStaticField(const char *className, const char *fieldName);
    template <typename T>
    static T getStaticField(jclass clazz, const char *fieldName);

    template <typename T>
    QJniObject getObjectField(const char *fieldName) const;
    QJniObject getObjectField(const char *fieldName, const char *signature) const;

    template <typename T>
    static QJniObject getStaticObjectField(const char *className, const char *fieldName);
    static QJniObject getStaticObjectField(const char *className,
                                           const char *fieldName,
                                           const char *signature);
    template <typename T>
    static QJniObject getStaticObjectField(const char *className,
                                           const char *fieldName,
                                           const char *signature);

    template <typename T>
    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName);
    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName,
                                           const char *signature);
    template <typename T>
    static QJniObject getStaticObjectField(jclass clazz, const char *fieldName,
                                           const char *signature);

    template <typename T>
    void setField(const char *fieldName, T value);
    template <typename T>
    void setField(const char *fieldName, const char *signature, T value);
    template <typename T>
    static void setStaticField(const char *className, const char *fieldName, T value);
    template <typename T>
    static void setStaticField(const char *className, const char *fieldName,
                               const char *signature, T value);
    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName,
                               const char *signature, T value);

    template <typename T>
    static void setStaticField(jclass clazz, const char *fieldName, T value);

    static QJniObject fromString(const QString &string);
    QString toString() const;

    static bool isClassAvailable(const char *className);
    bool isValid() const;

    // This function takes ownership of the jobject and releases the local ref. before returning.
    static QJniObject fromLocalRef(jobject lref);

    template <typename T> QJniObject &operator=(T obj);

private:
    struct QVaListPrivate { operator va_list &() const { return m_args; } va_list &m_args; };

    QJniObject(const char *className, const char *signature, const QVaListPrivate &args);
    QJniObject(jclass clazz, const char *signature, const QVaListPrivate &args);

    template <typename T>
    T callMethodV(const char *methodName, const char *signature, va_list args) const;
    QJniObject callObjectMethodV(const char *methodName,
                                 const char *signature,
                                 va_list args) const;
    template <typename T>
    static T callStaticMethodV(const char *className,
                               const char *methodName,
                               const char *signature,
                               va_list args);
    template <typename T>
    static T callStaticMethodV(jclass clazz,
                               const char *methodName,
                               const char *signature,
                               va_list args);

    static QJniObject callStaticObjectMethodV(const char *className,
                                                     const char *methodName,
                                                     const char *signature,
                                                     va_list args);

    static QJniObject callStaticObjectMethodV(jclass clazz,
                                                     const char *methodName,
                                                     const char *signature,
                                                     va_list args);

    bool isSameObject(jobject obj) const;
    bool isSameObject(const QJniObject &other) const;
    void assign(jobject obj);
    jobject javaObject() const;

    friend bool operator==(const QJniObject &, const QJniObject &);
    friend bool operator!=(const QJniObject&, const QJniObject&);

    QSharedPointer<QJniObjectPrivate> d;
};

inline bool operator==(const QJniObject &obj1, const QJniObject &obj2)
{
    return obj1.isSameObject(obj2);
}

inline bool operator!=(const QJniObject &obj1, const QJniObject &obj2)
{
    return !obj1.isSameObject(obj2);
}

QT_END_NAMESPACE

#endif // QJNIOBJECT_H
