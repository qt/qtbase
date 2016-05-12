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

#include "qeglfskmsscreen.h"
#include "qeglfskmsdevice.h"
#include "qeglfsintegration.h"

#include <QtCore/QLoggingCategory>

#include <QtGui/private/qguiapplication_p.h>
#include <QtPlatformSupport/private/qfbvthandler_p.h>

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

QEglFSKmsScreen::QEglFSKmsScreen(QEglFSKmsIntegration *integration,
                                 QEglFSKmsDevice *device,
                                 QEglFSKmsOutput output,
                                 QPoint position)
    : QEglFSScreen(eglGetDisplay(device->nativeDisplay()))
    , m_integration(integration)
    , m_device(device)
    , m_output(output)
    , m_pos(position)
    , m_powerState(PowerStateOn)
    , m_interruptHandler(new QEglFSKmsInterruptHandler(this))
{
    m_siblings << this;
}

QEglFSKmsScreen::~QEglFSKmsScreen()
{
    if (m_output.dpms_prop) {
        drmModeFreeProperty(m_output.dpms_prop);
        m_output.dpms_prop = Q_NULLPTR;
    }
    restoreMode();
    if (m_output.saved_crtc) {
        drmModeFreeCrtc(m_output.saved_crtc);
        m_output.saved_crtc = Q_NULLPTR;
    }
    delete m_interruptHandler;
}

QRect QEglFSKmsScreen::geometry() const
{
    const int mode = m_output.mode;
    return QRect(m_pos.x(), m_pos.y(),
                 m_output.modes[mode].hdisplay,
                 m_output.modes[mode].vdisplay);
}

int QEglFSKmsScreen::depth() const
{
    return 32;
}

QImage::Format QEglFSKmsScreen::format() const
{
    return QImage::Format_RGB32;
}

QSizeF QEglFSKmsScreen::physicalSize() const
{
    return m_output.physical_size;
}

QDpi QEglFSKmsScreen::logicalDpi() const
{
    const QSizeF ps = physicalSize();
    const QSize s = geometry().size();

    if (!ps.isEmpty() && !s.isEmpty())
        return QDpi(25.4 * s.width() / ps.width(),
                    25.4 * s.height() / ps.height());
    else
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
    return m_output.name;
}

void QEglFSKmsScreen::destroySurface()
{
}

void QEglFSKmsScreen::waitForFlip()
{
}

void QEglFSKmsScreen::flip()
{
}

void QEglFSKmsScreen::flipFinished()
{
}

void QEglFSKmsScreen::restoreMode()
{
    if (m_output.mode_set && m_output.saved_crtc) {
        drmModeSetCrtc(m_device->fd(),
                       m_output.saved_crtc->crtc_id,
                       m_output.saved_crtc->buffer_id,
                       0, 0,
                       &m_output.connector_id, 1,
                       &m_output.saved_crtc->mode);

        m_output.mode_set = false;
    }
}

qreal QEglFSKmsScreen::refreshRate() const
{
    quint32 refresh = m_output.modes[m_output.mode].vrefresh;
    return refresh > 0 ? refresh : 60;
}

QPlatformScreen::SubpixelAntialiasingType QEglFSKmsScreen::subpixelAntialiasingTypeHint() const
{
    switch (m_output.subpixel) {
    default:
    case DRM_MODE_SUBPIXEL_UNKNOWN:
    case DRM_MODE_SUBPIXEL_NONE:
        return Subpixel_None;
    case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
        return Subpixel_RGB;
    case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
        return Subpixel_BGR;
    case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
        return Subpixel_VRGB;
    case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
        return Subpixel_VBGR;
    }
}

QPlatformScreen::PowerState QEglFSKmsScreen::powerState() const
{
    return m_powerState;
}

void QEglFSKmsScreen::setPowerState(QPlatformScreen::PowerState state)
{
    if (!m_output.dpms_prop)
        return;

    drmModeConnectorSetProperty(m_device->fd(), m_output.connector_id,
                                m_output.dpms_prop->prop_id, (int)state);
    m_powerState = state;
}

QT_END_NAMESPACE
