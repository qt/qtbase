/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glinfo.h"

#include <QOpenGLFunctions>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtOpenGL/QOpenGLWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QWindow>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QTimer>

namespace QtDiag {

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

QString glInfo(const QObject *o)
{
    if (o->isWindowType()) {
        if (const QOpenGLWindow *oglw = qobject_cast<const QOpenGLWindow *>(o))
            return glInfo(oglw->context());
        return QString();
    }

    if (o->isWidgetType()) {
        if (const QOpenGLWidget *g = qobject_cast<const QOpenGLWidget *>(o))
            return glInfo(g->context());
    }
    return QString();
}

} // namespace QtDiag
