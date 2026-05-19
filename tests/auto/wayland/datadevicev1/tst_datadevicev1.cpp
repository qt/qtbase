// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mockcompositor.h"

#include <QtGui/QRasterWindow>
#include <QtGui/QClipboard>
#include <QtGui/QDrag>

using namespace MockCompositor;

constexpr int dataDeviceVersion = 3;

class DataDeviceCompositor : public DefaultCompositor {
public:
    explicit DataDeviceCompositor()
    {
        exec([this] {
            m_config.autoConfigure = true;
            add<DataDeviceManager>(dataDeviceVersion);
        });
    }
    DataDevice *dataDevice() { return get<DataDeviceManager>()->deviceFor(get<Seat>()); }
};

class tst_datadevicev1 : public QObject, private DataDeviceCompositor
{
    Q_OBJECT
private slots:
    void cleanup() {
        exec([&] {
            dataDevice()->clearSelection(client());
        });
        QTRY_VERIFY2(isClean(), qPrintable(dirtyMessage()));
    }
    void initTestCase();
    void pasteAscii();
    void pasteUtf8();
    void pasteMozUrl();
    void pasteSingleUtf8MozUrl();
    void destroysPreviousSelection();
    void dragWithoutFocus();
};

void tst_datadevicev1::initTestCase()
{
    QCOMPOSITOR_TRY_VERIFY(pointer());
    QCOMPOSITOR_TRY_VERIFY(keyboard());

    QCOMPOSITOR_TRY_VERIFY(dataDevice());
    QCOMPOSITOR_TRY_VERIFY(dataDevice()->resourceMap().contains(client()));
    QCOMPOSITOR_TRY_COMPARE(dataDevice()->resourceMap().value(client())->version(), dataDeviceVersion);
}

void tst_datadevicev1::pasteAscii()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *) override { m_text = QGuiApplication::clipboard()->text(); }
        QString m_text;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *client = xdgSurface()->resource()->client();
        auto *offer = dataDevice()->sendDataOffer(client, {"text/plain"});
        connect(offer, &DataOffer::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/plain");
            file.write(QByteArray("normal ascii"));
            file.close();
        }, Qt::DirectConnection);
        dataDevice()->sendSelection(offer);

        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 1);
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 0);
        pointer()->sendFrame(client);
    });
    QTRY_COMPARE(window.m_text, "normal ascii");
}

void tst_datadevicev1::pasteUtf8()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *) override { m_text = QGuiApplication::clipboard()->text(); }
        QString m_text;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *client = xdgSurface()->resource()->client();
        auto *offer = dataDevice()->sendDataOffer(client, {"text/plain", "text/plain;charset=utf-8"});
        connect(offer, &DataOffer::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/plain;charset=utf-8");
            file.write(QByteArray("face with tears of joy: 😂"));
            file.close();
        }, Qt::DirectConnection);
        dataDevice()->sendSelection(offer);

        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 1);
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 0);
        pointer()->sendFrame(client);
    });
    QTRY_COMPARE(window.m_text, "face with tears of joy: 😂");
}

void tst_datadevicev1::pasteMozUrl()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *) override { m_urls = QGuiApplication::clipboard()->mimeData()->urls(); }
        QList<QUrl> m_urls;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *client = xdgSurface()->resource()->client();
        auto *offer = dataDevice()->sendDataOffer(client, {"text/x-moz-url"});
        connect(offer, &DataOffer::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/x-moz-url");
            const QString content("https://www.qt.io/\nQt\nhttps://www.example.com/\nExample Website");
            // Need UTF-16.
            file.write(reinterpret_cast<const char *>(content.data()), content.size() * 2);
            file.close();
        }, Qt::DirectConnection);
        dataDevice()->sendSelection(offer);

        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 1);
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 0);
        pointer()->sendFrame(client);
    });

    QTRY_COMPARE(window.m_urls.count(), 2);
    QCOMPARE(window.m_urls.at(0), QUrl("https://www.qt.io/"));
    QCOMPARE(window.m_urls.at(1), QUrl("https://www.example.com/"));
}

void tst_datadevicev1::pasteSingleUtf8MozUrl()
{
    class Window : public QRasterWindow {
    public:
        void mousePressEvent(QMouseEvent *) override { m_urls = QGuiApplication::clipboard()->mimeData()->urls(); }
        QList<QUrl> m_urls;
    };

    Window window;
    window.resize(64, 64);
    window.show();

    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);
    exec([&] {
        auto *client = xdgSurface()->resource()->client();
        auto *offer = dataDevice()->sendDataOffer(client, {"text/x-moz-url"});
        connect(offer, &DataOffer::receive, offer, [](QString mimeType, int fd) {
            QFile file;
            QVERIFY(file.open(fd, QIODevice::WriteOnly, QFile::FileHandleFlag::AutoCloseHandle));
            QCOMPARE(mimeType, "text/x-moz-url");
            const QString content("https://www.qt.io/");
            file.write(content.toUtf8());
            file.close();
        }, Qt::DirectConnection);
        dataDevice()->sendSelection(offer);

        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol

        pointer()->sendEnter(surface, {32, 32});
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 1);
        pointer()->sendFrame(client);
        pointer()->sendButton(client, BTN_LEFT, 0);
        pointer()->sendFrame(client);
    });

    QTRY_COMPARE(window.m_urls.count(), 1);
    QCOMPARE(window.m_urls.at(0), QUrl("https://www.qt.io/"));
}

void tst_datadevicev1::destroysPreviousSelection()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    // When the client receives a selection event, it is required to destroy the previous offer
    exec([&] {
        QCOMPARE(dataDevice()->m_sentSelectionOffers.size(), 0);
        auto *offer = dataDevice()->sendDataOffer(client(), {"text/plain"});
        dataDevice()->sendSelection(offer);
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendEnter(surface); // Need to set keyboard focus according to protocol
        QCOMPARE(dataDevice()->m_sentSelectionOffers.size(), 1);
    });

    exec([&] {
        auto *offer = dataDevice()->sendDataOffer(client(), {"text/plain"});
        dataDevice()->sendSelection(offer);
        QCOMPARE(dataDevice()->m_sentSelectionOffers.size(), 2);
    });

    // Verify the first offer gets destroyed
    QCOMPOSITOR_TRY_COMPARE(dataDevice()->m_sentSelectionOffers.size(), 1);

    exec([&] {
        auto *offer = dataDevice()->sendDataOffer(client(), {"text/plain"});
        dataDevice()->sendSelection(offer);
        auto *surface = xdgSurface()->m_surface;
        keyboard()->sendLeave(surface);
    });
}

// The application should not crash if it attempts to start a drag operation
// when it doesn't have input focus (QTBUG-76368)
void tst_datadevicev1::dragWithoutFocus()
{
    QRasterWindow window;
    window.resize(64, 64);
    window.show();
    QCOMPOSITOR_TRY_VERIFY(xdgSurface() && xdgSurface()->m_committedConfigureSerial);

    auto *mimeData = new QMimeData;
    const QByteArray data("testData");
    mimeData->setData("text/plain", data);
    QDrag drag(&window);
    drag.setMimeData(mimeData);
    drag.exec();
}

QCOMPOSITOR_TEST_MAIN(tst_datadevicev1)
#include "tst_datadevicev1.moc"
