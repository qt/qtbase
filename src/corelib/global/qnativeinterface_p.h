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

#ifndef QNATIVEINTERFACE_P_H
#define QNATIVEINTERFACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
Q_CORE_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcNativeInterface)
}

// Provides a definition for the interface destructor
#define QT_DEFINE_NATIVE_INTERFACE_2(Namespace, InterfaceClass)                                    \
    QT_PREPEND_NAMESPACE(Namespace)::InterfaceClass::~InterfaceClass() = default

#define QT_DEFINE_NATIVE_INTERFACE(...)                                                            \
    QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface, __VA_ARGS__)
#define QT_DEFINE_PRIVATE_NATIVE_INTERFACE(...)                                                    \
    QT_OVERLOADED_MACRO(QT_DEFINE_NATIVE_INTERFACE, QNativeInterface::Private, __VA_ARGS__)

#define QT_NATIVE_INTERFACE_RETURN_IF(NativeInterface, baseType)                                   \
    {                                                                                              \
        using QtPrivate::lcNativeInterface;                                                        \
        using QNativeInterface::Private::TypeInfo;                                                 \
        qCDebug(lcNativeInterface, "Comparing requested interface name %s with available %s",      \
                name, TypeInfo<NativeInterface>::name());                                          \
        if (qstrcmp(name, TypeInfo<NativeInterface>::name()) == 0) {                               \
            qCDebug(lcNativeInterface,                                                             \
                    "Match for interface %s. Comparing revisions (requested %d / available %d)",   \
                    name, revision, TypeInfo<NativeInterface>::revision());                        \
            if (revision == TypeInfo<NativeInterface>::revision()) {                               \
                qCDebug(lcNativeInterface) << "Full match. Returning dynamic cast of" << baseType; \
                return dynamic_cast<NativeInterface *>(baseType);                                  \
            } else {                                                                               \
                qCWarning(lcNativeInterface,                                                       \
                          "Native interface revision mismatch (requested %d / available %d) for "  \
                          "interface %s",                                                          \
                          revision, TypeInfo<NativeInterface>::revision(), name);                  \
                return nullptr;                                                                    \
            }                                                                                      \
        } else {                                                                                   \
            qCDebug(lcNativeInterface, "No match for requested interface name %s", name);          \
        }                                                                                          \
    }

QT_END_NAMESPACE

#endif // QNATIVEINTERFACE_P_H
