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


#include "qplatformdefs.h"
#include "qthreadonce.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"

Q_GLOBAL_STATIC_WITH_ARGS(QMutex, onceInitializationMutex, (QMutex::Recursive))

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

#endif // QT_NO_THREAD
