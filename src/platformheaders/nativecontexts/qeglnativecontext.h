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

#ifndef QEGLNATIVECONTEXT_H
#define QEGLNATIVECONTEXT_H

#include <QtCore/QMetaType>

// Leave including egl.h with the appropriate defines to the client.

QT_BEGIN_NAMESPACE

#if defined(Q_CLANG_QDOC)
typedef int EGLContext;
typedef int EGLDisplay;
#endif

struct QEGLNativeContext
{
    QEGLNativeContext()
        : m_context(nullptr),
          m_display(nullptr)
    { }

    QEGLNativeContext(EGLContext ctx, EGLDisplay dpy)
        : m_context(ctx),
          m_display(dpy)
    { }

    EGLContext context() const { return m_context; }
    EGLDisplay display() const { return m_display; }

private:
    EGLContext m_context;
    EGLDisplay m_display;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QEGLNativeContext)

#endif // QEGLNATIVECONTEXT_H
