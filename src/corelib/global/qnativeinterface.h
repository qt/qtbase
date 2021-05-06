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

#include <QtCore/qglobal.h>

#ifndef QNATIVEINTERFACE_H
#define QNATIVEINTERFACE_H

QT_BEGIN_NAMESPACE

// Ensures that the interface's typeinfo is exported so that
// dynamic casts work reliably, and protects the destructor
// so that pointers to the interface can't be deleted.
#define QT_DECLARE_NATIVE_INTERFACE(InterfaceClass) \
    protected: virtual ~InterfaceClass(); public:

// Declares an accessor for the native interface
#define QT_DECLARE_NATIVE_INTERFACE_ACCESSOR \
    template <typename QNativeInterface> \
    QNativeInterface *nativeInterface() const;

// Provides a definition for the interface destructor
#define QT_DEFINE_NATIVE_INTERFACE_2(Namespace, InterfaceClass) \
    QT_PREPEND_NAMESPACE(Namespace)::InterfaceClass::~InterfaceClass() = default

// Provides a definition for the destructor, and an explicit
// template instantiation of the native interface accessor.
#define QT_DEFINE_NATIVE_INTERFACE_3(Namespace, InterfaceClass, PublicClass) \
    QT_DEFINE_NATIVE_INTERFACE_2(Namespace, InterfaceClass); \
    template Q_DECL_EXPORT QT_PREPEND_NAMESPACE(Namespace)::InterfaceClass *PublicClass::nativeInterface() const

#define QT_DEFINE_NATIVE_INTERFACE(...) QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface, __VA_ARGS__)
#define QT_DEFINE_PRIVATE_NATIVE_INTERFACE(...) QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface::Private, __VA_ARGS__)

QT_END_NAMESPACE

#endif // QNATIVEINTERFACE_H
