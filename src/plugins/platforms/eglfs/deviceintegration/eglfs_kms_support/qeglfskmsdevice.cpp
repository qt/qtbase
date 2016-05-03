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

#include "qeglfskmsdevice.h"
#include "qeglfskmsscreen.h"

#include "qeglfsintegration.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>
#include <QtGui/private/qguiapplication_p.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

enum OutputConfiguration {
    OutputConfigOff,
    OutputConfigPreferred,
    OutputConfigCurrent,
    OutputConfigMode,
    OutputConfigModeline
};

int QEglFSKmsDevice::crtcForConnector(drmModeResPtr resources, drmModeConnectorPtr connector)
{
    for (int i = 0; i < connector->count_encoders; i++) {
        drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->encoders[i]);
        if (!encoder) {
            qWarning("Failed to get encoder");
            continue;
        }

        quint32 possibleCrtcs = encoder->possible_crtcs;
        drmModeFreeEncoder(encoder);

        for (int j = 0; j < resources->count_crtcs; j++) {
            bool isPossible = possibleCrtcs & (1 << j);
            bool isAvailable = !(m_crtc_allocator & 1 << resources->crtcs[j]);

            if (isPossible && isAvailable)
                return j;
        }
    }

    return -1;
}

static const char * const connector_type_names[] = {
    "None",
    "VGA",
    "DVI",
    "DVI",
    "DVI",
    "Composite",
    "TV",
    "LVDS",
    "CTV",
    "DIN",
    "DP",
    "HDMI",
    "HDMI",
    "TV",
    "eDP",
};

static QByteArray nameForConnector(const drmModeConnectorPtr connector)
{
    QByteArray connectorName("UNKNOWN");

    if (connector->connector_type < ARRAY_LENGTH(connector_type_names))
        connectorName = connector_type_names[connector->connector_type];

    connectorName += QByteArray::number(connector->connector_type_id);

    return connectorName;
}

