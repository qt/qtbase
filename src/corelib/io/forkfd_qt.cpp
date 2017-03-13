/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

// these might be defined via precompiled headers
#include <QtCore/qatomic.h>

#define FORKFD_NO_SPAWNFD

#if defined(QT_NO_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif

typedef QT_PREPEND_NAMESPACE(QBasicAtomicInt) ffd_atomic_int;
#define ffd_atomic_pointer(type)    QT_PREPEND_NAMESPACE(QBasicAtomicPointer<type>)

QT_BEGIN_NAMESPACE

#define FFD_ATOMIC_INIT(val)    Q_BASIC_ATOMIC_INITIALIZER(val)

#define FFD_ATOMIC_RELAXED  Relaxed
#define FFD_ATOMIC_ACQUIRE  Acquire
#define FFD_ATOMIC_RELEASE  Release
#define loadRelaxed         load
#define storeRelaxed        store

#define FFD_CONCAT(x, y)    x ## y

#define ffd_atomic_load(ptr,order)      (ptr)->FFD_CONCAT(load, order)()
#define ffd_atomic_store(ptr,val,order) (ptr)->FFD_CONCAT(store, order)(val)
#define ffd_atomic_exchange(ptr,val,order) (ptr)->FFD_CONCAT(fetchAndStore, order)(val)
#define ffd_atomic_compare_exchange(ptr,expected,desired,order1,order2) \
    (ptr)->FFD_CONCAT(testAndSet, order1)(*expected, desired, *expected)
#define ffd_atomic_add_fetch(ptr,val,order) ((ptr)->FFD_CONCAT(fetchAndAdd, order)(val) + val)

QT_END_NAMESPACE

extern "C" {
#include "../../3rdparty/forkfd/forkfd.c"
}
