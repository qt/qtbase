/****************************************************************************
**
** Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
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
#include "qeglfsintegration_p.h"

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

QEglFSKmsScreen::QEglFSKmsScreen(QKmsDevice *device, const QKmsOutput &output)
    : QEglFSScreen(static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration())->display())
    , m_device(device)
    , m_output(output)
    , m_powerState(PowerStateOn)
    , m_interruptHandler(new QEglFSKmsInterruptHandler(this))
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
    if (!m_output.physical_size.isEmpty()) {
        return m_output.physical_size;
    } else {
        const QSize s = geometry().size();
        return QSizeF(0.254 * s.width(), 0.254 * s.height());
    }
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
    m_output.restoreMode(m_device);
}

qreal QEglFSKmsScreen::refreshRate() const
{
    quint32 refresh = m_output.modes[m_output.mode].vrefresh;
    return refresh > 0 ? refresh : 60;
}

QVector<QPlatformScreen::Mode> QEglFSKmsScreen::modes() const
{
    QVector<QPlatformScreen::Mode> list;
    list.reserve(m_output.modes.size());

    for (const drmModeModeInfo &info : qAsConst(m_output.modes))
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

QT_END_NAMESPACE
