/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QTHREADONCE_H
#define QTHREADONCE_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

QT_BEGIN_HEADER

QT_MODULE(Core)

#ifndef QT_NO_THREAD

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

#endif // QT_NO_THREAD

QT_END_HEADER

#endif
