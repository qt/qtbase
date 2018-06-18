/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qopenglfunctions_es2.h"
#include "qopenglcontext.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLFunctions_ES2
    \inmodule QtGui
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
    const QPair<int, int> v = qMakePair(f.majorVersion(), f.minorVersion());
    if (v < qMakePair(2, 0))
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
