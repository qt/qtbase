/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QTNAMESPACEMACROS_H
#define QTNAMESPACEMACROS_H

// valid for both C and C++
#define QT_MANGLE_NAMESPACE0(x) x
#define QT_MANGLE_NAMESPACE1(a, b) a##_##b
#define QT_MANGLE_NAMESPACE2(a, b) QT_MANGLE_NAMESPACE1(a,b)
#if !defined(QT_NAMESPACE) || defined(Q_MOC_RUN) /* user namespace */
# define QT_MANGLE_NAMESPACE(name) name
#else
# define QT_MANGLE_NAMESPACE(name) QT_MANGLE_NAMESPACE2( \
        QT_MANGLE_NAMESPACE0(name), QT_MANGLE_NAMESPACE0(QT_NAMESPACE))
#endif

#ifdef __cplusplus

#if !defined(QT_NAMESPACE) || defined(Q_MOC_RUN) /* user namespace */

# define QT_PREPEND_NAMESPACE(name) ::name
# define QT_USE_NAMESPACE
# define QT_BEGIN_NAMESPACE
# define QT_END_NAMESPACE
# define QT_BEGIN_INCLUDE_NAMESPACE
# define QT_END_INCLUDE_NAMESPACE
#ifndef QT_BEGIN_MOC_NAMESPACE
# define QT_BEGIN_MOC_NAMESPACE
#endif
#ifndef QT_END_MOC_NAMESPACE
# define QT_END_MOC_NAMESPACE
#endif
# define QT_FORWARD_DECLARE_CLASS(name) class name;
# define QT_FORWARD_DECLARE_STRUCT(name) struct name;

#else /* user namespace */

# define QT_PREPEND_NAMESPACE(name) ::QT_NAMESPACE::name
# define QT_USE_NAMESPACE using namespace ::QT_NAMESPACE;
# define QT_BEGIN_NAMESPACE namespace QT_NAMESPACE {
# define QT_END_NAMESPACE }
# define QT_BEGIN_INCLUDE_NAMESPACE }
# define QT_END_INCLUDE_NAMESPACE namespace QT_NAMESPACE {
#ifndef QT_BEGIN_MOC_NAMESPACE
# define QT_BEGIN_MOC_NAMESPACE QT_USE_NAMESPACE
#endif
#ifndef QT_END_MOC_NAMESPACE
# define QT_END_MOC_NAMESPACE
#endif
# define QT_FORWARD_DECLARE_CLASS(name) \
    QT_BEGIN_NAMESPACE class name; QT_END_NAMESPACE \
    using QT_PREPEND_NAMESPACE(name);

# define QT_FORWARD_DECLARE_STRUCT(name) \
    QT_BEGIN_NAMESPACE struct name; QT_END_NAMESPACE \
    using QT_PREPEND_NAMESPACE(name);

namespace QT_NAMESPACE {}

# ifndef QT_BOOTSTRAPPED
# ifndef QT_NO_USING_NAMESPACE
   /*
    This expands to a "using QT_NAMESPACE" also in _header files_.
    It is the only way the feature can be used without too much
    pain, but if people _really_ do not want it they can add
    DEFINES += QT_NO_USING_NAMESPACE to their .pro files.
    */
   QT_USE_NAMESPACE
# endif
# endif

#endif /* user namespace */

#else /* __cplusplus */

# define QT_BEGIN_NAMESPACE
# define QT_END_NAMESPACE
# define QT_USE_NAMESPACE
# define QT_BEGIN_INCLUDE_NAMESPACE
# define QT_END_INCLUDE_NAMESPACE

#endif /* __cplusplus */

#endif /* QTNAMESPACEMACROS_H */
