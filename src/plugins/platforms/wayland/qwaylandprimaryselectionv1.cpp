// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwaylandprimaryselectionv1_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylanddisplay_p.h"
#include "qwaylandmimehelper_p.h"

#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qplatformclipboard.h>

#include <signal.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandPrimarySelectionDeviceManagerV1::QWaylandPrimarySelectionDeviceManagerV1(QWaylandDisplay *display, uint id, uint version)
    : zwp_primary_selection_device_manager_v1(display->wl_registry(), id, qMin(version, uint(1)))
    , m_display(display)
{
}

QWaylandPrimarySelectionDeviceManagerV1::~QWaylandPrimarySelectionDeviceManagerV1()
{
    destroy();
}

QWaylandPrimarySelectionDeviceV1 *QWaylandPrimarySelectionDeviceManagerV1::createDevice(QWaylandInputDevice *seat)
{
    return new QWaylandPrimarySelectionDeviceV1(this, seat);
}

QWaylandPrimarySelectionOfferV1::QWaylandPrimarySelectionOfferV1(QWaylandDisplay *display, ::zwp_primary_selection_offer_v1 *offer)
    : zwp_primary_selection_offer_v1(offer)
    , m_display(display)
    , m_mimeData(new QWaylandMimeData(this))
{}

void QWaylandPrimarySelectionOfferV1::startReceiving(const QString &mimeType, int fd)
{
    receive(mimeType, fd);
    wl_display_flush(m_display->wl_display());
}

void QWaylandPrimarySelectionOfferV1::zwp_primary_selection_offer_v1_offer(const QString &mime_type)
{
    m_mimeData->appendFormat(mime_type);
}

QWaylandPrimarySelectionDeviceV1::QWaylandPrimarySelectionDeviceV1(
        QWaylandPrimarySelectionDeviceManagerV1 *manager, QWaylandInputDevice *seat)
    : QtWayland::zwp_primary_selection_device_v1(manager->get_device(seat->wl_seat()))
    , m_display(manager->display())
    , m_seat(seat)
{
}

QWaylandPrimarySelectionDeviceV1::~QWaylandPrimarySelectionDeviceV1()
{
    destroy();
}

void QWaylandPrimarySelectionDeviceV1::invalidateSelectionOffer()
{
    if (!m_selectionOffer)
        return;

    m_selectionOffer.reset();
    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Selection);
}

void QWaylandPrimarySelectionDeviceV1::setSelectionSource(QWaylandPrimarySelectionSourceV1 *source)
{
    if (source) {
        connect(source, &QWaylandPrimarySelectionSourceV1::cancelled, this, [this]() {
            m_selectionSource.reset();
            QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Selection);
        });
    }
    set_selection(source ? source->object() : nullptr, m_seat->serial());
    m_selectionSource.reset(source);
}

void QWaylandPrimarySelectionDeviceV1::zwp_primary_selection_device_v1_data_offer(zwp_primary_selection_offer_v1 *offer)
{
    new QWaylandPrimarySelectionOfferV1(m_display, offer);
}

void QWaylandPrimarySelectionDeviceV1::zwp_primary_selection_device_v1_selection(zwp_primary_selection_offer_v1 *id)
{

    if (id)
        m_selectionOffer.reset(static_cast<QWaylandPrimarySelectionOfferV1 *>(zwp_primary_selection_offer_v1_get_user_data(id)));
    else
        m_selectionOffer.reset();

    QGuiApplicationPrivate::platformIntegration()->clipboard()->emitChanged(QClipboard::Selection);
}

QWaylandPrimarySelectionSourceV1::QWaylandPrimarySelectionSourceV1(QWaylandPrimarySelectionDeviceManagerV1 *manager, QMimeData *mimeData)
    : QtWayland::zwp_primary_selection_source_v1(manager->create_source())
    , m_mimeData(mimeData)
{
    if (!mimeData)
        return;
    for (auto &format : mimeData->formats())
        offer(format);
}

QWaylandPrimarySelectionSourceV1::~QWaylandPrimarySelectionSourceV1()
{
    destroy();
}

void QWaylandPrimarySelectionSourceV1::zwp_primary_selection_source_v1_send(const QString &mime_type, int32_t fd)
{
    QByteArray content = QWaylandMimeHelper::getByteArray(m_mimeData, mime_type);
    if (!content.isEmpty()) {
        // Create a sigpipe handler that does nothing, or clients may be forced to terminate
        // if the pipe is closed in the other end.
        struct sigaction action, oldAction;
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

#include "moc_qwaylandprimaryselectionv1_p.cpp"
