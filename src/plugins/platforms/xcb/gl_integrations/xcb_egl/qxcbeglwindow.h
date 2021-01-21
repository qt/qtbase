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

#ifndef QXCBEGLWINDOW_H
#define QXCBEGLWINDOW_H

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

#if QT_CONFIG(xcb_xlib)
    const xcb_visualtype_t *createVisual() override;
#endif

private:
    QXcbEglIntegration *m_glIntegration;
    EGLConfig m_config;
    EGLSurface m_surface;
};

QT_END_NAMESPACE
#endif //QXCBEGLWINDOW_H