static bool parseModeline(const QByteArray &text, drmModeModeInfoPtr mode)
{
    char hsync[16];
    char vsync[16];
    float fclock;

    mode->type = DRM_MODE_TYPE_USERDEF;
    mode->hskew = 0;
    mode->vscan = 0;
    mode->vrefresh = 0;
    mode->flags = 0;

    if (sscanf(text.constData(), "%f %hd %hd %hd %hd %hd %hd %hd %hd %15s %15s",
               &fclock,
               &mode->hdisplay,
               &mode->hsync_start,
               &mode->hsync_end,
               &mode->htotal,
               &mode->vdisplay,
               &mode->vsync_start,
               &mode->vsync_end,
               &mode->vtotal, hsync, vsync) != 11)
        return false;

    mode->clock = fclock * 1000;

    if (strcmp(hsync, "+hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PHSYNC;
    else if (strcmp(hsync, "-hsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NHSYNC;
    else
        return false;

    if (strcmp(vsync, "+vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_PVSYNC;
    else if (strcmp(vsync, "-vsync") == 0)
        mode->flags |= DRM_MODE_FLAG_NVSYNC;
    else
        return false;

    return true;
}

QEglFSKmsScreen *QEglFSKmsDevice::screenForConnector(drmModeResPtr resources, drmModeConnectorPtr connector, QPoint pos)
{
    const QByteArray connectorName = nameForConnector(connector);

    const int crtc = crtcForConnector(resources, connector);
    if (crtc < 0) {
        qWarning() << "No usable crtc/encoder pair for connector" << connectorName;
        return Q_NULLPTR;
    }

    OutputConfiguration configuration;
    QSize configurationSize;
    drmModeModeInfo configurationModeline;

    const QByteArray mode = m_integration->outputSettings().value(QString::fromUtf8(connectorName))
            .value(QStringLiteral("mode"), QStringLiteral("preferred")).toByteArray().toLower();
    if (mode == "off") {
        configuration = OutputConfigOff;
    } else if (mode == "preferred") {
        configuration = OutputConfigPreferred;
    } else if (mode == "current") {
        configuration = OutputConfigCurrent;
    } else if (sscanf(mode.constData(), "%dx%d", &configurationSize.rwidth(), &configurationSize.rheight()) == 2) {
        configuration = OutputConfigMode;
    } else if (parseModeline(mode, &configurationModeline)) {
        configuration = OutputConfigModeline;
    } else {
        qWarning("Invalid mode \"%s\" for output %s", mode.constData(), connectorName.constData());
        configuration = OutputConfigPreferred;
    }

    const uint32_t crtc_id = resources->crtcs[crtc];

    if (configuration == OutputConfigOff) {
        qCDebug(qLcEglfsKmsDebug) << "Turning off output" << connectorName;
        drmModeSetCrtc(m_dri_fd, crtc_id, 0, 0, 0, 0, 0, Q_NULLPTR);
        return Q_NULLPTR;
    }

    // Skip disconnected output
    if (configuration == OutputConfigPreferred && connector->connection == DRM_MODE_DISCONNECTED) {
        qCDebug(qLcEglfsKmsDebug) << "Skipping disconnected output" << connectorName;
        return Q_NULLPTR;
    }

    // Get the current mode on the current crtc
    drmModeModeInfo crtc_mode;
    memset(&crtc_mode, 0, sizeof crtc_mode);
    if (drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->connector_id)) {
        drmModeCrtcPtr crtc = drmModeGetCrtc(m_dri_fd, encoder->crtc_id);
        drmModeFreeEncoder(encoder);

        if (!crtc)
            return Q_NULLPTR;

        if (crtc->mode_valid)
            crtc_mode = crtc->mode;

        drmModeFreeCrtc(crtc);
    }

    QList<drmModeModeInfo> modes;
    modes.reserve(connector->count_modes);
    qCDebug(qLcEglfsKmsDebug) << connectorName << "mode count:" << connector->count_modes;
    for (int i = 0; i < connector->count_modes; i++) {
        const drmModeModeInfo &mode = connector->modes[i];
        qCDebug(qLcEglfsKmsDebug) << "mode" << i << mode.hdisplay << "x" << mode.vdisplay
                                  << '@' << mode.vrefresh << "hz";
        modes << connector->modes[i];
    }

    int preferred = -1;
    int current = -1;
    int configured = -1;
    int best = -1;

    for (int i = modes.size() - 1; i >= 0; i--) {
        const drmModeModeInfo &m = modes.at(i);

        if (configuration == OutputConfigMode &&
                m.hdisplay == configurationSize.width() &&
                m.vdisplay == configurationSize.height()) {
            configured = i;
        }

        if (!memcmp(&crtc_mode, &m, sizeof m))
            current = i;

        if (m.type & DRM_MODE_TYPE_PREFERRED)
            preferred = i;

        best = i;
    }

    if (configuration == OutputConfigModeline) {
        modes << configurationModeline;
        configured = modes.size() - 1;
    }

    if (current < 0 && crtc_mode.clock != 0) {
        modes << crtc_mode;
        current = mode.size() - 1;
    }

    if (configuration == OutputConfigCurrent)
        configured = current;

    int selected_mode = -1;

    if (configured >= 0)
        selected_mode = configured;
    else if (preferred >= 0)
        selected_mode = preferred;
    else if (current >= 0)
        selected_mode = current;
    else if (best >= 0)
        selected_mode = best;

    if (selected_mode < 0) {
        qWarning() << "No modes available for output" << connectorName;
        return Q_NULLPTR;
    } else {
        int width = modes[selected_mode].hdisplay;
        int height = modes[selected_mode].vdisplay;
        int refresh = modes[selected_mode].vrefresh;
        qCDebug(qLcEglfsKmsDebug) << "Selected mode" << selected_mode << ":" << width << "x" << height
                                  << '@' << refresh << "hz for output" << connectorName;
    }
    static const int width = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_WIDTH");
    static const int height = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_HEIGHT");
    QSizeF size(width, height);
    if (size.isEmpty()) {
        size.setWidth(connector->mmWidth);
        size.setHeight(connector->mmHeight);
    }
    QEglFSKmsOutput output = {
        QString::fromUtf8(connectorName),
        connector->connector_id,
        crtc_id,
        size,
        selected_mode,
        false,
        drmModeGetCrtc(m_dri_fd, crtc_id),
        modes,
        connector->subpixel,
        connectorProperty(connector, QByteArrayLiteral("DPMS"))
    };

    m_crtc_allocator |= (1 << output.crtc_id);
    m_connector_allocator |= (1 << output.connector_id);

    return createScreen(m_integration, this, output, pos);
}

drmModePropertyPtr QEglFSKmsDevice::connectorProperty(drmModeConnectorPtr connector, const QByteArray &name)
{
    drmModePropertyPtr prop;

    for (int i = 0; i < connector->count_props; i++) {
        prop = drmModeGetProperty(m_dri_fd, connector->props[i]);
        if (!prop)
            continue;
        if (strcmp(prop->name, name.constData()) == 0)
            return prop;
        drmModeFreeProperty(prop);
    }

    return Q_NULLPTR;
}

QEglFSKmsDevice::QEglFSKmsDevice(QEglFSKmsIntegration *integration, const QString &path)
    : m_integration(integration)
    , m_path(path)
    , m_dri_fd(-1)
    , m_crtc_allocator(0)
    , m_connector_allocator(0)
{
}

QEglFSKmsDevice::~QEglFSKmsDevice()
{
}

void QEglFSKmsDevice::createScreens()
{
    drmModeResPtr resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
        qWarning("drmModeGetResources failed");
        return;
    }

    QEglFSKmsScreen *primaryScreen = Q_NULLPTR;
    QList<QPlatformScreen *> siblings;
    QPoint pos(0, 0);
    QEglFSIntegration *integration = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnectorPtr connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        QEglFSKmsScreen *screen = screenForConnector(resources, connector, pos);
        if (screen) {
            integration->addScreen(screen);
            pos.rx() += screen->geometry().width();
            siblings << screen;

            if (!primaryScreen)
                primaryScreen = screen;
        }

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);

    if (!m_integration->separateScreens()) {
        Q_FOREACH (QPlatformScreen *screen, siblings)
            static_cast<QEglFSKmsScreen *>(screen)->setVirtualSiblings(siblings);
    }
}

int QEglFSKmsDevice::fd() const
{
    return m_dri_fd;
}

QString QEglFSKmsDevice::devicePath() const
{
    return m_path;
}

QEglFSKmsScreen *QEglFSKmsDevice::createScreen(QEglFSKmsIntegration *integration, QEglFSKmsDevice *device, QEglFSKmsOutput output, QPoint position)
{
    return new QEglFSKmsScreen(integration, device, output, position);
}

void QEglFSKmsDevice::setFd(int fd)
{
    m_dri_fd = fd;
}

QT_END_NAMESPACE
