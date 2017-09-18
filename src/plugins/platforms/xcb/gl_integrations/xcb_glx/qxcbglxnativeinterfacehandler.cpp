/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbglxnativeinterfacehandler.h"

#include "qglxintegration.h"
#include <QtGui/QOpenGLContext>
QT_BEGIN_NAMESPACE

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match QXcbGlxNativeInterfaceHandler::ResourceType
        QByteArrayLiteral("glxconfig"),
        QByteArrayLiteral("glxcontext"),
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (key == names[i])
            return i;
    }

    return sizeof(names) / sizeof(names[0]);
}

QXcbGlxNativeInterfaceHandler::QXcbGlxNativeInterfaceHandler(QXcbNativeInterface *nativeInterface)
    : QXcbNativeInterfaceHandler(nativeInterface)
{
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbGlxNativeInterfaceHandler::nativeResourceFunctionForContext(const QByteArray &resource) const
{
    switch (resourceType(resource)) {
    case GLXConfig:
        return glxConfigForContext;
    case GLXContext:
        return glxContextForContext;
    default:
        break;
    }
    return nullptr;
}

void *QXcbGlxNativeInterfaceHandler::glxContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QGLXContext *glxPlatformContext = static_cast<QGLXContext *>(context->handle());
    return glxPlatformContext->glxContext();
}

void *QXcbGlxNativeInterfaceHandler::glxConfigForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QGLXContext *glxPlatformContext = static_cast<QGLXContext *>(context->handle());
    return glxPlatformContext->glxConfig();

}

QT_END_NAMESPACE
