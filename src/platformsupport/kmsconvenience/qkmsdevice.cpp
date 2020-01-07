/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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

#include "qkmsdevice_p.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <errno.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcKmsDebug, "qt.qpa.eglfs.kms")

enum OutputConfiguration {
    OutputConfigOff,
    OutputConfigPreferred,
    OutputConfigCurrent,
    OutputConfigSkip,
    OutputConfigMode,
    OutputConfigModeline
};

int QKmsDevice::crtcForConnector(drmModeResPtr resources, drmModeConnectorPtr connector)
{
    int candidate = -1;

    for (int i = 0; i < connector->count_encoders; i++) {
        drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->encoders[i]);
        if (!encoder) {
            qWarning("Failed to get encoder");
            continue;
        }

        quint32 encoderId = encoder->encoder_id;
        quint32 crtcId = encoder->crtc_id;
        quint32 possibleCrtcs = encoder->possible_crtcs;
        drmModeFreeEncoder(encoder);

        for (int j = 0; j < resources->count_crtcs; j++) {
            bool isPossible = possibleCrtcs & (1 << j);
            bool isAvailable = !(m_crtc_allocator & (1 << j));
            // Preserve the existing CRTC -> encoder -> connector routing if
            // any. It makes the initialization faster, and may be better
            // since we have a very dumb picking algorithm.
            bool isBestChoice = (!connector->encoder_id ||
                                 (connector->encoder_id == encoderId &&
                                  resources->crtcs[j] == crtcId));

            if (isPossible && isAvailable && isBestChoice) {
                return j;
            } else if (isPossible && isAvailable) {
                candidate = j;
            }
        }
    }

    return candidate;
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

static inline void assignPlane(QKmsOutput *output, QKmsPlane *plane)
{
    if (output->eglfs_plane)
        output->eglfs_plane->activeCrtcId = 0;

    plane->activeCrtcId = output->crtc_id;
    output->eglfs_plane = plane;
}

