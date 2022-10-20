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

#ifndef QGUIAPPLICATION_PLATFORM_H
#define QGUIAPPLICATION_PLATFORM_H

#include <QtGui/qtguiglobal.h>

#include <QtCore/qnativeinterface.h>
#include <QtGui/qguiapplication.h>

#if QT_CONFIG(xcb)
typedef struct _XDisplay Display;
struct xcb_connection_t;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface
{

#if QT_CONFIG(xcb) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QX11Application
{
    QT_DECLARE_NATIVE_INTERFACE(QX11Application, 1, QGuiApplication)
    virtual Display *display() const = 0;
    virtual xcb_connection_t *connection() const = 0;
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QGUIAPPLICATION_PLATFORM_H
