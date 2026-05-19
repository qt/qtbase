// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"

#include <qwayland-server-wp-primary-selection-unstable-v1.h>

#include <QtGui/QRasterWindow>
#include <QtGui/QClipboard>
#include <QtCore/private/qcore_unix_p.h>

#include <fcntl.h>

using namespace MockCompositor;

constexpr int primarySelectionVersion = 1; // protocol VERSION, not the name suffix (_v1)

class PrimarySelectionDeviceV1;
class PrimarySelectionDeviceManagerV1;

class PrimarySelectionOfferV1 : public QObject, public QtWaylandServer::zwp_primary_selection_offer_v1
{
    Q_OBJECT
public:
    explicit PrimarySelectionOfferV1(PrimarySelectionDeviceV1 *device, wl_client *client, int version)
        : zwp_primary_selection_offer_v1(client, 0, version)
        , m_device(device)
    {}
    void send_offer() = delete;
    void sendOffer(const QString &offer)
    {
        zwp_primary_selection_offer_v1::send_offer(offer);
        m_mimeTypes << offer;
    }

    PrimarySelectionDeviceV1 *m_device = nullptr;
    QStringList m_mimeTypes;

signals:
    void receive(QString mimeType, int fd);

protected:
    void zwp_primary_selection_offer_v1_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource);
        delete this;
    }

    void zwp_primary_selection_offer_v1_receive(Resource *resource, const QString &mime_type, int32_t fd) override
    {
        Q_UNUSED(resource);
        QTRY_VERIFY(m_mimeTypes.contains(mime_type));
        emit receive(mime_type, fd);
    }

    void zwp_primary_selection_offer_v1_destroy(Resource *resource) override;
};

class PrimarySelectionSourceV1 : public QObject, public QtWaylandServer::zwp_primary_selection_source_v1
{
    Q_OBJECT
public:
    explicit PrimarySelectionSourceV1(wl_client *client, int id, int version)
        : zwp_primary_selection_source_v1(client, id, version)
    {
    }
    QStringList m_offers;
protected:
    void zwp_primary_selection_source_v1_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource);
        delete this;
    }
    void zwp_primary_selection_source_v1_offer(Resource *resource, const QString &mime_type) override
    {
        Q_UNUSED(resource);
        m_offers << mime_type;
    }
    void zwp_primary_selection_source_v1_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }
};

class PrimarySelectionDeviceV1 : public QObject, public QtWaylandServer::zwp_primary_selection_device_v1
{
    Q_OBJECT
public:
    explicit PrimarySelectionDeviceV1(PrimarySelectionDeviceManagerV1 *manager, Seat *seat)
        : m_manager(manager)
        , m_seat(seat)
    {}

    void send_data_offer(::wl_resource *resource) = delete;

    PrimarySelectionOfferV1 *sendDataOffer(::wl_client *client, const QStringList &mimeTypes = {});

    PrimarySelectionOfferV1 *sendDataOffer(const QStringList &mimeTypes = {}) // creates a new offer for the focused surface and sends it
    {
        Q_ASSERT(m_seat->m_capabilities & Seat::capability_keyboard);
        Q_ASSERT(m_seat->m_keyboard->m_enteredSurface);
        auto *client = m_seat->m_keyboard->m_enteredSurface->resource()->client();
        return sendDataOffer(client, mimeTypes);
    }

    void send_selection(::wl_resource *resource) = delete;
    void sendSelection(PrimarySelectionOfferV1 *offer)
    {
        auto *client = offer->resource()->client();
        for (auto *resource : resourceMap().values(client))
            zwp_primary_selection_device_v1::send_selection(resource->handle, offer->resource()->handle);
        m_sentSelectionOffers << offer;
    }
    void clearSelection(wl_client *client)
    {
        for (auto *resource : resourceMap().values(client))
            zwp_primary_selection_device_v1::send_selection(resource->handle, nullptr);
    }


    PrimarySelectionDeviceManagerV1 *m_manager = nullptr;
    Seat *m_seat = nullptr;
    QList<PrimarySelectionOfferV1 *> m_sentSelectionOffers;
    PrimarySelectionSourceV1 *m_selectionSource = nullptr;
    uint m_serial = 0;

protected:
    void zwp_primary_selection_device_v1_set_selection(Resource *resource, ::wl_resource *source, uint32_t serial) override
    {
        Q_UNUSED(resource);
        m_selectionSource = fromResource<PrimarySelectionSourceV1>(source);
        m_serial = serial;
    }
    void zwp_primary_selection_device_v1_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }
};