QPlatformScreen *QKmsDevice::createScreenForConnector(drmModeResPtr resources,
                                                      drmModeConnectorPtr connector,
                                                      ScreenInfo *vinfo)
{
    Q_ASSERT(vinfo);
    const QByteArray connectorName = nameForConnector(connector);

    const int crtc = crtcForConnector(resources, connector);
    if (crtc < 0) {
        qWarning() << "No usable crtc/encoder pair for connector" << connectorName;
        return nullptr;
    }

    OutputConfiguration configuration;
    QSize configurationSize;
    int configurationRefresh = 0;
    drmModeModeInfo configurationModeline;

    auto userConfig = m_screenConfig->outputSettings();
    QVariantMap userConnectorConfig = userConfig.value(QString::fromUtf8(connectorName));
    // default to the preferred mode unless overridden in the config
    const QByteArray mode = userConnectorConfig.value(QStringLiteral("mode"), QStringLiteral("preferred"))
        .toByteArray().toLower();
    if (mode == "off") {
        configuration = OutputConfigOff;
    } else if (mode == "preferred") {
        configuration = OutputConfigPreferred;
    } else if (mode == "current") {
        configuration = OutputConfigCurrent;
    } else if (mode == "skip") {
        configuration = OutputConfigSkip;
    } else if (sscanf(mode.constData(), "%dx%d@%d", &configurationSize.rwidth(), &configurationSize.rheight(),
                      &configurationRefresh) == 3)
    {
        configuration = OutputConfigMode;
    } else if (sscanf(mode.constData(), "%dx%d", &configurationSize.rwidth(), &configurationSize.rheight()) == 2) {
        configuration = OutputConfigMode;
    } else if (parseModeline(mode, &configurationModeline)) {
        configuration = OutputConfigModeline;
    } else {
        qWarning("Invalid mode \"%s\" for output %s", mode.constData(), connectorName.constData());
        configuration = OutputConfigPreferred;
    }

    *vinfo = ScreenInfo();
    vinfo->virtualIndex = userConnectorConfig.value(QStringLiteral("virtualIndex"), INT_MAX).toInt();
    if (userConnectorConfig.contains(QStringLiteral("virtualPos"))) {
        const QByteArray vpos = userConnectorConfig.value(QStringLiteral("virtualPos")).toByteArray();
        const QByteArrayList vposComp = vpos.split(',');
        if (vposComp.count() == 2)
            vinfo->virtualPos = QPoint(vposComp[0].trimmed().toInt(), vposComp[1].trimmed().toInt());
    }
    if (userConnectorConfig.value(QStringLiteral("primary")).toBool())
        vinfo->isPrimary = true;

    const uint32_t crtc_id = resources->crtcs[crtc];

    if (configuration == OutputConfigOff) {
        qCDebug(qLcKmsDebug) << "Turning off output" << connectorName;
        drmModeSetCrtc(m_dri_fd, crtc_id, 0, 0, 0, 0, 0, nullptr);
        return nullptr;
    }

    // Skip disconnected output
    if (configuration == OutputConfigPreferred && connector->connection == DRM_MODE_DISCONNECTED) {
        qCDebug(qLcKmsDebug) << "Skipping disconnected output" << connectorName;
        return nullptr;
    }

    if (configuration == OutputConfigSkip) {
        qCDebug(qLcKmsDebug) << "Skipping output" << connectorName;
        return nullptr;
    }

    // Get the current mode on the current crtc
    drmModeModeInfo crtc_mode;
    memset(&crtc_mode, 0, sizeof crtc_mode);
    if (drmModeEncoderPtr encoder = drmModeGetEncoder(m_dri_fd, connector->encoder_id)) {
        drmModeCrtcPtr crtc = drmModeGetCrtc(m_dri_fd, encoder->crtc_id);
        drmModeFreeEncoder(encoder);

        if (!crtc)
            return nullptr;

        if (crtc->mode_valid)
            crtc_mode = crtc->mode;

        drmModeFreeCrtc(crtc);
    }

    QList<drmModeModeInfo> modes;
    modes.reserve(connector->count_modes);
    qCDebug(qLcKmsDebug) << connectorName << "mode count:" << connector->count_modes
                         << "crtc index:" << crtc << "crtc id:" << crtc_id;
    for (int i = 0; i < connector->count_modes; i++) {
        const drmModeModeInfo &mode = connector->modes[i];
        qCDebug(qLcKmsDebug) << "mode" << i << mode.hdisplay << "x" << mode.vdisplay
                                  << '@' << mode.vrefresh << "hz";
        modes << connector->modes[i];
    }

    int preferred = -1;
    int current = -1;
    int configured = -1;
    int best = -1;

    for (int i = modes.size() - 1; i >= 0; i--) {
        const drmModeModeInfo &m = modes.at(i);

        if (configuration == OutputConfigMode
                && m.hdisplay == configurationSize.width()
                && m.vdisplay == configurationSize.height()
                && (!configurationRefresh || m.vrefresh == uint32_t(configurationRefresh)))
        {
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
        return nullptr;
    } else {
        int width = modes[selected_mode].hdisplay;
        int height = modes[selected_mode].vdisplay;
        int refresh = modes[selected_mode].vrefresh;
        qCDebug(qLcKmsDebug) << "Selected mode" << selected_mode << ":" << width << "x" << height
                                  << '@' << refresh << "hz for output" << connectorName;
    }

    // physical size from connector < config values < env vars
    int pwidth = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_WIDTH");
    if (!pwidth)
        pwidth = qEnvironmentVariableIntValue("QT_QPA_PHYSICAL_WIDTH");
    int pheight = qEnvironmentVariableIntValue("QT_QPA_EGLFS_PHYSICAL_HEIGHT");
    if (!pheight)
        pheight = qEnvironmentVariableIntValue("QT_QPA_PHYSICAL_HEIGHT");
    QSizeF physSize(pwidth, pheight);
    if (physSize.isEmpty()) {
        physSize = QSize(userConnectorConfig.value(QStringLiteral("physicalWidth")).toInt(),
                         userConnectorConfig.value(QStringLiteral("physicalHeight")).toInt());
        if (physSize.isEmpty()) {
            physSize.setWidth(connector->mmWidth);
            physSize.setHeight(connector->mmHeight);
        }
    }
    qCDebug(qLcKmsDebug) << "Physical size is" << physSize << "mm" << "for output" << connectorName;

    const QByteArray formatStr = userConnectorConfig.value(QStringLiteral("format"), QString())
            .toByteArray().toLower();
    uint32_t drmFormat;
    bool drmFormatExplicit = true;
    if (formatStr.isEmpty()) {
        drmFormat = DRM_FORMAT_XRGB8888;
        drmFormatExplicit = false;
    } else if (formatStr == "xrgb8888") {
        drmFormat = DRM_FORMAT_XRGB8888;
    } else if (formatStr == "xbgr8888") {
        drmFormat = DRM_FORMAT_XBGR8888;
    } else if (formatStr == "argb8888") {
        drmFormat = DRM_FORMAT_ARGB8888;
    } else if (formatStr == "abgr8888") {
        drmFormat = DRM_FORMAT_ABGR8888;
    } else if (formatStr == "rgb565") {
        drmFormat = DRM_FORMAT_RGB565;
    } else if (formatStr == "bgr565") {
        drmFormat = DRM_FORMAT_BGR565;
    } else if (formatStr == "xrgb2101010") {
        drmFormat = DRM_FORMAT_XRGB2101010;
    } else if (formatStr == "xbgr2101010") {
        drmFormat = DRM_FORMAT_XBGR2101010;
    } else if (formatStr == "argb2101010") {
        drmFormat = DRM_FORMAT_ARGB2101010;
    } else if (formatStr == "abgr2101010") {
        drmFormat = DRM_FORMAT_ABGR2101010;
    } else {
        qWarning("Invalid pixel format \"%s\" for output %s", formatStr.constData(), connectorName.constData());
        drmFormat = DRM_FORMAT_XRGB8888;
        drmFormatExplicit = false;
    }
    qCDebug(qLcKmsDebug) << "Format is" << Qt::hex << drmFormat << Qt::dec << "requested_by_user =" << drmFormatExplicit
                         << "for output" << connectorName;

    const QString cloneSource = userConnectorConfig.value(QStringLiteral("clones")).toString();
    if (!cloneSource.isEmpty())
        qCDebug(qLcKmsDebug) << "Output" << connectorName << " clones output " << cloneSource;

    QSize framebufferSize;
    bool framebufferSizeSet = false;
    const QByteArray fbsize = userConnectorConfig.value(QStringLiteral("size")).toByteArray().toLower();
    if (!fbsize.isEmpty()) {
        if (sscanf(fbsize.constData(), "%dx%d", &framebufferSize.rwidth(), &framebufferSize.rheight()) == 2) {
#if QT_CONFIG(drm_atomic)
            if (hasAtomicSupport())
                framebufferSizeSet = true;
#endif
            if (!framebufferSizeSet)
                qWarning("Setting framebuffer size is only available with DRM atomic API");
        } else {
            qWarning("Invalid framebuffer size '%s'", fbsize.constData());
        }
    }
    if (!framebufferSizeSet) {
        framebufferSize.setWidth(modes[selected_mode].hdisplay);
        framebufferSize.setHeight(modes[selected_mode].vdisplay);
    }

    qCDebug(qLcKmsDebug) << "Output" << connectorName << "framebuffer size is " << framebufferSize;

    QKmsOutput output;
    output.name = QString::fromUtf8(connectorName);
    output.connector_id = connector->connector_id;
    output.crtc_index = crtc;
    output.crtc_id = crtc_id;
    output.physical_size = physSize;
    output.preferred_mode = preferred >= 0 ? preferred : selected_mode;
    output.mode = selected_mode;
    output.mode_set = false;
    output.saved_crtc = drmModeGetCrtc(m_dri_fd, crtc_id);
    output.modes = modes;
    output.subpixel = connector->subpixel;
    output.dpms_prop = connectorProperty(connector, QByteArrayLiteral("DPMS"));
    output.edid_blob = connectorPropertyBlob(connector, QByteArrayLiteral("EDID"));
    output.wants_forced_plane = false;
    output.forced_plane_id = 0;
    output.forced_plane_set = false;
    output.drm_format = drmFormat;
    output.drm_format_requested_by_user = drmFormatExplicit;
    output.clone_source = cloneSource;
    output.size = framebufferSize;

#if QT_CONFIG(drm_atomic)
    if (drmModeCreatePropertyBlob(m_dri_fd, &modes[selected_mode], sizeof(drmModeModeInfo),
                                  &output.mode_blob_id) != 0) {
        qCDebug(qLcKmsDebug) << "Failed to create mode blob for mode" << selected_mode;
    }

    parseConnectorProperties(output.connector_id, &output);
    parseCrtcProperties(output.crtc_id, &output);
#endif

    QString planeListStr;
    for (QKmsPlane &plane : m_planes) {
        if (plane.possibleCrtcs & (1 << output.crtc_index)) {
            output.available_planes.append(plane);
            planeListStr.append(QString::number(plane.id));
            planeListStr.append(QLatin1Char(' '));

            // Choose the first primary plane that is not already assigned to
            // another screen's associated crtc.
            if (!output.eglfs_plane && plane.type == QKmsPlane::PrimaryPlane && !plane.activeCrtcId)
                assignPlane(&output, &plane);
        }
    }
    qCDebug(qLcKmsDebug, "Output %s can use %d planes: %s",
            connectorName.constData(), output.available_planes.count(), qPrintable(planeListStr));

    // This is for the EGLDevice/EGLStream backend. On some of those devices one
    // may want to target a pre-configured plane. It is probably useless for
    // eglfs_kms and others. Do not confuse with generic plane support (available_planes).
    bool ok;
    int idx = qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_PLANE_INDEX", &ok);
    if (ok) {
        drmModePlaneRes *planeResources = drmModeGetPlaneResources(m_dri_fd);
        if (planeResources) {
            if (idx >= 0 && idx < int(planeResources->count_planes)) {
                drmModePlane *plane = drmModeGetPlane(m_dri_fd, planeResources->planes[idx]);
                if (plane) {
                    output.wants_forced_plane = true;
                    output.forced_plane_id = plane->plane_id;
                    qCDebug(qLcKmsDebug, "Forcing plane index %d, plane id %u (belongs to crtc id %u)",
                            idx, plane->plane_id, plane->crtc_id);

                    for (QKmsPlane &kmsplane : m_planes) {
                        if (kmsplane.id == output.forced_plane_id) {
                            assignPlane(&output, &kmsplane);
                            break;
                        }
                    }

                    drmModeFreePlane(plane);
                }
            } else {
                qWarning("Invalid plane index %d, must be between 0 and %u", idx, planeResources->count_planes - 1);
            }
        }
    }

    // A more useful version: allows specifying "crtc_id,plane_id:crtc_id,plane_id:..."
    // in order to allow overriding the plane used for a given crtc.
    if (qEnvironmentVariableIsSet("QT_QPA_EGLFS_KMS_PLANES_FOR_CRTCS")) {
        const QString val = qEnvironmentVariable("QT_QPA_EGLFS_KMS_PLANES_FOR_CRTCS");
        qCDebug(qLcKmsDebug, "crtc_id:plane_id override list: %s", qPrintable(val));
        const QStringList crtcPlanePairs = val.split(QLatin1Char(':'));
        for (const QString &crtcPlanePair : crtcPlanePairs) {
            const QStringList values = crtcPlanePair.split(QLatin1Char(','));
            if (values.count() == 2 && uint(values[0].toInt()) == output.crtc_id) {
                uint planeId = values[1].toInt();
                for (QKmsPlane &kmsplane : m_planes) {
                    if (kmsplane.id == planeId) {
                        assignPlane(&output, &kmsplane);
                        break;
                    }
                }
            }
        }
    }

    if (output.eglfs_plane) {
        qCDebug(qLcKmsDebug, "Chose plane %u for output %s (crtc id %u) (may not be applicable)",
                output.eglfs_plane->id, connectorName.constData(), output.crtc_id);
    }

#if QT_CONFIG(drm_atomic)
    if (hasAtomicSupport() && !output.eglfs_plane) {
        qCDebug(qLcKmsDebug, "No plane associated with output %s (crtc id %u) and atomic modesetting is enabled. This is bad.",
                connectorName.constData(), output.crtc_id);
    }
#endif

    m_crtc_allocator |= (1 << output.crtc_index);

    vinfo->output = output;

    return createScreen(output);
}

drmModePropertyPtr QKmsDevice::connectorProperty(drmModeConnectorPtr connector, const QByteArray &name)
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

    return nullptr;
}

