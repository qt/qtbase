/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QGLXNATIVECONTEXT_H
#define QGLXNATIVECONTEXT_H

#include <QtCore/QMetaType>
#include <X11/Xlib.h>
#include <GL/glx.h>

QT_BEGIN_NAMESPACE

#if defined(Q_CLANG_QDOC)
typedef int GLXContext;
typedef void Display;
typedef int Window;
typedef int VisualID;
#endif

struct QGLXNativeContext
{
    QGLXNativeContext()
        : m_context(nullptr),
          m_display(nullptr),
          m_window(0),
          m_visualId(0)
    { }

    QGLXNativeContext(GLXContext ctx, Display *dpy = nullptr, Window wnd = 0, VisualID vid = 0)
        : m_context(ctx),
          m_display(dpy),
          m_window(wnd),
          m_visualId(vid)
    { }

    GLXContext context() const { return m_context; }
    Display *display() const { return m_display; }
    Window window() const { return m_window; }
    VisualID visualId() const { return m_visualId; }

private:
    GLXContext m_context;
    Display *m_display;
    Window m_window;
    VisualID m_visualId;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGLXNativeContext)

#endif // QGLXNATIVECONTEXT_H
