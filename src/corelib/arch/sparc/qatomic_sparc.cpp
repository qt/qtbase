/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtCore/qatomic.h>

#include <limits.h>
#include <sched.h>

extern "C" {

int q_atomic_trylock_int(volatile int *addr);
int q_atomic_trylock_ptr(volatile void *addr);

Q_CORE_EXPORT int q_atomic_lock_int(volatile int *addr)
{
    int returnValue = q_atomic_trylock_int(addr);

    if (returnValue == INT_MIN) {
        do {
            // spin until we think we can succeed
            do {
                sched_yield();
                returnValue = *addr;
            } while (returnValue == INT_MIN);

            // try again
            returnValue = q_atomic_trylock_int(addr);
        } while (returnValue == INT_MIN);
    }

    return returnValue;
}

Q_CORE_EXPORT int q_atomic_lock_ptr(volatile void *addr)
{
    int returnValue = q_atomic_trylock_ptr(addr);

    if (returnValue == -1) {
        do {
            // spin until we think we can succeed
            do {
                sched_yield();
                returnValue = *reinterpret_cast<volatile int *>(addr);
            } while (returnValue == -1);

            // try again
            returnValue = q_atomic_trylock_ptr(addr);
        } while (returnValue == -1);
    }

    return returnValue;
}

} // extern "C"
