/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfskmsintegration.h"
#include "qeglfskmsdevice.h"
#include "qeglfskmsscreen.h"
#include "qeglfskmscursor.h"
#include "qeglfscursor.h"

#include <QtPlatformSupport/private/qdevicediscovery_p.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformcursor.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

QMutex QEglFSKmsScreen::m_waitForFlipMutex;

QEglFSKmsIntegration::QEglFSKmsIntegration()
    : m_device(Q_NULLPTR)
    , m_hwCursor(true)
    , m_pbuffers(false)
    , m_separateScreens(false)
{}

void QEglFSKmsIntegration::platformInit()
{
    loadConfig();

    if (!m_devicePath.isEmpty()) {
        qCDebug(qLcEglfsKmsDebug) << "Using DRM device" << m_devicePath << "specified in config file";
    } else {

        QDeviceDiscovery *d = QDeviceDiscovery::create(QDeviceDiscovery::Device_VideoMask);
        QStringList devices = d->scanConnectedDevices();
        qCDebug(qLcEglfsKmsDebug) << "Found the following video devices:" << devices;
        d->deleteLater();

        if (devices.isEmpty())
            qFatal("Could not find DRM device!");

        m_devicePath = devices.first();
        qCDebug(qLcEglfsKmsDebug) << "Using" << m_devicePath;
    }

    m_device = new QEglFSKmsDevice(this, m_devicePath);
    if (!m_device->open())
        qFatal("Could not open device %s - aborting!", qPrintable(m_devicePath));
}

void QEglFSKmsIntegration::platformDestroy()
{
    m_device->close();
    delete m_device;
    m_device = Q_NULLPTR;
}

EGLNativeDisplayType QEglFSKmsIntegration::platformDisplay() const
{
    Q_ASSERT(m_device);
    return reinterpret_cast<EGLNativeDisplayType>(m_device->device());
}

bool QEglFSKmsIntegration::usesDefaultScreen()
{
    return false;
}

void QEglFSKmsIntegration::screenInit()
{
    m_device->createScreens();
}

QSurfaceFormat QEglFSKmsIntegration::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat format(inputFormat);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    return format;
}

EGLNativeWindowType QEglFSKmsIntegration::createNativeWindow(QPlatformWindow *platformWindow,
                                                     const QSize &size,
                                                     const QSurfaceFormat &format)
{
    Q_UNUSED(size);
    Q_UNUSED(format);

    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(platformWindow->screen());
    if (screen->surface()) {
        qWarning("Only single window per screen supported!");
        return 0;
    }

    return reinterpret_cast<EGLNativeWindowType>(screen->createSurface());
}

EGLNativeWindowType QEglFSKmsIntegration::createNativeOffscreenWindow(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    Q_ASSERT(m_device);

    qCDebug(qLcEglfsKmsDebug) << "Creating native off screen window";
    gbm_surface *surface = gbm_surface_create(m_device->device(),
                                              1, 1,
                                              GBM_FORMAT_XRGB8888,
                                              GBM_BO_USE_RENDERING);

    return reinterpret_cast<EGLNativeWindowType>(surface);
}

void QEglFSKmsIntegration::destroyNativeWindow(EGLNativeWindowType window)
{
    gbm_surface *surface = reinterpret_cast<gbm_surface *>(window);
    gbm_surface_destroy(surface);
}

bool QEglFSKmsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case QPlatformIntegration::ThreadedPixmaps:
    case QPlatformIntegration::OpenGL:
    case QPlatformIntegration::ThreadedOpenGL:
        return true;
    default:
        return false;
    }
}

QPlatformCursor *QEglFSKmsIntegration::createCursor(QPlatformScreen *screen) const
{
    if (m_hwCursor)
        return Q_NULLPTR;
    else
        return new QEglFSCursor(screen);
}

void QEglFSKmsIntegration::waitForVSync(QPlatformSurface *surface) const
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->waitForFlip();
}

void QEglFSKmsIntegration::presentBuffer(QPlatformSurface *surface)
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->flip();
}

bool QEglFSKmsIntegration::supportsPBuffers() const
{
    return m_pbuffers;
}

bool QEglFSKmsIntegration::hwCursor() const
{
    return m_hwCursor;
}

bool QEglFSKmsIntegration::separateScreens() const
{
    return m_separateScreens;
}

QMap<QString, QVariantMap> QEglFSKmsIntegration::outputSettings() const
{
    return m_outputSettings;
}

void QEglFSKmsIntegration::loadConfig()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_KMS_CONFIG");
    if (json.isEmpty())
        return;

    qCDebug(qLcEglfsKmsDebug) << "Loading KMS setup from" << json;

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        qCDebug(qLcEglfsKmsDebug) << "Could not open config file"
                                  << json << "for reading";
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qCDebug(qLcEglfsKmsDebug) << "Invalid config file" << json
                                  << "- no top-level JSON object";
        return;
    }

    const QJsonObject object = doc.object();

    m_hwCursor = object.value(QStringLiteral("hwcursor")).toBool(m_hwCursor);
    m_pbuffers = object.value(QStringLiteral("pbuffers")).toBool(m_pbuffers);
    m_devicePath = object.value(QStringLiteral("device")).toString();
    m_separateScreens = object.value(QStringLiteral("separateScreens")).toBool(m_separateScreens);

    const QJsonArray outputs = object.value(QStringLiteral("outputs")).toArray();
    for (int i = 0; i < outputs.size(); i++) {
        const QVariantMap outputSettings = outputs.at(i).toObject().toVariantMap();

        if (outputSettings.contains(QStringLiteral("name"))) {
            const QString name = outputSettings.value(QStringLiteral("name")).toString();

            if (m_outputSettings.contains(name)) {
                qCDebug(qLcEglfsKmsDebug) << "Output" << name << "configured multiple times!";
            }

            m_outputSettings.insert(name, outputSettings);
        }
    }

    qCDebug(qLcEglfsKmsDebug) << "Configuration:\n"
                              << "\thwcursor:" << m_hwCursor << "\n"
                              << "\tpbuffers:" << m_pbuffers << "\n"
                              << "\tseparateScreens:" << m_separateScreens << "\n"
                              << "\toutputs:" << m_outputSettings;
}

QT_END_NAMESPACE
