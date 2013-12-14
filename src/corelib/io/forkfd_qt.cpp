/****************************************************************************
**
** Copyright (C) 2014 Intel Corporation
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

// these might be defined via precompiled headers
#ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#endif
#if !defined(_XOPEN_SOURCE) && !defined(__QNXNTO__)
#  define _XOPEN_SOURCE 500
#endif

#include <QtCore/qatomic.h>

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
