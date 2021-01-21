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

#ifndef QXCBGLINTEGRATION_H

#include "qxcbexport.h"
#include "qxcbwindow.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QXcbNativeInterfaceHandler;

Q_XCB_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcQpaGl)

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

#endif //QXCBGLINTEGRATION_H