drmModePropertyBlobPtr QKmsDevice::connectorPropertyBlob(drmModeConnectorPtr connector, const QByteArray &name)
{
    drmModePropertyPtr prop;
    drmModePropertyBlobPtr blob = nullptr;

    for (int i = 0; i < connector->count_props && !blob; i++) {
        prop = drmModeGetProperty(m_dri_fd, connector->props[i]);
        if (!prop)
            continue;
        if ((prop->flags & DRM_MODE_PROP_BLOB) && (strcmp(prop->name, name.constData()) == 0))
            blob = drmModeGetPropertyBlob(m_dri_fd, connector->prop_values[i]);
        drmModeFreeProperty(prop);
    }

    return blob;
}

QKmsDevice::QKmsDevice(QKmsScreenConfig *screenConfig, const QString &path)
    : m_screenConfig(screenConfig)
    , m_path(path)
    , m_dri_fd(-1)
    , m_has_atomic_support(false)
    , m_crtc_allocator(0)
{
    if (m_path.isEmpty()) {
        m_path = m_screenConfig->devicePath();
        qCDebug(qLcKmsDebug, "Using DRM device %s specified in config file", qPrintable(m_path));
        if (m_path.isEmpty())
            qFatal("No DRM device given");
    } else {
        qCDebug(qLcKmsDebug, "Using backend-provided DRM device %s", qPrintable(m_path));
    }
}

