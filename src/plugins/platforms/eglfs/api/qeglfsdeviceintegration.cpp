// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfsdeviceintegration_p.h"
#include "qeglfsintegration_p.h"
#ifndef QT_NO_OPENGL
# include "qeglfscursor_p.h"
#endif
#include "qeglfswindow_p.h"
#include "qeglfsscreen_p.h"
#include "qeglfshooks_p.h"

#include <QtGui/private/qeglconvenience_p.h>
#include <QGuiApplication>
#include <private/qguiapplication_p.h>
#include <QScreen>
#include <QDir>
#if QT_CONFIG(regularexpression)
#  include <QFileInfo>
#  include <QRegularExpression>
#endif
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

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(qLcEglDevDebug, "qt.qpa.egldeviceintegration")

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QEglFSDeviceIntegrationFactoryInterface_iid, "/egldeviceintegrations"_L1, Qt::CaseInsensitive))

QStringList QEglFSDeviceIntegrationFactory::keys()
{
    QStringList list;
    list.append(loader()->keyMap().values());
    qCDebug(qLcEglDevDebug) << "EGL device integration plugin keys:" << list;
    return list;
}

QEglFSDeviceIntegration *QEglFSDeviceIntegrationFactory::create(const QString &key)
{
    QEglFSDeviceIntegration *integration = nullptr;
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
    QRegularExpression fbIndexRx("fb(\\d+)"_L1);
    QFileInfo fbinfo(QString::fromLocal8Bit(fbDeviceName()));
    QRegularExpressionMatch match;
    if (fbinfo.isSymLink())
        match = fbIndexRx.match(fbinfo.symLinkTarget());
    else
        match = fbIndexRx.match(fbinfo.fileName());
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
    bool displayOk;
    const int defaultDisplay = qEnvironmentVariableIntValue("QT_QPA_EGLFS_DEFAULT_DISPLAY", &displayOk);
    return displayOk ? EGLNativeDisplayType(quintptr(defaultDisplay)) : EGL_DEFAULT_DISPLAY;
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
    while (!app->screens().isEmpty())
        QWindowSystemInterface::handleScreenRemoved(app->screens().constLast()->handle());
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
    return QDpi(100, 100);
}

QDpi QEglFSDeviceIntegration::logicalBaseDpi() const
{
    return QDpi(100, 100);
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

#include "moc_qeglfsdeviceintegration_p.cpp"
