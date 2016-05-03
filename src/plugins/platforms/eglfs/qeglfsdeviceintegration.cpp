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

#include "qeglfsdeviceintegration.h"
#include "qeglfsintegration.h"
#include "qeglfscursor.h"
#include "qeglfswindow.h"
#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QGuiApplication>
#include <private/qguiapplication_p.h>
#include <QScreen>
#include <QDir>
#include <QRegularExpression>
#include <QLoggingCategory>

#if defined(Q_OS_LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#endif

#include <private/qfactoryloader_p.h>
#include <private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglDevDebug, "qt.qpa.egldeviceintegration")

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QEGLDeviceIntegrationFactoryInterface_iid, QLatin1String("/egldeviceintegrations"), Qt::CaseInsensitive))

#ifndef QT_NO_LIBRARY
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QEGLDeviceIntegrationFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))
#endif // QT_NO_LIBRARY

QStringList QEGLDeviceIntegrationFactory::keys(const QString &pluginPath)
{
    QStringList list;
#ifndef QT_NO_LIBRARY
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QStringLiteral(" (from ")
                    + QDir::toNativeSeparators(pluginPath)
                    + QLatin1Char(')');
            const QStringList::iterator end = list.end();
            for (QStringList::iterator it = list.begin(); it != end; ++it)
                (*it).append(postFix);
        }
    }
#else
    Q_UNUSED(pluginPath);
#endif
    list.append(loader()->keyMap().values());
    qCDebug(qLcEglDevDebug) << "EGL device integration plugin keys:" << list;
    return list;
}

QEGLDeviceIntegration *QEGLDeviceIntegrationFactory::create(const QString &key, const QString &pluginPath)
{
    QEGLDeviceIntegration *integration = Q_NULLPTR;
#ifndef QT_NO_LIBRARY
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        integration = qLoadPlugin<QEGLDeviceIntegration, QEGLDeviceIntegrationPlugin>(directLoader(), key);
    }
#else
    Q_UNUSED(pluginPath);
#endif
    if (!integration)
        integration = qLoadPlugin<QEGLDeviceIntegration, QEGLDeviceIntegrationPlugin>(loader(), key);
    if (integration)
        qCDebug(qLcEglDevDebug) << "Using EGL device integration" << key;
    else
        qCWarning(qLcEglDevDebug) << "Failed to load EGL device integration" << key;

    return integration;
}

static int framebuffer = -1;

QByteArray QEGLDeviceIntegration::fbDeviceName() const
{
#ifdef Q_OS_LINUX
    QByteArray fbDev = qgetenv("QT_QPA_EGLFS_FB");
    if (fbDev.isEmpty())
        fbDev = QByteArrayLiteral("/dev/fb0");

    return fbDev;
#else
    return QByteArray();
#endif
}

int QEGLDeviceIntegration::framebufferIndex() const
{
    int fbIndex = 0;
#ifndef QT_NO_REGULAREXPRESSION
    QRegularExpression fbIndexRx(QLatin1String("fb(\\d+)"));
    QRegularExpressionMatch match = fbIndexRx.match(QString::fromLocal8Bit(fbDeviceName()));
    if (match.hasMatch())
        fbIndex = match.captured(1).toInt();
#endif
    return fbIndex;
}

void QEGLDeviceIntegration::platformInit()
{
#ifdef Q_OS_LINUX
    QByteArray fbDev = fbDeviceName();

    framebuffer = qt_safe_open(fbDev, O_RDONLY);

    if (Q_UNLIKELY(framebuffer == -1)) {
        qWarning("EGLFS: Failed to open %s", fbDev.constData());
        qFatal("EGLFS: Can't continue without a display");
    }

#ifdef FBIOBLANK
    ioctl(framebuffer, FBIOBLANK, VESA_NO_BLANKING);
#endif
#endif
}

void QEGLDeviceIntegration::platformDestroy()
{
#ifdef Q_OS_LINUX
    if (framebuffer != -1)
        close(framebuffer);
#endif
}

EGLNativeDisplayType QEGLDeviceIntegration::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