QKmsDevice::~QKmsDevice()
{
#if QT_CONFIG(drm_atomic)
    threadLocalAtomicReset();
#endif
}

struct OrderedScreen
{
    OrderedScreen() : screen(nullptr) { }
    OrderedScreen(QPlatformScreen *screen, const QKmsDevice::ScreenInfo &vinfo)
        : screen(screen), vinfo(vinfo) { }
    QPlatformScreen *screen;
    QKmsDevice::ScreenInfo vinfo;
};

QDebug operator<<(QDebug dbg, const OrderedScreen &s)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "OrderedScreen(QPlatformScreen=" << s.screen << " (" << s.screen->name() << ") : "
                  << s.vinfo.virtualIndex
                  << " / " << s.vinfo.virtualPos
                  << " / primary: " << s.vinfo.isPrimary
                  << ")";
    return dbg;
}

static bool orderedScreenLessThan(const OrderedScreen &a, const OrderedScreen &b)
{
    return a.vinfo.virtualIndex < b.vinfo.virtualIndex;
}

void QKmsDevice::createScreens()
{
    // Headless mode using a render node: cannot do any output related DRM
    // stuff. Skip it all and register a dummy screen.
    if (m_screenConfig->headless()) {
        QPlatformScreen *screen = createHeadlessScreen();
        if (screen) {
            qCDebug(qLcKmsDebug, "Headless mode enabled");
            registerScreen(screen, true, QPoint(0, 0), QList<QPlatformScreen *>());
            return;
        } else {
            qWarning("QKmsDevice: Requested headless mode without support in the backend. Request is ignored.");
        }
    }

    drmSetClientCap(m_dri_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

#if QT_CONFIG(drm_atomic)
    // check atomic support
    m_has_atomic_support = !drmSetClientCap(m_dri_fd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (m_has_atomic_support) {
        qCDebug(qLcKmsDebug, "Atomic reported as supported");
        if (qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_ATOMIC")) {
            qCDebug(qLcKmsDebug, "Atomic enabled");
        } else {
            qCDebug(qLcKmsDebug, "Atomic disabled");
            m_has_atomic_support = false;
        }
    }
#endif

    drmModeResPtr resources = drmModeGetResources(m_dri_fd);
    if (!resources) {
        qErrnoWarning(errno, "drmModeGetResources failed");
        return;
    }

    discoverPlanes();

    QVector<OrderedScreen> screens;

    int wantedConnectorIndex = -1;
    bool ok;
    int idx = qEnvironmentVariableIntValue("QT_QPA_EGLFS_KMS_CONNECTOR_INDEX", &ok);
    if (ok) {
        if (idx >= 0 && idx < resources->count_connectors)
            wantedConnectorIndex = idx;
        else
            qWarning("Invalid connector index %d, must be between 0 and %u", idx, resources->count_connectors - 1);
    }

    for (int i = 0; i < resources->count_connectors; i++) {
        if (wantedConnectorIndex >= 0 && i != wantedConnectorIndex)
            continue;

        drmModeConnectorPtr connector = drmModeGetConnector(m_dri_fd, resources->connectors[i]);
        if (!connector)
            continue;

        ScreenInfo vinfo;
        QPlatformScreen *screen = createScreenForConnector(resources, connector, &vinfo);
        if (screen)
            screens.append(OrderedScreen(screen, vinfo));

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(resources);

    // Use stable sort to preserve the original (DRM connector) order
    // for outputs with unspecified indices.
    std::stable_sort(screens.begin(), screens.end(), orderedScreenLessThan);
    qCDebug(qLcKmsDebug) << "Sorted screen list:" << screens;

    // The final list of screens is available, so do the second phase setup.
    // Hook up clone sources and targets.
    for (const OrderedScreen &orderedScreen : screens) {
        QVector<QPlatformScreen *> screensCloningThisScreen;
        for (const OrderedScreen &s : screens) {
            if (s.vinfo.output.clone_source == orderedScreen.vinfo.output.name)
                screensCloningThisScreen.append(s.screen);
        }
        QPlatformScreen *screenThisScreenClones = nullptr;
        if (!orderedScreen.vinfo.output.clone_source.isEmpty()) {
            for (const OrderedScreen &s : screens) {
                if (s.vinfo.output.name == orderedScreen.vinfo.output.clone_source) {
                    screenThisScreenClones = s.screen;
                    break;
                }
            }
        }
        if (screenThisScreenClones)
            qCDebug(qLcKmsDebug) << orderedScreen.screen->name() << "clones" << screenThisScreenClones;
        if (!screensCloningThisScreen.isEmpty())
            qCDebug(qLcKmsDebug) << orderedScreen.screen->name() << "is cloned by" << screensCloningThisScreen;

        registerScreenCloning(orderedScreen.screen, screenThisScreenClones, screensCloningThisScreen);
    }

    // Figure out the virtual desktop and register the screens to QPA/QGuiApplication.
    QPoint pos(0, 0);
    QList<QPlatformScreen *> siblings;
    QVector<QPoint> virtualPositions;
    int primarySiblingIdx = -1;

    for (const OrderedScreen &orderedScreen : screens) {
        QPlatformScreen *s = orderedScreen.screen;
        QPoint virtualPos(0, 0);
        // set up a horizontal or vertical virtual desktop
        if (orderedScreen.vinfo.virtualPos.isNull()) {
            virtualPos = pos;
            if (m_screenConfig->virtualDesktopLayout() == QKmsScreenConfig::VirtualDesktopLayoutVertical)
                pos.ry() += s->geometry().height();
            else
                pos.rx() += s->geometry().width();
        } else {
            virtualPos = orderedScreen.vinfo.virtualPos;
        }
        qCDebug(qLcKmsDebug) << "Adding QPlatformScreen" << s << "(" << s->name() << ")"
                             << "to QPA with geometry" << s->geometry()
                             << "and isPrimary=" << orderedScreen.vinfo.isPrimary;
        // The order in qguiapp's screens list will match the order set by
        // virtualIndex. This is not only handy but also required since for instance
        // evdevtouch relies on it when performing touch device - screen mapping.
        if (!m_screenConfig->separateScreens()) {
            siblings.append(s);
            virtualPositions.append(virtualPos);
            if (orderedScreen.vinfo.isPrimary)
                primarySiblingIdx = siblings.count() - 1;
        } else {
            registerScreen(s, orderedScreen.vinfo.isPrimary, virtualPos, QList<QPlatformScreen *>() << s);
        }
    }

    if (!m_screenConfig->separateScreens()) {
        // enable the virtual desktop
        for (int i = 0; i < siblings.count(); ++i)
            registerScreen(siblings[i], i == primarySiblingIdx, virtualPositions[i], siblings);
    }
}

QPlatformScreen *QKmsDevice::createHeadlessScreen()
{
    // headless mode not supported by default
    return nullptr;
}

// not all subclasses support screen cloning
void QKmsDevice::registerScreenCloning(QPlatformScreen *screen,
                                       QPlatformScreen *screenThisScreenClones,
                                       const QVector<QPlatformScreen *> &screensCloningThisScreen)
{
    Q_UNUSED(screen);
    Q_UNUSED(screenThisScreenClones);
    Q_UNUSED(screensCloningThisScreen);
}

// drm_property_type_is is not available in old headers
static inline bool propTypeIs(drmModePropertyPtr prop, uint32_t type)
{
    if (prop->flags & DRM_MODE_PROP_EXTENDED_TYPE)
        return (prop->flags & DRM_MODE_PROP_EXTENDED_TYPE) == type;
    return prop->flags & type;
}

void QKmsDevice::enumerateProperties(drmModeObjectPropertiesPtr objProps, PropCallback callback)
{
    for (uint32_t propIdx = 0; propIdx < objProps->count_props; ++propIdx) {
        drmModePropertyPtr prop = drmModeGetProperty(m_dri_fd, objProps->props[propIdx]);
        if (!prop)
            continue;

        const quint64 value = objProps->prop_values[propIdx];
        qCDebug(qLcKmsDebug, "  property %d: id = %u name = '%s'", propIdx, prop->prop_id, prop->name);

        if (propTypeIs(prop, DRM_MODE_PROP_SIGNED_RANGE)) {
            qCDebug(qLcKmsDebug, "  type is SIGNED_RANGE, value is %lld, possible values are:", qint64(value));
            for (int i = 0; i < prop->count_values; ++i)
                qCDebug(qLcKmsDebug, "    %lld", qint64(prop->values[i]));
        } else if (propTypeIs(prop, DRM_MODE_PROP_RANGE)) {
            qCDebug(qLcKmsDebug, "  type is RANGE, value is %llu, possible values are:", value);
            for (int i = 0; i < prop->count_values; ++i)
                qCDebug(qLcKmsDebug, "    %llu", quint64(prop->values[i]));
        } else if (propTypeIs(prop, DRM_MODE_PROP_ENUM)) {
            qCDebug(qLcKmsDebug, "  type is ENUM, value is %llu, possible values are:", value);
            for (int i = 0; i < prop->count_enums; ++i)
                qCDebug(qLcKmsDebug, "    enum %d: %s - %llu", i, prop->enums[i].name, quint64(prop->enums[i].value));
        } else if (propTypeIs(prop, DRM_MODE_PROP_BITMASK)) {
            qCDebug(qLcKmsDebug, "  type is BITMASK, value is %llu, possible bits are:", value);
            for (int i = 0; i < prop->count_enums; ++i)
                qCDebug(qLcKmsDebug, "    bitmask %d: %s - %u", i, prop->enums[i].name, 1 << prop->enums[i].value);
        } else if (propTypeIs(prop, DRM_MODE_PROP_BLOB)) {
            qCDebug(qLcKmsDebug, "  type is BLOB");
        } else if (propTypeIs(prop, DRM_MODE_PROP_OBJECT)) {
            qCDebug(qLcKmsDebug, "  type is OBJECT");
        }

        callback(prop, value);

        drmModeFreeProperty(prop);
    }
}

void QKmsDevice::discoverPlanes()
{
    m_planes.clear();

    drmModePlaneResPtr planeResources = drmModeGetPlaneResources(m_dri_fd);
    if (!planeResources)
        return;

    const int countPlanes = planeResources->count_planes;
    qCDebug(qLcKmsDebug, "Found %d planes", countPlanes);
    for (int planeIdx = 0; planeIdx < countPlanes; ++planeIdx) {
        drmModePlanePtr drmplane = drmModeGetPlane(m_dri_fd, planeResources->planes[planeIdx]);
        if (!drmplane) {
            qCDebug(qLcKmsDebug, "Failed to query plane %d, ignoring", planeIdx);
            continue;
        }

        QKmsPlane plane;
        plane.id = drmplane->plane_id;
        plane.possibleCrtcs = drmplane->possible_crtcs;

        const int countFormats = drmplane->count_formats;
        QString formatStr;
        for (int i = 0; i < countFormats; ++i) {
            uint32_t f = drmplane->formats[i];
            plane.supportedFormats.append(f);
            formatStr += QString::asprintf("%c%c%c%c ", f, f >> 8, f >> 16, f >> 24);
        }

        qCDebug(qLcKmsDebug, "plane %d: id = %u countFormats = %d possibleCrtcs = 0x%x supported formats = %s",
                planeIdx, plane.id, countFormats, plane.possibleCrtcs, qPrintable(formatStr));

        drmModeFreePlane(drmplane);

        drmModeObjectPropertiesPtr objProps = drmModeObjectGetProperties(m_dri_fd, plane.id, DRM_MODE_OBJECT_PLANE);
        if (!objProps) {
            qCDebug(qLcKmsDebug, "Failed to query plane %d object properties, ignoring", planeIdx);
            continue;
        }

        enumerateProperties(objProps, [&plane](drmModePropertyPtr prop, quint64 value) {
            if (!strcmp(prop->name, "type")) {
                plane.type = QKmsPlane::Type(value);
            } else if (!strcmp(prop->name, "rotation")) {
                plane.initialRotation = QKmsPlane::Rotations(int(value));
                plane.availableRotations = { };
                if (propTypeIs(prop, DRM_MODE_PROP_BITMASK)) {
                    for (int i = 0; i < prop->count_enums; ++i)
                        plane.availableRotations |= QKmsPlane::Rotation(1 << prop->enums[i].value);
                }
                plane.rotationPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "crtc_id")) {
                plane.crtcPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "fb_id")) {
                plane.framebufferPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "src_w")) {
                plane.srcwidthPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "src_h")) {
                plane.srcheightPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "crtc_w")) {
                plane.crtcwidthPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "crtc_h")) {
                plane.crtcheightPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "src_x")) {
                plane.srcXPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "src_y")) {
                plane.srcYPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "crtc_x")) {
                plane.crtcXPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "crtc_y")) {
                plane.crtcYPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "zpos")) {
                plane.zposPropertyId = prop->prop_id;
            } else if (!strcasecmp(prop->name, "blend_op")) {
                plane.blendOpPropertyId = prop->prop_id;
            }
        });

        m_planes.append(plane);

        drmModeFreeObjectProperties(objProps);
    }

    drmModeFreePlaneResources(planeResources);
}

