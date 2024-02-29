// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include "qxcbwindow.h"

#include "qxcbeglinclude.h"

QT_BEGIN_NAMESPACE

class QXcbEglIntegration;

class QXcbEglWindow : public QXcbWindow
{
public:
    QXcbEglWindow(QWindow *window, QXcbEglIntegration *glIntegration);
    ~QXcbEglWindow();

    EGLSurface eglSurface() const { return m_surface; }

    QXcbEglIntegration *glIntegration() const { return m_glIntegration; }

protected:
    void create() override;
    void resolveFormat(const QSurfaceFormat &format) override;
    const xcb_visualtype_t *createVisual() override;

private:
    QXcbEglIntegration *m_glIntegration;
    EGLConfig m_config;
    EGLSurface m_surface;
};

QT_END_NAMESPACE
