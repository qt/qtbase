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

#ifndef QDIRECTFB_EGL_H
#define QDIRECTFB_EGL_H

#include "qdirectfbintegration.h"

#ifdef DIRECTFB_GL_EGL

QT_BEGIN_NAMESPACE

class QDirectFbIntegrationEGL : public QDirectFbIntegration {
public:
    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const;

    bool hasCapability(QPlatformIntegration::Capability cap) const;

protected:
    void initializeScreen();
};

QT_END_NAMESPACE

#endif
#endif