int QKmsDevice::fd() const
{
    return m_dri_fd;
}

QString QKmsDevice::devicePath() const
{
    return m_path;
}

void QKmsDevice::setFd(int fd)
{
    m_dri_fd = fd;
}


bool QKmsDevice::hasAtomicSupport()
{
    return m_has_atomic_support;
}

#if QT_CONFIG(drm_atomic)
drmModeAtomicReq *QKmsDevice::threadLocalAtomicRequest()
{
    if (!m_has_atomic_support)
        return nullptr;

    AtomicReqs &a(m_atomicReqs.localData());
    if (!a.request)
        a.request = drmModeAtomicAlloc();

    return a.request;
}

bool QKmsDevice::threadLocalAtomicCommit(void *user_data)
{
    if (!m_has_atomic_support)
        return false;

    AtomicReqs &a(m_atomicReqs.localData());
    if (!a.request)
        return false;

    int ret = drmModeAtomicCommit(m_dri_fd, a.request,
                                  DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_ALLOW_MODESET,
                                  user_data);

    if (ret) {
        qWarning("Failed to commit atomic request (code=%d)", ret);
        return false;
    }

    a.previous_request = a.request;
    a.request = nullptr;

    return true;
}

void QKmsDevice::threadLocalAtomicReset()
{
    if (!m_has_atomic_support)
        return;

    AtomicReqs &a(m_atomicReqs.localData());
    if (a.previous_request) {
        drmModeAtomicFree(a.previous_request);
        a.previous_request = nullptr;
    }
}
#endif

