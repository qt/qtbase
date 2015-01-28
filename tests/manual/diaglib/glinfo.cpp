/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "glinfo.h"

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLWidget>
#if QT_VERSION > 0x050000
#  if QT_VERSION >= 0x050400
#    include <QtWidgets/QOpenGLWidget>
#    include <QtGui/QOpenGLWindow>
#  else // 5.4
#    include <QtGui/QWindow>
#  endif // 5.0..5.4
#  include <QtGui/QOpenGLContext>
#  include <QtGui/QOpenGLFunctions>
#  include <QtGui/QWindow>
#endif
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QTimer>

namespace QtDiag {

#if QT_VERSION > 0x050000

static QString getGlString(const QOpenGLContext *ctx, GLenum name)
{
    if (const GLubyte *p = ctx->functions()->glGetString(name))
        return QString::fromLatin1(reinterpret_cast<const char *>(p));
    return QString();
}

static QString glInfo(const QOpenGLContext *ctx)
{
    return getGlString(ctx, GL_VENDOR)
        + QLatin1Char('\n')
        + getGlString(ctx, GL_RENDERER);
}

static QString glInfo(const QGLContext *ctx)
{
    return glInfo(ctx->contextHandle());
}

QString glInfo(const QObject *o)
{
#  if QT_VERSION >= 0x050400
    if (o->isWindowType()) {
        if (const QOpenGLWindow *oglw = qobject_cast<const QOpenGLWindow *>(o))
            return glInfo(oglw->context());
        return QString();
    }
#  endif // 5.4
    if (o->isWidgetType()) {
        if (const QGLWidget *g = qobject_cast<const QGLWidget *>(o))
            return glInfo(g->context());
#  if QT_VERSION >= 0x050400
        if (const QOpenGLWidget *g = qobject_cast<const QOpenGLWidget *>(o))
            return glInfo(g->context());
#  endif // 5.4
    }
    return QString();
}

#else // Qt4:

static QString getGlString(GLenum name)
{
    if (const GLubyte *p = glGetString(name))
        return QString::fromLatin1(reinterpret_cast<const char *>(p));
    return QString();
}

QString glInfo(const QObject *)
{
    return getGlString(GL_VENDOR) + QLatin1Char('\n') + getGlString(GL_RENDERER);
}

#endif

} // namespace QtDiag
