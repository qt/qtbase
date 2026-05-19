// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "datadevice.h"

namespace MockCompositor {

bool DataDeviceManager::isClean()
{
    for (auto *device : std::as_const(m_dataDevices)) {
        // The client should not leak selection offers, i.e. if this fails, there is a missing
        // data_offer.destroy request
        if (!device->m_sentSelectionOffers.empty())
            return false;
    }
    return true;
}

DataDevice *DataDeviceManager::deviceFor(Seat *seat)
{
    Q_ASSERT(seat);
    if (auto *device = m_dataDevices.value(seat, nullptr))
        return device;

    auto *device = new DataDevice(this, seat);
    m_dataDevices[seat] = device;
    return device;
}

void DataDeviceManager::data_device_manager_get_data_device(Resource *resource, uint32_t id, wl_resource *seatResource)
{
    auto *seat = fromResource<Seat>(seatResource);
    QVERIFY(seat);
    auto *device = deviceFor(seat);
    device->add(resource->client(), id, resource->version());
}

void DataDeviceManager::data_device_manager_create_data_source(Resource *resource, uint32_t id)
{
    new QtWaylandServer::wl_data_source(resource->client(), id, 1);
}

DataDevice::~DataDevice()
{
    // If the client(s) hasn't deleted the wayland object, just ignore subsequent events
    for (auto *r : resourceMap())
        wl_resource_set_implementation(r->handle, nullptr, nullptr, nullptr);
}

DataOffer *DataDevice::sendDataOffer(wl_client *client, const QStringList &mimeTypes)
{
    Q_ASSERT(client);
    auto *offer = new DataOffer(this, client, m_manager->m_version);
    m_offer = offer;
    for (auto *resource : resourceMap().values(client))
        wl_data_device::send_data_offer(resource->handle, offer->resource()->handle);
    for (const auto &mimeType : mimeTypes)
        offer->send_offer(mimeType);
    return offer;
}

void DataDevice::sendSelection(DataOffer *offer)
{
    auto *client = offer->resource()->client();
    for (auto *resource : resourceMap().values(client))
        wl_data_device::send_selection(resource->handle, offer->resource()->handle);
    m_sentSelectionOffers << offer;
}

void DataDevice::clearSelection(wl_client *client)
{
    for (auto *resource : resourceMap().values(client))
        wl_data_device::send_selection(resource->handle, nullptr);
}

void DataDevice::sendEnter(Surface *surface, const QPoint &position)
{
    uint serial = m_manager->m_compositor->nextSerial();
    Resource *resource = resourceMap().value(surface->resource()->client());
    if (m_offer)
        wl_data_device::send_enter(resource->handle, serial, surface->resource()->handle, position.x(), position.y(), m_offer->resource()->handle);
}

void DataDevice::sendMotion(Surface *surface, const QPoint &position)
{
    uint32_t time = m_manager->m_compositor->nextSerial();
    Resource *resource = resourceMap().value(surface->resource()->client());
    wl_data_device::send_motion(resource->handle, time, position.x(), position.y());
}

void DataDevice::sendDrop(Surface *surface)
{
    Resource *resource = resourceMap().value(surface->resource()->client());
    wl_data_device::send_drop(resource->handle);
}

void DataDevice::sendLeave(Surface *surface)
{
    Resource *resource = resourceMap().value(surface->resource()->client());
    wl_data_device::send_leave(resource->handle);
}

void DataOffer::data_offer_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void DataOffer::data_offer_receive(Resource *resource, const QString &mime_type, int32_t fd)
{
    Q_UNUSED(resource);
    emit receive(mime_type, fd);
}

void DataOffer::data_offer_destroy(QtWaylandServer::wl_data_offer::Resource *resource)
{
    m_dataDevice->m_sentSelectionOffers.removeOne(this);
    wl_resource_destroy(resource->handle);
}

} // namespace MockCompositor
