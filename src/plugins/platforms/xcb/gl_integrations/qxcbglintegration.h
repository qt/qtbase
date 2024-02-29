// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qxcbexport.h"
#include "qxcbwindow.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QXcbNativeInterfaceHandler;

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcQpaGl, Q_XCB_EXPORT)

class Q_XCB_EXPORT QXcbGlIntegration
{
public:
    QXcbGlIntegration();
    virtual ~QXcbGlIntegration();
    virtual bool initialize(QXcbConnection *connection) = 0;

    virtual bool supportsThreadedOpenGL() const { return false; }
    virtual bool supportsSwitchableWidgetComposition()  const { return true; }
    virtual bool handleXcbEvent(xcb_generic_event_t *event, uint responseType);

    virtual QXcbWindow *createWindow(QWindow *window) const = 0;
#ifndef QT_NO_OPENGL
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const = 0;
#endif
    virtual QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const = 0;

    virtual QXcbNativeInterfaceHandler *nativeInterfaceHandler() const  { return nullptr; }
};

QT_END_NAMESPACE
