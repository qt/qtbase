/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QTHREADONCE_H
#define QTHREADONCE_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>


class QOnceControl
{
public:
    QOnceControl(QBasicAtomicInt *);
    ~QOnceControl();

    bool mustRunCode();
    void done();

private:
    QBasicAtomicInt *gv;
    union {
        qint32 extra;
        void *d;
    };
};

#define Q_ONCE_GV_NAME2(prefix, line)   prefix ## line
#define Q_ONCE_GV_NAME(prefix, line)    Q_ONCE_GV_NAME2(prefix, line)
#define Q_ONCE_GV                       Q_ONCE_GV_NAME(_q_once_, __LINE__)

#define Q_ONCE \
    static QBasicAtomicInt Q_ONCE_GV = Q_BASIC_ATOMIC_INITIALIZER(0);   \
    if (0){} else                                                       \
        for (QOnceControl _control_(&Q_ONCE_GV); _control_.mustRunCode(); _control_.done())

template<typename T>
class QSingleton
{
    // this is a POD-like class
    struct Destructor
    {
        T *&pointer;
        Destructor(T *&ptr) : pointer(ptr) {}
        ~Destructor() { delete pointer; }
    };

public:
    T *_q_value;
    QBasicAtomicInt _q_guard;

    inline T *value()
    {
        for (QOnceControl control(&_q_guard); control.mustRunCode(); control.done()) {
            _q_value = new T();
            static Destructor cleanup(_q_value);
        }
        return _q_value;
    }

    inline T& operator*() { return *value(); }
    inline T* operator->() { return value(); }
    inline operator T*() { return value(); }
};

#endif
