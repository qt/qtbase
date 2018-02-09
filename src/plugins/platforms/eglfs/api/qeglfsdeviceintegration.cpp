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

#include "qeglfsdeviceintegration_p.h"
#include "qeglfsintegration_p.h"
#ifndef QT_NO_OPENGL
# include "qeglfscursor_p.h"
#endif
#include "qeglfswindow_p.h"
#include "qeglfsscreen_p.h"
#include "qeglfshooks_p.h"

#include <QtEglSupport/private/qeglconvenience_p.h>
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
                          (QEglFSDeviceIntegrationFactoryInterface_iid, QLatin1String("/egldeviceintegrations"), Qt::CaseInsensitive))

#if QT_CONFIG(library)
Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, directLoader,
                          (QEglFSDeviceIntegrationFactoryInterface_iid, QLatin1String(""), Qt::CaseInsensitive))

#endif // QT_CONFIG(library)

QStringList QEglFSDeviceIntegrationFactory::keys(const QString &pluginPath)
{
    QStringList list;
#if QT_CONFIG(library)
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        list = directLoader()->keyMap().values();
        if (!list.isEmpty()) {
            const QString postFix = QLatin1String(" (from ")
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

QEglFSDeviceIntegration *QEglFSDeviceIntegrationFactory::create(const QString &key, const QString &pluginPath)
{
    QEglFSDeviceIntegration *integration = nullptr;
#if QT_CONFIG(library)
    if (!pluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(pluginPath);
        integration = qLoadPlugin<QEglFSDeviceIntegration, QEglFSDeviceIntegrationPlugin>(directLoader(), key);
    }
#else
    Q_UNUSED(pluginPath);
#endif
    if (!integration)
        integration = qLoadPlugin<QEglFSDeviceIntegration, QEglFSDeviceIntegrationPlugin>(loader(), key);
    if (integration)
        qCDebug(qLcEglDevDebug) << "Using EGL device integration" << key;
    else
        qCWarning(qLcEglDevDebug) << "Failed to load EGL device integration" << key;

    return integration;
}

static int framebuffer = -1;

QByteArray QEglFSDeviceIntegration::fbDeviceName() const
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

int QEglFSDeviceIntegration::framebufferIndex() const
{
    int fbIndex = 0;
#if QT_CONFIG(regularexpression)
    QRegularExpression fbIndexRx(QLatin1String("fb(\\d+)"));
    QRegularExpressionMatch match = fbIndexRx.match(QString::fromLocal8Bit(fbDeviceName()));
    if (match.hasMatch())
        fbIndex = match.captured(1).toInt();
#endif
    return fbIndex;
}

void QEglFSDeviceIntegration::platformInit()
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

void QEglFSDeviceIntegration::platformDestroy()
{
#ifdef Q_OS_LINUX
    if (framebuffer != -1)
        close(framebuffer);
#endif
}

EGLNativeDisplayType QEglFSDeviceIntegration::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

EGLDisplay QEglFSDeviceIntegration::createDisplay(EGLNativeDisplayType nativeDisplay)
{
    return eglGetDisplay(nativeDisplay);
}

bool QEglFSDeviceIntegration::usesDefaultScreen()
{
    return true;
}

void QEglFSDeviceIntegration::screenInit()
{
    // Nothing to do here. Called only when usesDefaultScreen is false.
}

void QEglFSDeviceIntegration::screenDestroy()
{
    QGuiApplication *app = qGuiApp;
    QEglFSIntegration *platformIntegration = static_cast<QEglFSIntegration *>(
        QGuiApplicationPrivate::platformIntegration());
    while (!app->screens().isEmpty())
        platformIntegration->removeScreen(app->screens().constLast()->handle());
}

QSizeF QEglFSDeviceIntegration::physicalScreenSize() const
{
    return q_physicalScreenSizeFromFb(framebuffer, screenSize());
}

QSize QEglFSDeviceIntegration::screenSize() const
{
    return q_screenSizeFromFb(framebuffer);
}

QDpi QEglFSDeviceIntegration::logicalDpi() const
{
    const QSizeF ps = physicalScreenSize();
    const QSize s = screenSize();

    if (!ps.isEmpty() && !s.isEmpty())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
        return QDpi(100, 100);
}

qreal QEglFSDeviceIntegration::pixelDensity() const
{
    return qMax(1, qRound(logicalDpi().first / qreal(100)));
}

Qt::ScreenOrientation QEglFSDeviceIntegration::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QEglFSDeviceIntegration::orientation() const
{
    return Qt::PrimaryOrientation;
}

int QEglFSDeviceIntegration::screenDepth() const
{
    return q_screenDepthFromFb(framebuffer);
}

QImage::Format QEglFSDeviceIntegration::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

qreal QEglFSDeviceIntegration::refreshRate() const
{
    return q_refreshRateFromFb(framebuffer);
}

EGLint QEglFSDeviceIntegration::surfaceType() const
{
    return EGL_WINDOW_BIT;
}

QSurfaceFormat QEglFSDeviceIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
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

bool QEglFSDeviceIntegration::filterConfig(EGLDisplay, EGLConfig) const
{
    return true;
}

QEglFSWindow *QEglFSDeviceIntegration::createWindow(QWindow *window) const
{
    return new QEglFSWindow(window);
}

EGLNativeWindowType QEglFSDeviceIntegration::createNativeWindow(QPlatformWindow *platformWindow,
                                                    const QSize &size,
                                                    const QSurfaceFormat &format)
{
    Q_UNUSED(platformWindow);
    Q_UNUSED(size);
    Q_UNUSED(format);
    return 0;
}

EGLNativeWindowType QEglFSDeviceIntegration::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    return 0;
}

void QEglFSDeviceIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEglFSDeviceIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QPlatformCursor *QEglFSDeviceIntegration::createCursor(QPlatformScreen *screen) const
{
#ifndef QT_NO_OPENGL
    return new QEglFSCursor(static_cast<QEglFSScreen *>(screen));
#else
    Q_UNUSED(screen);
    return nullptr;
#endif
}

void QEglFSDeviceIntegration::waitForVSync(QPlatformSurface *surface) const
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

void QEglFSDeviceIntegration::presentBuffer(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
}

bool QEglFSDeviceIntegration::supportsPBuffers() const
{
    return true;
}

bool QEglFSDeviceIntegration::supportsSurfacelessContexts() const
{
    return true;
}

QFunctionPointer QEglFSDeviceIntegration::platformFunction(const QByteArray &function) const
{
    Q_UNUSED(function);
    return nullptr;
}

void *QEglFSDeviceIntegration::nativeResourceForIntegration(const QByteArray &name)
{
    Q_UNUSED(name);
    return nullptr;
}

void *QEglFSDeviceIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    Q_UNUSED(resource);
    Q_UNUSED(screen);
    return nullptr;
}

void *QEglFSDeviceIntegration::wlDisplay() const
{
    return nullptr;
}

EGLConfig QEglFSDeviceIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public QEglConfigChooser {
    public:
        Chooser(EGLDisplay display)
            : QEglConfigChooser(display) { }
        bool filterConfig(EGLConfig config) const override {
            return qt_egl_device_integration()->filterConfig(display(), config)
                    && QEglConfigChooser::filterConfig(config);
        }
    };

    Chooser chooser(display);
    chooser.setSurfaceType(qt_egl_device_integration()->surfaceType());
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

QT_END_NAMESPACE