class PrimarySelectionDeviceManagerV1 : public Global, public QtWaylandServer::zwp_primary_selection_device_manager_v1
{
    Q_OBJECT
public:
    explicit PrimarySelectionDeviceManagerV1(CoreCompositor *compositor, int version = 1)
        : QtWaylandServer::zwp_primary_selection_device_manager_v1(compositor->m_display, version)
        , m_version(version)
    {}
    ~PrimarySelectionDeviceManagerV1() override
    {
        qDeleteAll(m_devices);
    }
    bool isClean() override
    {
        for (auto *device : std::as_const(m_devices)) {
            // The client should not leak selection offers, i.e. if this fails, there is a missing
            // zwp_primary_selection_offer_v1.destroy request
            if (!device->m_sentSelectionOffers.empty())
                return false;
        }
        return true;
    }

    PrimarySelectionDeviceV1 *deviceFor(Seat *seat)
    {
        Q_ASSERT(seat);
        if (auto *device = m_devices.value(seat, nullptr))
            return device;

        auto *device = new PrimarySelectionDeviceV1(this, seat);
        m_devices[seat] = device;
        return device;
    }

    int m_version = 1; // TODO: Remove on libwayland upgrade
    QMap<Seat *, PrimarySelectionDeviceV1 *> m_devices;
    QList<PrimarySelectionSourceV1 *> m_sources;
protected:
    void zwp_primary_selection_device_manager_v1_destroy(Resource *resource) override
    {
        // The protocol doesn't say whether managed objects should be destroyed as well,
        // so leave them alone, they'll be cleaned up in the destructor anyway
        wl_resource_destroy(resource->handle);
    }

    void zwp_primary_selection_device_manager_v1_create_source(Resource *resource, uint32_t id) override
    {
        int version = m_version;
        m_sources << new PrimarySelectionSourceV1(resource->client(), id, version);
    }
    void zwp_primary_selection_device_manager_v1_get_device(Resource *resource, uint32_t id, ::wl_resource *seatResource) override
    {
        auto *seat = fromResource<Seat>(seatResource);
        QVERIFY(seat);
        auto *device = deviceFor(seat);
        device->add(resource->client(), id, resource->version());
    }
};

PrimarySelectionOfferV1 *PrimarySelectionDeviceV1::sendDataOffer(wl_client *client, const QStringList &mimeTypes)
{
    Q_ASSERT(client);
    auto *offer = new PrimarySelectionOfferV1(this, client, m_manager->m_version);
    for (auto *resource : resourceMap().values(client))
        zwp_primary_selection_device_v1::send_data_offer(resource->handle, offer->resource()->handle);
    for (const auto &mimeType : mimeTypes)
        offer->sendOffer(mimeType);
    return offer;
}

void PrimarySelectionOfferV1::zwp_primary_selection_offer_v1_destroy(QtWaylandServer::zwp_primary_selection_offer_v1::Resource *resource)
{
    bool removed = m_device->m_sentSelectionOffers.removeOne(this);
    QVERIFY(removed);
    wl_resource_destroy(resource->handle);
}

class PrimarySelectionCompositor : public DefaultCompositor {
public:
    explicit PrimarySelectionCompositor()
    {
        exec([this] {
            m_config.autoConfigure = true;
            add<PrimarySelectionDeviceManagerV1>(primarySelectionVersion);
        });
    }
    PrimarySelectionDeviceV1 *primarySelectionDevice(int i = 0) {
        return get<PrimarySelectionDeviceManagerV1>()->deviceFor(get<Seat>(i));
    }
};

class tst_primaryselectionv1 : public QObject, private PrimarySelectionCompositor
{
    Q_OBJECT
private slots:
    void cleanup() {
        exec([&] {
            primarySelectionDevice()->clearSelection(client());
        });
        QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
    }
    void initTestCase();
    void bindsToManager();
    void createsPrimaryDevice();
    void createsPrimaryDeviceForNewSeats();
    void pasteAscii();
    void pasteUtf8();
    void destroysPreviousSelection();
    void copy();
};

void tst_primaryselectionv1::initTestCase()
{
    QCOMPOSITOR_TRY_VERIFY(pointer());
    QCOMPOSITOR_TRY_VERIFY(keyboard());
}

void tst_primaryselectionv1::bindsToManager()
{
    QCOMPOSITOR_TRY_COMPARE(get<PrimarySelectionDeviceManagerV1>()->resourceMap().size(), 1);
    QCOMPOSITOR_TRY_COMPARE(get<PrimarySelectionDeviceManagerV1>()->resourceMap().first()->version(), primarySelectionVersion);
}

void tst_primaryselectionv1::createsPrimaryDevice()
{
    QCOMPOSITOR_TRY_VERIFY(primarySelectionDevice());
    QCOMPOSITOR_TRY_VERIFY(primarySelectionDevice()->resourceMap().contains(client()));
    QCOMPOSITOR_TRY_COMPARE(primarySelectionDevice()->resourceMap().value(client())->version(), primarySelectionVersion);
    QTRY_VERIFY(QGuiApplication::clipboard()->supportsSelection());
}

void tst_primaryselectionv1::createsPrimaryDeviceForNewSeats()
{
    exec([&] { add<Seat>(); });
    QCOMPOSITOR_TRY_VERIFY(primarySelectionDevice(1));
}

