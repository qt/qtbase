// Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qopenglfunctions_es2.h"
#include "qopenglcontext.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLFunctions_ES2
    \inmodule QtOpenGL
    \since 5.1
    \wrapper
    \brief The QOpenGLFunctions_ES2 class provides all functions for OpenGL ES 2.

    This class is a wrapper for OpenGL ES 2 functions. See reference pages on
    \l {http://www.khronos.org/opengles/sdk/docs/man/}{khronos.org} for
    function documentation.

    \sa QAbstractOpenGLFunctions
*/

QOpenGLFunctions_ES2::QOpenGLFunctions_ES2()
 : QAbstractOpenGLFunctions()
 , d_es2(0)
{
}

QOpenGLFunctions_ES2::~QOpenGLFunctions_ES2()
{
}

bool QOpenGLFunctions_ES2::initializeOpenGLFunctions()
{
    if ( isInitialized() )
        return true;

    QOpenGLContext* context = QOpenGLContext::currentContext();

    // If owned by a context object make sure it is current.
    // Also check that current context is compatible
    if (((owningContext() && owningContext() == context) || !owningContext())
        && QOpenGLFunctions_ES2::isContextCompatible(context))
    {
        // Nothing to do, just flag that we are initialized
        QAbstractOpenGLFunctions::initializeOpenGLFunctions();
    }
    return isInitialized();
}

bool QOpenGLFunctions_ES2::isContextCompatible(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QSurfaceFormat f = context->format();
    const auto v = std::pair(f.majorVersion(), f.minorVersion());
    if (v < std::pair(2, 0))
        return false;
    if (f.renderableType() != QSurfaceFormat::OpenGLES)
        return false;

    return true;
}

QOpenGLVersionProfile QOpenGLFunctions_ES2::versionProfile()
{
    QOpenGLVersionProfile v;
    return v;
}

QT_END_NAMESPACE
