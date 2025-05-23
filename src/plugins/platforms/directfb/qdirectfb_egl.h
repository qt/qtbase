// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