void tst_primaryselectionv1::pasteAscii()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override
        {
            Q_UNUSED(event);
            auto *mimeData = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
            m_formats = mimeData->formats();
            m_text = QGuiApplication::clipboard()->text(QClipboard::Selection);
        }
        QStringList m_formats;
        QString m_text;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        auto *device = primarySelectionDevice();
        auto *offer = device->sendDataOffer({"text/plain"});
        connect(offer, &PrimarySelectionOfferV1::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/plain");
            file.write(QByteArray("normal ascii"));
            file.close();
        }, Qt::DirectConnection);
        device->sendSelection(offer);

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_MIDDLE, 1);
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_MIDDLE, 0);
        pointer()->sendFrame(client());
    });
    QTRY_COMPARE(window.m_formats, QStringList{"text/plain"});
    QTRY_COMPARE(window.m_text, "normal ascii");
}

void tst_primaryselectionv1::pasteUtf8()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override
        {
            Q_UNUSED(event);
            auto *mimeData = QGuiApplication::clipboard()->mimeData(QClipboard::Selection);
            m_formats = mimeData->formats();
            m_text = QGuiApplication::clipboard()->text(QClipboard::Selection);
        }
        QStringList m_formats;
        QString m_text;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        auto *device = primarySelectionDevice();
        auto *offer = device->sendDataOffer({"text/plain", "text/plain;charset=utf-8"});
        connect(offer, &PrimarySelectionOfferV1::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/plain;charset=utf-8");
            file.write(QByteArray("face with tears of joy: 😂"));
            file.close();
        }, Qt::DirectConnection);
        device->sendSelection(offer);

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_MIDDLE, 1);
        pointer()->sendFrame(client());
        pointer()->sendButton(client(), BTN_MIDDLE, 0);
        pointer()->sendFrame(client());
    });
    QTRY_COMPARE(window.m_formats, QStringList({"text/plain"}));
    QTRY_COMPARE(window.m_text, "face with tears of joy: 😂");
}

void tst_primaryselectionv1::destroysPreviousSelection()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    // When the client receives a selection event, it is required to destroy the previous offer
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        auto *offer = primarySelectionDevice()->sendDataOffer({"text/plain"});
        primarySelectionDevice()->sendSelection(offer);
    });

    exec([&] {
        auto *offer = primarySelectionDevice()->sendDataOffer({"text/plain"});
        primarySelectionDevice()->sendSelection(offer);
        QCOMPARE(primarySelectionDevice()->m_sentSelectionOffers.size(), 2);
    });

    // Verify the first offer gets destroyed
    QCOMPOSITOR_TRY_COMPARE(primarySelectionDevice()->m_sentSelectionOffers.size(), 1);
}

void tst_primaryselectionv1::copy()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *event) override
        {
            Q_UNUSED(event);
            QGuiApplication::clipboard()->setText("face with tears of joy: 😂", QClipboard::Selection);
        }
        QStringList m_formats;
        QString m_text;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    QList<uint> mouseSerials;
    exec([&] {
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol
        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client());
        mouseSerials << pointer()->sendButton(client(), BTN_MIDDLE, 1);
        pointer()->sendFrame(client());
        mouseSerials << pointer()->sendButton(client(), BTN_MIDDLE, 0);
        pointer()->sendFrame(client());
    });
    QCOMPOSITOR_TRY_VERIFY(primarySelectionDevice()->m_selectionSource);
    QCOMPOSITOR_TRY_VERIFY(mouseSerials.contains(primarySelectionDevice()->m_serial));
    QVERIFY(QGuiApplication::clipboard()->ownsSelection());
    QByteArray pastedBuf;
    exec([&](){
        auto *source = primarySelectionDevice()->m_selectionSource;
        QCOMPARE(source->m_offers, QStringList({"text/plain", "text/plain;charset=utf-8"}));
        int fd[2];
        if (pipe(fd) == -1)
            QSKIP("Failed to create pipe");
        fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL, 0) | O_NONBLOCK);
        source->send_send("text/plain;charset=utf-8", fd[1]);
        auto *notifier = new QSocketNotifier(fd[0], QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated, this, [&](int fd) {
            exec([&]{
                static char buf[1024];
                int n = QT_READ(fd, buf, sizeof buf);
                if (n <= 0) {
                    delete notifier;
                    close(fd);
                } else {
                    pastedBuf.append(buf, n);
                }
            });
        }, Qt::DirectConnection);
    });

    QCOMPOSITOR_TRY_VERIFY(pastedBuf.size()); // this assumes we got everything in one read
    auto pasted = QString::fromUtf8(pastedBuf);
    QCOMPARE(pasted, "face with tears of joy: 😂");
}

QCOMPOSITOR_TEST_MAIN(tst_primaryselectionv1)
#include "tst_primaryselectionv1.moc"
