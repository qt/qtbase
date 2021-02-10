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

#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QNativeInterface::QAndroidApplication
    \since 6.2
    \brief Native interface to a core application on Android.

    Accessed through QCoreApplication::nativeInterface().

    \inmodule QtCore
    \inheaderfile QCoreApplication
    \ingroup native-interfaces
    \ingroup native-interfaces-qcoreapplication
*/
QT_DEFINE_NATIVE_INTERFACE(QAndroidApplication);

/*!
     \fn jobject QNativeInterface::QAndroidApplication::context()

    Returns the Android context as a \c jobject. The context is an \c Activity
    if the main activity object is valid. Otherwise, the context is a \c Service.

    \since 6.2
*/
jobject QNativeInterface::QAndroidApplication::context()
{
    return QtAndroidPrivate::context();
}

/*!
     \fn bool QNativeInterface::QAndroidApplication::isActivityContext()

    Returns \c true if QAndroidApplication::context() provides an \c Activity
    context.

    \since 6.2
*/
bool QNativeInterface::QAndroidApplication::isActivityContext()
{
    return QtAndroidPrivate::activity();
}

QT_END_NAMESPACE
