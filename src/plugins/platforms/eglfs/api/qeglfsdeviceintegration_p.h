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

#ifndef QEGLFSDEVICEINTEGRATION_H
#define QEGLFSDEVICEINTEGRATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qeglfsglobal_p.h"
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <QtCore/QString>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QPlatformSurface;
class QEglFSWindow;

#define QEglFSDeviceIntegrationFactoryInterface_iid "org.qt-project.qt.qpa.egl.QEglFSDeviceIntegrationFactoryInterface.5.5"

class Q_EGLFS_EXPORT QEglFSDeviceIntegration
{
public:
    virtual ~QEglFSDeviceIntegration() { }

    virtual void platformInit();
    virtual void platformDestroy();
    virtual EGLNativeDisplayType platformDisplay() const;
    virtual EGLDisplay createDisplay(EGLNativeDisplayType nativeDisplay);
    virtual bool usesDefaultScreen();
    virtual void screenInit();
    virtual void screenDestroy();
    virtual QSizeF physicalScreenSize() const;
    virtual QSize screenSize() const;
    virtual QDpi logicalDpi() const;
    virtual qreal pixelDensity() const;
    virtual Qt::ScreenOrientation nativeOrientation() const;
    virtual Qt::ScreenOrientation orientation() const;
    virtual int screenDepth() const;
    virtual QImage::Format screenFormat() const;
    virtual qreal refreshRate() const;
    virtual QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const;
    virtual EGLint surfaceType() const;
    virtual QEglFSWindow *createWindow(QWindow *window) const;
    virtual EGLNativeWindowType createNativeWindow(QPlatformWindow *platformWindow,
                                                   const QSize &size,
                                                   const QSurfaceFormat &format);
    virtual EGLNativeWindowType createNativeOffscreenWindow(const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
    virtual bool hasCapability(QPlatformIntegration::Capability cap) const;
    virtual QPlatformCursor *createCursor(QPlatformScreen *screen) const;
    virtual bool filterConfig(EGLDisplay display, EGLConfig config) const;
    virtual void waitForVSync(QPlatformSurface *surface) const;
    virtual void presentBuffer(QPlatformSurface *surface);
    virtual QByteArray fbDeviceName() const;
    virtual int framebufferIndex() const;
    virtual bool supportsPBuffers() const;
    virtual bool supportsSurfacelessContexts() const;
    virtual QFunctionPointer platformFunction(const QByteArray &function) const;
    virtual void *nativeResourceForIntegration(const QByteArray &name);
    virtual void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen);
    virtual void *wlDisplay() const;

#if QT_CONFIG(vulkan)
    virtual QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance);
#endif

    static EGLConfig chooseConfig(EGLDisplay display, const QSurfaceFormat &format);
};

class Q_EGLFS_EXPORT QEglFSDeviceIntegrationPlugin : public QObject
{
    Q_OBJECT

public:
    virtual QEglFSDeviceIntegration *create() = 0;

    // the pattern expected by qLoadPlugin calls for a QString argument.
    // we don't need it, so don't bother subclasses with it:
    QEglFSDeviceIntegration *create(const QString &) { return create(); }
};

class Q_EGLFS_EXPORT QEglFSDeviceIntegrationFactory
{
public:
    static QStringList keys(const QString &pluginPath = QString());
    static QEglFSDeviceIntegration *create(const QString &name, const QString &platformPluginPath = QString());
};

QT_END_NAMESPACE

#endif // QEGLDEVICEINTEGRATION_H
