/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// these might be defined via precompiled headers
#include <QtCore/qatomic.h>
#include "qprocess_p.h"

#ifdef QPROCESS_USE_SPAWN
#  define FORKFD_NO_FORKFD
#else
#  define FORKFD_NO_SPAWNFD
#endif

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