EGLDisplay QEGLDeviceIntegration::createDisplay(EGLNativeDisplayType nativeDisplay)
{
    return eglGetDisplay(nativeDisplay);
}

bool QEGLDeviceIntegration::usesDefaultScreen()
{
    return true;
}

void QEGLDeviceIntegration::screenInit()
{
    // Nothing to do here. Called only when usesDefaultScreen is false.
}

void QEGLDeviceIntegration::screenDestroy()
{
    QGuiApplication *app = qGuiApp;
    QEglFSIntegration *platformIntegration = static_cast<QEglFSIntegration *>(
        QGuiApplicationPrivate::platformIntegration());
    while (!app->screens().isEmpty())
        platformIntegration->removeScreen(app->screens().last()->handle());
}

QSizeF QEGLDeviceIntegration::physicalScreenSize() const
{
    return q_physicalScreenSizeFromFb(framebuffer, screenSize());
}

QSize QEGLDeviceIntegration::screenSize() const
{
    return q_screenSizeFromFb(framebuffer);
}

QDpi QEGLDeviceIntegration::logicalDpi() const
{
    const QSizeF ps = physicalScreenSize();
    const QSize s = screenSize();

    if (!ps.isEmpty() && !s.isEmpty())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
        return QDpi(100, 100);
}

qreal QEGLDeviceIntegration::pixelDensity() const
{
    return qRound(logicalDpi().first / qreal(100));
}

Qt::ScreenOrientation QEGLDeviceIntegration::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QEGLDeviceIntegration::orientation() const
{
    return Qt::PrimaryOrientation;
}

int QEGLDeviceIntegration::screenDepth() const
{
    return q_screenDepthFromFb(framebuffer);
}

QImage::Format QEGLDeviceIntegration::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

qreal QEGLDeviceIntegration::refreshRate() const
{
    return q_refreshRateFromFb(framebuffer);
}

EGLint QEGLDeviceIntegration::surfaceType() const
{
    return EGL_WINDOW_BIT;
}

QSurfaceFormat QEGLDeviceIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format = inputFormat;

    static const bool force888 = qEnvironmentVariableIntValue("QT_QPA_EGLFS_FORCE888");
    if (force888) {
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
    }

    return format;
}

bool QEGLDeviceIntegration::filterConfig(EGLDisplay, EGLConfig) const
{
    return true;
}

QEglFSWindow *QEGLDeviceIntegration::createWindow(QWindow *window) const
{
    return new QEglFSWindow(window);
}

EGLNativeWindowType QEGLDeviceIntegration::createNativeWindow(QPlatformWindow *platformWindow,
                                                    const QSize &size,
                                                    const QSurfaceFormat &format)
{
    Q_UNUSED(platformWindow);
    Q_UNUSED(size);
    Q_UNUSED(format);
    return 0;
}

EGLNativeWindowType QEGLDeviceIntegration::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    return 0;
}

void QEGLDeviceIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEGLDeviceIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QPlatformCursor *QEGLDeviceIntegration::createCursor(QPlatformScreen *screen) const
{
    return new QEglFSCursor(screen);
}

void QEGLDeviceIntegration::waitForVSync(QPlatformSurface *surface) const
{
    Q_UNUSED(surface);

#if defined(Q_OS_LINUX) && defined(FBIO_WAITFORVSYNC)
    static const bool forceSync = qEnvironmentVariableIntValue("QT_QPA_EGLFS_FORCEVSYNC");
    if (forceSync && framebuffer != -1) {
        int arg = 0;
        if (ioctl(framebuffer, FBIO_WAITFORVSYNC, &arg) == -1)
            qWarning("Could not wait for vsync.");
    }
#endif
}

void QEGLDeviceIntegration::presentBuffer(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
}

bool QEGLDeviceIntegration::supportsPBuffers() const
{
    return true;
}

bool QEGLDeviceIntegration::supportsSurfacelessContexts() const
{
    return true;
}

void *QEGLDeviceIntegration::wlDisplay() const
{
    return Q_NULLPTR;
}

QT_END_NAMESPACE
