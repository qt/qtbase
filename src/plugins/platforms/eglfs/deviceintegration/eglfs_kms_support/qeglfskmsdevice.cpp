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

#include "qeglfsintegration_p.h"

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

static const char * const connector_type_names[] = { // must match DRM_MODE_CONNECTOR_*
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
    "Virtual",
    "DSI"
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

QEglFSKmsScreen *QEglFSKmsDevice::createScreenForConnector(drmModeResPtr resources,
                                                           drmModeConnectorPtr connector,
                                                           VirtualDesktopInfo *vinfo)
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

    auto userConfig = m_integration->outputSettings();
    auto userConnectorConfig = userConfig.value(QString::fromUtf8(connectorName));
    // default to the preferred mode unless overridden in the config
    const QByteArray mode = userConnectorConfig.value(QStringLiteral("mode"), QStringLiteral("preferred"))
        .toByteArray().toLower();
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
    if (vinfo) {
        *vinfo = VirtualDesktopInfo();
        vinfo->virtualIndex = userConnectorConfig.value(QStringLiteral("virtualIndex"), INT_MAX).toInt();
        if (userConnectorConfig.contains(QStringLiteral("virtualPos"))) {
            const QByteArray vpos = userConnectorConfig.value(QStringLiteral("virtualPos")).toByteArray();
            const QByteArrayList vposComp = vpos.split(',');
            if (vposComp.count() == 2)
                vinfo->virtualPos = QPoint(vposComp[0].trimmed().toInt(), vposComp[1].trimmed().toInt());
        }
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

    // physical size from connector < config values < env vars
    static const int width = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_WIDTH");
    static const int height = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_HEIGHT");
    QSizeF physSize(width, height);
    if (physSize.isEmpty()) {
        physSize = QSize(userConnectorConfig.value(QStringLiteral("physicalWidth")).toInt(),
                         userConnectorConfig.value(QStringLiteral("physicalHeight")).toInt());
        if (physSize.isEmpty()) {
            physSize.setWidth(connector->mmWidth);
            physSize.setHeight(connector->mmHeight);
        }
    }
    qCDebug(qLcEglfsKmsDebug) << "Physical size is" << physSize << "mm" << "for output" << connectorName;

    QEglFSKmsOutput output = {
        QString::fromUtf8(connectorName),
        connector->connector_id,
        crtc_id,
        physSize,
        selected_mode,
        false,
        drmModeGetCrtc(m_dri_fd, crtc_id),
        modes,
        connector->subpixel,
        connectorProperty(connector, QByteArrayLiteral("DPMS"))
    };

    m_crtc_allocator |= (1 << output.crtc_id);
    m_connector_allocator |= (1 << output.connector_id);

    return createScreen(m_integration, this, output);
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

struct OrderedScreen
{
    OrderedScreen() : screen(nullptr) { }
    OrderedScreen(QEglFSKmsScreen *screen, const QEglFSKmsDevice::VirtualDesktopInfo &vinfo)
        : screen(screen), vinfo(vinfo) { }
    QEglFSKmsScreen *screen;
    QEglFSKmsDevice::VirtualDesktopInfo vinfo;
};

QDebug operator<<(QDebug dbg, const OrderedScreen &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "OrderedScreen(" << s.screen << " : " << s.vinfo.virtualIndex
                  << " / " << s.vinfo.virtualPos << ")";
    return dbg;
}

static bool orderedScreenLessThan(const OrderedScreen &a, const OrderedScreen &b)
{
    return a.vinfo.virtualIndex < b.vinfo.virtualIndex;
}

void QEglFSKmsDevice::createScreens()
{
    drmModeResPtr resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
        qWarning("drmModeGetResources failed");
        return;
    }

    QVector<OrderedScreen> screens;

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnectorPtr connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        VirtualDesktopInfo vinfo;
        QEglFSKmsScreen *screen = createScreenForConnector(resources, connector, &vinfo);
        if (screen)
            screens.append(OrderedScreen(screen, vinfo));

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);

    // Use stable sort to preserve the original order for outputs with unspecified indices.
    std::stable_sort(screens.begin(), screens.end(), orderedScreenLessThan);
    qCDebug(qLcEglfsKmsDebug) << "Sorted screen list:" << screens;

    QPoint pos(0, 0);
    QList<QPlatformScreen *> siblings;
    QEglFSIntegration *qpaIntegration = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());

    for (const OrderedScreen &orderedScreen : screens) {
        QEglFSKmsScreen *s = orderedScreen.screen;
        // set up a horizontal or vertical virtual desktop
        if (orderedScreen.vinfo.virtualPos.isNull()) {
            s->setVirtualPosition(pos);
            if (m_integration->virtualDesktopLayout() == QEglFSKmsIntegration::VirtualDesktopLayoutVertical)
                pos.ry() += s->geometry().height();
            else
                pos.rx() += s->geometry().width();
        } else {
            s->setVirtualPosition(orderedScreen.vinfo.virtualPos);
        }
        qCDebug(qLcEglfsKmsDebug) << "Adding screen" << s << "to QPA with geometry" << s->geometry();
        // The order in qguiapp's screens list will match the order set by
        // virtualIndex. This is not only handy but also required since for instance
        // evdevtouch relies on it when performing touch device - screen mapping.
        qpaIntegration->addScreen(s);
        siblings << s;
    }

    if (!m_integration->separateScreens()) {
        // enable the virtual desktop
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

QEglFSKmsScreen *QEglFSKmsDevice::createScreen(QEglFSKmsIntegration *integration, QEglFSKmsDevice *device, QEglFSKmsOutput output)
{
    return new QEglFSKmsScreen(integration, device, output);
}

void QEglFSKmsDevice::setFd(int fd)
{
    m_dri_fd = fd;
}

QT_END_NAMESPACE
