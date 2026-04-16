// Copyright (C) 2024 Jie Liu <liujie01@kylinos.cn>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylanddatacontrolv1_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandmimehelper_p.h"

#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformclipboard.h>

#include <signal.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandDataControlManagerV1::QWaylandDataControlManagerV1(QWaylandDisplay *display, uint id, uint version)
    : zwlr_data_control_manager_v1(display->wl_registry(), id, qMin(version, uint(2)))
    , m_display(display)
{
}

QWaylandDataControlDeviceV1 *QWaylandDataControlManagerV1::createDevice(QWaylandInputDevice *seat)
{
    return new QWaylandDataControlDeviceV1(this, seat);
}

QWaylandDataControlOfferV1::QWaylandDataControlOfferV1(QWaylandDisplay *display, ::zwlr_data_control_offer_v1 *offer)
    : zwlr_data_control_offer_v1(offer)
    , m_display(display)
    , m_mimeData(new QWaylandMimeData(this))
{}

void QWaylandDataControlOfferV1::startReceiving(const QString &mimeType, int fd)
{
    receive(mimeType, fd);
    wl_display_flush(m_display->wl_display());
}

void QWaylandDataControlOfferV1::zwlr_data_control_offer_v1_offer(const QString &mime_type)
{
    m_mimeData->appendFormat(mime_type);
}

QWaylandDataControlDeviceV1::QWaylandDataControlDeviceV1(
        QWaylandDataControlManagerV1 *manager, QWaylandInputDevice *seat)
    : QtWayland::zwlr_data_control_device_v1(manager->get_data_device(seat->wl_seat()))
    , m_display(manager->display())
    , m_seat(seat)
{
}

QWaylandDataControlDeviceV1::~QWaylandDataControlDeviceV1()
{
    destroy();
}

void QWaylandDataControlDeviceV1::invalidateSelectionOffer()
{
    if (!m_selectionOffer)
        return;

    m_selectionOffer.reset();
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
}

void QWaylandDataControlDeviceV1::setSelectionSource(QWaylandDataControlSourceV1 *source)
{
    if (source) {
        connect(source, &QWaylandDataControlSourceV1::cancelled, this, [this]() {
            m_selectionSource.reset();
            QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
        });
    }
    set_selection(source ? source->object() : nullptr);
    m_selectionSource.reset(source);
}

void QWaylandDataControlDeviceV1::setPrimarySelectionSource(QWaylandDataControlSourceV1 *source)
{
    if (source) {
        connect(source, &QWaylandDataControlSourceV1::cancelled, this, [this]() {
            m_primarySelectionSource.reset();
            QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Selection);
        });
    }
    set_primary_selection(source ? source->object() : nullptr);
    m_primarySelectionSource.reset(source);
}

void QWaylandDataControlDeviceV1::zwlr_data_control_device_v1_data_offer(zwlr_data_control_offer_v1 *offer)
{
    new QWaylandDataControlOfferV1(m_display, offer);
}

void QWaylandDataControlDeviceV1::zwlr_data_control_device_v1_selection(zwlr_data_control_offer_v1 *id)
{
    if (!id)
        m_selectionOffer.reset();
    else
        m_selectionOffer.reset(static_cast<QWaylandDataControlOfferV1 *>(zwlr_data_control_offer_v1_get_user_data(id)));

    // The selection event may be sent before platfrmIntegration is set.
    if (auto* integration = QGuiApplicationPrivate::platformIntegration())
            integration->clipboard()->emitChanged(QClipboard::Clipboard);
}

void QWaylandDataControlDeviceV1::zwlr_data_control_device_v1_finished()
{
    m_selectionOffer.reset();
    m_primarySelectionOffer.reset();
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Clipboard);
}

void QWaylandDataControlDeviceV1::zwlr_data_control_device_v1_primary_selection(struct ::zwlr_data_control_offer_v1 *id)
{
    if (!id)
        m_primarySelectionOffer.reset();
    else
        m_primarySelectionOffer.reset(static_cast<QWaylandDataControlOfferV1 *>(zwlr_data_control_offer_v1_get_user_data(id)));

    // The selection event may be sent before platfrmIntegration is set.
    if (auto* integration = QGuiApplicationPrivate::platformIntegration())
            integration->clipboard()->emitChanged(QClipboard::Selection);
}

QWaylandDataControlSourceV1::QWaylandDataControlSourceV1(QWaylandDataControlManagerV1 *manager, QMimeData *mimeData)
    : QtWayland::zwlr_data_control_source_v1(manager->create_data_source())
    , m_mimeData(mimeData)
{
    if (!mimeData)
        return;
    const auto formats = QInternalMimeData::formatsHelper(mimeData);
    for (const QString &format : formats)
        offer(format);
}

QWaylandDataControlSourceV1::~QWaylandDataControlSourceV1()
{
    destroy();
}

void QWaylandDataControlSourceV1::zwlr_data_control_source_v1_send(const QString &mime_type, int32_t fd)
{
    QByteArray content = QWaylandMimeHelper::getByteArray(m_mimeData, mime_type);
    if (!content.isEmpty()) {
        // Create a sigpipe handler that does nothing, or clients may be forced to terminate
        // if the pipe is closed in the other end.
        struct sigaction action = {};
        struct sigaction oldAction = {};
        action.sa_handler = SIG_IGN;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;

        sigaction(SIGPIPE, &action, &oldAction);
        ssize_t unused = write(fd, content.constData(), size_t(content.size()));
        Q_UNUSED(unused);
        sigaction(SIGPIPE, &oldAction, nullptr);
    }
    close(fd);
}

} // namespace QtWaylandClient

QT_END_NAMESPACE
