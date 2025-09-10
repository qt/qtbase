// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglfskmsscreen_p.h"
#include "qeglfskmsdevice_p.h"
#include <private/qeglfsintegration_p.h>

#include <QtCore/QLoggingCategory>

#include <QtGui/private/qguiapplication_p.h>
#include <QtFbSupport/private/qfbvthandler_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

class QEglFSKmsInterruptHandler : public QObject
{
public:
    QEglFSKmsInterruptHandler(QEglFSKmsScreen *screen) : m_screen(screen) {
        m_vtHandler = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration())->vtHandler();
        connect(m_vtHandler, &QFbVtHandler::interrupted, this, &QEglFSKmsInterruptHandler::restoreVideoMode);
        connect(m_vtHandler, &QFbVtHandler::aboutToSuspend, this, &QEglFSKmsInterruptHandler::restoreVideoMode);
    }

public slots:
    void restoreVideoMode() { m_screen->restoreMode(); }

private:
    QFbVtHandler *m_vtHandler;
    QEglFSKmsScreen *m_screen;
};

QEglFSKmsScreen::QEglFSKmsScreen(QEglFSKmsDevice *device, const QKmsOutput &output, bool headless)
    : QEglFSScreen(static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration())->display())
    , m_device(device)
    , m_output(output)
    , m_cursorOutOfRange(false)
    , m_powerState(PowerStateOn)
    , m_interruptHandler(new QEglFSKmsInterruptHandler(this))
    , m_headless(headless)
{
    m_siblings << this; // gets overridden later

    if (m_output.edid_blob) {
        QByteArray edid(reinterpret_cast<const char *>(m_output.edid_blob->data), m_output.edid_blob->length);
        if (m_edid.parse(edid))
            qCDebug(qLcEglfsKmsDebug, "EDID data for output \"%s\": identifier '%s', manufacturer '%s', model '%s', serial '%s', physical size: %.2fx%.2f",
                    name().toLatin1().constData(),
                    m_edid.identifier.toLatin1().constData(),
                    m_edid.manufacturer.toLatin1().constData(),
                    m_edid.model.toLatin1().constData(),
                    m_edid.serialNumber.toLatin1().constData(),
                    m_edid.physicalSize.width(), m_edid.physicalSize.height());
        else
            qCDebug(qLcEglfsKmsDebug) << "Failed to parse EDID data for output" << name(); // keep this debug, not warning
    } else {
        qCDebug(qLcEglfsKmsDebug) << "No EDID data for output" << name();
    }
}

QEglFSKmsScreen::~QEglFSKmsScreen()
{
    m_output.cleanup(m_device);
    delete m_interruptHandler;
}

void QEglFSKmsScreen::setVirtualPosition(const QPoint &pos)
{
    m_pos = pos;
}

// Reimplement rawGeometry(), not geometry(). The base class implementation of
// geometry() calls rawGeometry() and may apply additional transforms.
QRect QEglFSKmsScreen::rawGeometry() const
{
    if (m_headless)
        return QRect(QPoint(0, 0), m_device->screenConfig()->headlessSize());

    return QRect(m_pos.x(), m_pos.y(),
                 m_output.size.width(),
                 m_output.size.height());
}

int QEglFSKmsScreen::depth() const
{
    return format() == QImage::Format_RGB16 ? 16 : 32;
}

QImage::Format QEglFSKmsScreen::format() const
{
    // the result can be slightly incorrect, it won't matter in practice
    switch (m_output.drm_format) {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_ABGR8888:
        return QImage::Format_ARGB32;
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
        return QImage::Format_RGB16;
    case DRM_FORMAT_XRGB2101010:
        return QImage::Format_RGB30;
    case DRM_FORMAT_XBGR2101010:
        return QImage::Format_BGR30;
    case DRM_FORMAT_ARGB2101010:
        return QImage::Format_A2RGB30_Premultiplied;
    case DRM_FORMAT_ABGR2101010:
        return QImage::Format_A2BGR30_Premultiplied;
    default:
        return QImage::Format_RGB32;
    }
}

QSizeF QEglFSKmsScreen::physicalSize() const
{
    if (!m_output.physical_size.isEmpty()) {
        return m_output.physical_size;
    } else {
        const QSize s = geometry().size();
        return QSizeF(0.254 * s.width(), 0.254 * s.height());
    }
}

QDpi QEglFSKmsScreen::logicalDpi() const
{
    return logicalBaseDpi();
}

QDpi QEglFSKmsScreen::logicalBaseDpi() const
{
    return QDpi(100, 100);
}

Qt::ScreenOrientation QEglFSKmsScreen::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

Qt::ScreenOrientation QEglFSKmsScreen::orientation() const
{
    return Qt::PrimaryOrientation;
}

QString QEglFSKmsScreen::name() const
{
    return !m_headless ? m_output.name : QStringLiteral("qt_Headless");
}

QString QEglFSKmsScreen::manufacturer() const
{
    return m_edid.manufacturer;
}

QString QEglFSKmsScreen::model() const
{
    return m_edid.model.isEmpty() ? m_edid.identifier : m_edid.model;
}

QString QEglFSKmsScreen::serialNumber() const
{
    return m_edid.serialNumber;
}

void QEglFSKmsScreen::waitForFlip()
{
}

void QEglFSKmsScreen::restoreMode()
{
    m_output.restoreMode(m_device);
}

qreal QEglFSKmsScreen::refreshRate() const
{
    if (m_headless)
        return 60;

    quint32 refresh = m_output.modes[m_output.mode].vrefresh;
    return refresh > 0 ? refresh : 60;
}

QList<QPlatformScreen::Mode> QEglFSKmsScreen::modes() const
{
    QList<QPlatformScreen::Mode> list;
    list.reserve(m_output.modes.size());

    for (const drmModeModeInfo &info : std::as_const(m_output.modes))
        list.append({QSize(info.hdisplay, info.vdisplay),
                     qreal(info.vrefresh > 0 ? info.vrefresh : 60)});

    return list;
}

int QEglFSKmsScreen::currentMode() const
{
    return m_output.mode;
}

int QEglFSKmsScreen::preferredMode() const
{
    return m_output.preferred_mode;
}

QPlatformScreen::SubpixelAntialiasingType QEglFSKmsScreen::subpixelAntialiasingTypeHint() const
{
    return m_output.subpixelAntialiasingTypeHint();
}

QPlatformScreen::PowerState QEglFSKmsScreen::powerState() const
{
    return m_powerState;
}

void QEglFSKmsScreen::setPowerState(QPlatformScreen::PowerState state)
{
    m_output.setPowerState(m_device, state);
    m_powerState = state;
}

/* Informs exact page flip timing which can be used rendering optimization.
   Consider this is from drm event reader thread. */
void QEglFSKmsScreen::pageFlipped(unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec)
{
    Q_UNUSED(sequence);
    Q_UNUSED(tv_sec);
    Q_UNUSED(tv_usec);
}

QT_END_NAMESPACE
