// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
