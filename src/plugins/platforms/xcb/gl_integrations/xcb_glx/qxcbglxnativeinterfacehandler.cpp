/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
    return Q_NULLPTR;
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