void QKmsDevice::parseConnectorProperties(uint32_t connectorId, QKmsOutput *output)
{
    drmModeObjectPropertiesPtr objProps = drmModeObjectGetProperties(m_dri_fd, connectorId, DRM_MODE_OBJECT_CONNECTOR);
    if (!objProps) {
        qCDebug(qLcKmsDebug, "Failed to query connector %d object properties", connectorId);
        return;
    }

    enumerateProperties(objProps, [output](drmModePropertyPtr prop, quint64 value) {
        Q_UNUSED(value);
        if (!strcasecmp(prop->name, "crtc_id"))
            output->crtcIdPropertyId = prop->prop_id;
    });

    drmModeFreeObjectProperties(objProps);
}

void QKmsDevice::parseCrtcProperties(uint32_t crtcId, QKmsOutput *output)
{
    drmModeObjectPropertiesPtr objProps = drmModeObjectGetProperties(m_dri_fd, crtcId, DRM_MODE_OBJECT_CRTC);
    if (!objProps) {
        qCDebug(qLcKmsDebug, "Failed to query crtc %d object properties", crtcId);
        return;
    }

    enumerateProperties(objProps, [output](drmModePropertyPtr prop, quint64 value) {
        Q_UNUSED(value)
        if (!strcasecmp(prop->name, "mode_id"))
            output->modeIdPropertyId = prop->prop_id;
        else if (!strcasecmp(prop->name, "active"))
            output->activePropertyId = prop->prop_id;
    });

    drmModeFreeObjectProperties(objProps);
}

