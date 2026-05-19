// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MOCKCOMPOSITOR_DATADEVICE_H
#define MOCKCOMPOSITOR_DATADEVICE_H

//TODO: need this?
#include "coreprotocol.h"

namespace MockCompositor {

class DataOffer;

class DataDeviceManager : public Global, public QtWaylandServer::wl_data_device_manager
{
    Q_OBJECT
public:
    explicit DataDeviceManager(CoreCompositor *compositor, int version = 1)
        : QtWaylandServer::wl_data_device_manager(compositor->m_display, version)
        , m_version(version)
        , m_compositor(compositor)
    {}
    ~DataDeviceManager() override { qDeleteAll(m_dataDevices); }
    bool isClean() override;
    DataDevice *deviceFor(Seat *seat);

    int m_version = 1; // TODO: remove on libwayland upgrade
    QMap<Seat *, DataDevice *> m_dataDevices;
    CoreCompositor *m_compositor;

protected:
    void data_device_manager_get_data_device(Resource *resource, uint32_t id, ::wl_resource *seatResource) override;
    void data_device_manager_create_data_source(Resource *resource, uint32_t id) override;
};

class DataDevice : public QObject, public QtWaylandServer::wl_data_device
{
    Q_OBJECT
public:
    explicit DataDevice(DataDeviceManager *manager, Seat *seat)
        : m_manager(manager)
        , m_seat(seat)
    {}
    ~DataDevice() override;
    void send_data_offer(::wl_resource *resource) = delete;
    DataOffer *sendDataOffer(::wl_client *client, const QStringList &mimeTypes = {});

    void send_selection(::wl_resource *resource) = delete;
    void sendSelection(DataOffer *offer);

    void clearSelection(::wl_client *client);

    void send_enter(uint32_t serial, ::wl_resource *surface, wl_fixed_t x, wl_fixed_t y, ::wl_resource *id) = delete;
    void sendEnter(Surface *surface, const QPoint& position);

    void send_motion(uint32_t time, wl_fixed_t x, wl_fixed_t y) = delete;
    void sendMotion(Surface *surface, const QPoint &position);

    void send_drop(::wl_resource *resource) = delete;
    void sendDrop(Surface *surface);

    void send_leave(::wl_resource *resource) = delete;
    void sendLeave(Surface *surface);

    DataDeviceManager *m_manager = nullptr;
    Seat *m_seat = nullptr;
    QList<DataOffer *> m_sentSelectionOffers;
    QPointer<DataOffer> m_offer;

signals:
    void dragStarted();

protected:
    void data_device_start_drag(Resource *resource, ::wl_resource *source, ::wl_resource *origin, ::wl_resource *icon, uint32_t serial) override
    {
        Q_UNUSED(resource);
        Q_UNUSED(source);
        Q_UNUSED(origin);
        Q_UNUSED(icon);
        Q_UNUSED(serial);
        emit dragStarted();
    }

    void data_device_release(Resource *resource) override
    {
        int removed = m_manager->m_dataDevices.remove(m_seat);
        QVERIFY(removed);
        wl_resource_destroy(resource->handle);
    }
};

class DataOffer : public QObject, public QtWaylandServer::wl_data_offer
{
    Q_OBJECT
public:
    explicit DataOffer(DataDevice *dataDevice, ::wl_client *client, int version)
        : QtWaylandServer::wl_data_offer (client, 0, version)
        , m_dataDevice(dataDevice)
    {}

    DataDevice *m_dataDevice = nullptr;

signals:
    void receive(QString mimeType, int fd);

protected:
    void data_offer_destroy_resource(Resource *resource) override;
    void data_offer_receive(Resource *resource, const QString &mime_type, int32_t fd) override;
//    void data_offer_accept(Resource *resource, uint32_t serial, const QString &mime_type) override;
    void data_offer_destroy(Resource *resource) override;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_DATADEVICE_H
