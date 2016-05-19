/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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

#include "qeglfsglobal.h"
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>
#include <QtCore/QString>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QPlatformSurface;
class QEglFSWindow;

#define QEGLDeviceIntegrationFactoryInterface_iid "org.qt-project.qt.qpa.egl.QEGLDeviceIntegrationFactoryInterface.5.5"

class Q_EGLFS_EXPORT QEGLDeviceIntegration
{
public:
    virtual ~QEGLDeviceIntegration() { }

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

    virtual void *wlDisplay() const;
};

class Q_EGLFS_EXPORT QEGLDeviceIntegrationPlugin : public QObject
{
    Q_OBJECT

public:
    virtual QEGLDeviceIntegration *create() = 0;

    // the pattern expected by qLoadPlugin calls for a QString argument.
    // we don't need it, so don't bother subclasses with it:
    QEGLDeviceIntegration *create(const QString &) { return create(); }
};

class Q_EGLFS_EXPORT QEGLDeviceIntegrationFactory
{
public:
    static QStringList keys(const QString &pluginPath = QString());
    static QEGLDeviceIntegration *create(const QString &name, const QString &platformPluginPath = QString());
};

QT_END_NAMESPACE

#endif // QEGLDEVICEINTEGRATION_H