QKmsScreenConfig *QKmsDevice::screenConfig() const
{
    return m_screenConfig;
}

QKmsScreenConfig::QKmsScreenConfig()
    : m_headless(false)
    , m_hwCursor(true)
    , m_separateScreens(false)
    , m_pbuffers(false)
    , m_virtualDesktopLayout(VirtualDesktopLayoutHorizontal)
{
    loadConfig();
}

void QKmsScreenConfig::loadConfig()
{
    QByteArray json = qgetenv("QT_QPA_EGLFS_KMS_CONFIG");
    if (json.isEmpty()) {
        json = qgetenv("QT_QPA_KMS_CONFIG");
        if (json.isEmpty())
            return;
    }

    qCDebug(qLcKmsDebug) << "Loading KMS setup from" << json;

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        qCWarning(qLcKmsDebug) << "Could not open config file"
                               << json << "for reading";
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qCWarning(qLcKmsDebug) << "Invalid config file" << json
                              << "- no top-level JSON object";
        return;
    }

    const QJsonObject object = doc.object();

    const QString headlessStr = object.value(QLatin1String("headless")).toString();
    const QByteArray headless = headlessStr.toUtf8();
    QSize headlessSize;
    if (sscanf(headless.constData(), "%dx%d", &headlessSize.rwidth(), &headlessSize.rheight()) == 2) {
        m_headless = true;
        m_headlessSize = headlessSize;
    } else {
        m_headless = false;
    }

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
            qCWarning(qLcKmsDebug) << "Unknown virtualDesktopOrientation value" << vdOriString;
    }

    const QJsonArray outputs = object.value(QLatin1String("outputs")).toArray();
    for (int i = 0; i < outputs.size(); i++) {
        const QVariantMap outputSettings = outputs.at(i).toObject().toVariantMap();

        if (outputSettings.contains(QStringLiteral("name"))) {
            const QString name = outputSettings.value(QStringLiteral("name")).toString();

            if (m_outputSettings.contains(name)) {
                qCDebug(qLcKmsDebug) << "Output" << name << "configured multiple times!";
            }

            m_outputSettings.insert(name, outputSettings);
        }
    }

    qCDebug(qLcKmsDebug) << "Requested configuration (some settings may be ignored):\n"
                         << "\theadless:" << m_headless << "\n"
                         << "\thwcursor:" << m_hwCursor << "\n"
                         << "\tpbuffers:" << m_pbuffers << "\n"
                         << "\tseparateScreens:" << m_separateScreens << "\n"
                         << "\tvirtualDesktopLayout:" << m_virtualDesktopLayout << "\n"
                         << "\toutputs:" << m_outputSettings;
}

