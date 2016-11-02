/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#include "qeglfskmsintegration.h"
#include "qeglfskmsdevice.h"
#include "qeglfskmsscreen.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/QScreen>

#include <xf86drm.h>
#include <xf86drmMode.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEglfsKmsDebug, "qt.qpa.eglfs.kms")

QEglFSKmsIntegration::QEglFSKmsIntegration()
    : m_device(Q_NULLPTR)
    , m_hwCursor(false)
    , m_pbuffers(false)
    , m_separateScreens(false)
    , m_virtualDesktopLayout(VirtualDesktopLayoutHorizontal)
{}

void QEglFSKmsIntegration::platformInit()
{
    loadConfig();

    if (!m_devicePath.isEmpty()) {
        qCDebug(qLcEglfsKmsDebug) << "Using DRM device" << m_devicePath << "specified in config file";
    }

    m_device = createDevice(m_devicePath);
    if (Q_UNLIKELY(!m_device->open()))
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
    return m_device->nativeDisplay();
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

void QEglFSKmsIntegration::waitForVSync(QPlatformSurface *surface) const
{
    QWindow *window = static_cast<QWindow *>(surface->surface());
    QEglFSKmsScreen *screen = static_cast<QEglFSKmsScreen *>(window->screen()->handle());

    screen->waitForFlip();
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

QEglFSKmsIntegration::VirtualDesktopLayout QEglFSKmsIntegration::virtualDesktopLayout() const
{
    return m_virtualDesktopLayout;
}

QMap<QString, QVariantMap> QEglFSKmsIntegration::outputSettings() const
{
    return m_outputSettings;
}

QEglFSKmsDevice *QEglFSKmsIntegration::device() const
{
    return m_device;
}

void QEglFSKmsIntegration::loadConfig()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_KMS_CONFIG");
    if (json.isEmpty())
        return;

    qCDebug(qLcEglfsKmsDebug) << "Loading KMS setup from" << json;

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(qLcEglfsKmsDebug) << "Could not open config file"
                                    << json << "for reading";
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qCWarning(qLcEglfsKmsDebug) << "Invalid config file" << json
                                    << "- no top-level JSON object";
        return;
    }

    const QJsonObject object = doc.object();

    m_hwCursor = object.value(QLatin1String("hwcursor")).toBool(m_hwCursor);
    m_pbuffers = object.value(QLatin1String("pbuffers")).toBool(m_pbuffers);
    m_devicePath = object.value(QLatin1String("device")).toString();
    m_separateScreens = object.value(QLatin1String("separateScreens")).toBool(m_separateScreens);

    const QString vdOriString = object.value(QLatin1String("virtualDesktopLayout")).toString();
    if (!vdOriString.isEmpty()) {
        if (vdOriString == QLatin1String("horizontal"))
            m_virtualDesktopLayout = VirtualDesktopLayoutHorizontal;
        else if (vdOriString == QLatin1String("vertical"))
            m_virtualDesktopLayout = VirtualDesktopLayoutVertical;
        else
            qCWarning(qLcEglfsKmsDebug) << "Unknown virtualDesktopOrientation value" << vdOriString;
    }

    const QJsonArray outputs = object.value(QLatin1String("outputs")).toArray();
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
                              << "\tvirtualDesktopLayout:" << m_virtualDesktopLayout << "\n"
                              << "\toutputs:" << m_outputSettings;
}

QT_END_NAMESPACE
