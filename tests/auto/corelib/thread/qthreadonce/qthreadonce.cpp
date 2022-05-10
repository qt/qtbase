// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include "qplatformdefs.h"
#include "qthreadonce.h"

#include "qmutex.h"

Q_GLOBAL_STATIC(QRecursiveMutex, onceInitializationMutex)

enum QOnceExtra {
    MustRunCode = 0x01,
    MustUnlockMutex = 0x02
};

/*!
    \internal
    Initialize the Q_ONCE structure.

    Q_ONCE consists of two variables:
     - a static POD QOnceControl::ControlVariable (it's a QBasicAtomicInt)
     - an automatic QOnceControl that controls the former

    The POD is initialized to 0.

    When QOnceControl's constructor starts, it'll lock the global
    initialization mutex. It'll then check if it's the first to up
    the control variable and will take note.

    The QOnceControl's destructor will unlock the global
    initialization mutex.
*/
QOnceControl::QOnceControl(QBasicAtomicInt *control)
{
    d = 0;
    gv = control;
    // check if code has already run once
    if (gv->loadAcquire() == 2) {
        // uncontended case: it has already initialized
        // no waiting
        return;
    }

    // acquire the path
    onceInitializationMutex()->lock();
    extra = MustUnlockMutex;

    if (gv->testAndSetAcquire(0, 1)) {
        // path acquired, we're the first
        extra |= MustRunCode;
    }
}

QOnceControl::~QOnceControl()
{
    if (mustRunCode())
        // code wasn't run!
        gv->testAndSetRelease(1, 0);
    else
        gv->testAndSetRelease(1, 2);
    if (extra & MustUnlockMutex)
        onceInitializationMutex()->unlock();
}

/*!
    \internal
    Returns true if the initialization code must be run.

    Obviously, the initialization code must be run only once...
*/
bool QOnceControl::mustRunCode()
{
    return extra & MustRunCode;
}

void QOnceControl::done()
{
    extra &= ~MustRunCode;
}
