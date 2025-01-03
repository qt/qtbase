// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXCBGLXINTEGRATION_H
#define QXCBGLXINTEGRATION_H

#include "qxcbglintegration.h"

QT_BEGIN_NAMESPACE

class QXcbNativeInterfaceHandler;

class QXcbGlxIntegration : public QXcbGlIntegration,
                           public QNativeInterface::Private::QGLXIntegration
{
public:
    QXcbGlxIntegration();
    ~QXcbGlxIntegration();

    bool initialize(QXcbConnection *connection) override;
    bool handleXcbEvent(xcb_generic_event_t *event, uint responseType) override;

    QXcbWindow *createWindow(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
    QOpenGLContext *createOpenGLContext(GLXContext context, void *visualInfo, QOpenGLContext *shareContext) const override;

    bool supportsThreadedOpenGL() const override;
    bool supportsSwitchableWidgetComposition() const override;

private:
    QXcbConnection *m_connection;
    uint32_t m_glx_first_event;

    QScopedPointer<QXcbNativeInterfaceHandler> m_native_interface_handler;
};

QT_END_NAMESPACE

#endif //QXCBGLXINTEGRATION_H
