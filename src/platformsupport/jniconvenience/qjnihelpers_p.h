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

#ifndef QJNIHELPERS_H
#define QJNIHELPERS_H

#include <jni.h>
#include <qglobal.h>
#include <QString>
#include <QThreadStorage>

QT_BEGIN_NAMESPACE

template <typename T>
class QJNILocalRef;

QString qt_convertJString(jstring string);
QJNILocalRef<jstring> qt_toJString(const QString &string);


struct QAttachedJNIEnv
{
    QAttachedJNIEnv();
    ~QAttachedJNIEnv();

    static JavaVM *javaVM();

    JNIEnv *operator->()
    {
        return jniEnv;
    }

    operator JNIEnv*() const
    {
        return jniEnv;
    }

    JNIEnv *jniEnv;

private:
    static QThreadStorage<int> m_refCount;
};


template <typename T>
class QJNILocalRef
{
public:
    inline QJNILocalRef() : m_obj(0) { }
    inline explicit QJNILocalRef(T o) : m_obj(o) { }
    inline QJNILocalRef(const QJNILocalRef<T> &other) : m_obj(other.m_obj)
    {
        if (other.m_obj)
            m_obj = static_cast<T>(m_env->NewLocalRef(other.m_obj));
    }

    template <typename X>
    inline QJNILocalRef(const QJNILocalRef<X> &other) : m_obj(other.m_obj)
    {
        if (other.m_obj)
            m_obj = static_cast<T>(m_env->NewLocalRef(other.m_obj));
    }

    inline ~QJNILocalRef() { release(); }

    inline QJNILocalRef<T> &operator=(const QJNILocalRef<T> &other)
    {
        release();
        m_obj = other.m_obj; // for type checking
        if (other.m_obj)
            m_obj = static_cast<T>(m_env->NewLocalRef(other.m_obj));
        return *this;
    }

    template <typename X>
    inline QJNILocalRef<T> &operator=(const QJNILocalRef<X> &other)
    {
        release();
        m_obj = other.m_obj; // for type checking
        if (other.m_obj)
            m_obj = static_cast<T>(m_env->NewLocalRef(other.m_obj));
        return *this;
    }

    inline QJNILocalRef<T> &operator=(T o)
    {
        release();
        m_obj = o;
        return *this;
    }

    template <typename X>
    inline QJNILocalRef<T> &operator=(X o)
    {
        release();
        m_obj = o;
        return *this;
    }

    inline bool operator !() const { return !m_obj; }
    inline bool isNull() const { return !m_obj; }
    inline T object() const { return m_obj; }

private:
    void release()
    {
        if (m_obj) {
            m_env->DeleteLocalRef(m_obj);
            m_obj = 0;
        }
    }

    QAttachedJNIEnv m_env;
    T m_obj;

    template <class X> friend class QJNILocalRef;
};

template <class T, class X>
bool operator==(const QJNILocalRef<T> &ptr1, const QJNILocalRef<X> &ptr2)
{
    return ptr1.m_obj == ptr2.m_obj;
}
template <class T, class X>
bool operator!=(const QJNILocalRef<T> &ptr1, const QJNILocalRef<X> &ptr2)
{
    return ptr1.m_obj != ptr2.m_obj;
}

template <class T, class X>
bool operator==(const QJNILocalRef<T> &ptr1, X ptr2)
{
    return ptr1.m_obj == ptr2;
}
template <class T, class X>
bool operator==(T ptr1, const QJNILocalRef<X> &ptr2)
{
    return ptr1 == ptr2.m_obj;
}
template <class T, class X>
bool operator!=(const QJNILocalRef<T> &ptr1, X ptr2)
{
    return !(ptr1 == ptr2);
}
template <class T, class X>
bool operator!=(const T *ptr1, const QJNILocalRef<X> &ptr2)
{
    return !(ptr2 == ptr1);
}

QT_END_NAMESPACE

#endif // QJNIHELPERS_H