void QKmsOutput::restoreMode(QKmsDevice *device)
{
    if (mode_set && saved_crtc) {
        drmModeSetCrtc(device->fd(),
                       saved_crtc->crtc_id,
                       saved_crtc->buffer_id,
                       0, 0,
                       &connector_id, 1,
                       &saved_crtc->mode);
        mode_set = false;
    }
}

void QKmsOutput::cleanup(QKmsDevice *device)
{
    if (dpms_prop) {
        drmModeFreeProperty(dpms_prop);
        dpms_prop = nullptr;
    }

    if (edid_blob) {
        drmModeFreePropertyBlob(edid_blob);
        edid_blob = nullptr;
    }

    restoreMode(device);

    if (saved_crtc) {
        drmModeFreeCrtc(saved_crtc);
        saved_crtc = nullptr;
    }
}

QPlatformScreen::SubpixelAntialiasingType QKmsOutput::subpixelAntialiasingTypeHint() const
{
    switch (subpixel) {
    default:
    case DRM_MODE_SUBPIXEL_UNKNOWN:
    case DRM_MODE_SUBPIXEL_NONE:
        return QPlatformScreen::Subpixel_None;
    case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
        return QPlatformScreen::Subpixel_RGB;
    case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
        return QPlatformScreen::Subpixel_BGR;
    case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
        return QPlatformScreen::Subpixel_VRGB;
    case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
        return QPlatformScreen::Subpixel_VBGR;
    }
}

void QKmsOutput::setPowerState(QKmsDevice *device, QPlatformScreen::PowerState state)
{
    if (dpms_prop)
        drmModeConnectorSetProperty(device->fd(), connector_id,
                                    dpms_prop->prop_id, (int) state);
}

QT_END_NAMESPACE
