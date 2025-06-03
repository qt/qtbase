// Copyright (C) 2021 David Redondo <qt@david-redondo.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QSignalSpy>
#include <QtGui/private/qguiapplication_p.h>
#include <QtWaylandClient/private/qwayland-wayland.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <QtWaylandClient/qwaylandclientextension.h>
#include <qwayland-server-test.h>
#include <qwayland-test.h>
#include "mockcompositor.h"
#include "coreprotocol.h"

using namespace MockCompositor;

class TestExtension
    : public QWaylandClientExtensionTemplate<TestExtension, &QtWayland::test_interface::release>,
      public QtWayland::test_interface
{
public:
    TestExtension() : QWaylandClientExtensionTemplate(1){};
    void initialize() { QWaylandClientExtension::initialize(); }
};

class TestGlobal : public Global, public QtWaylandServer::test_interface
{
    Q_OBJECT
public:
    explicit TestGlobal(CoreCompositor *compositor)
        : QtWaylandServer::test_interface(compositor->m_display, 1)
    {
    }
};

class tst_clientextension : public QObject, private CoreCompositor
{
    Q_OBJECT
private:
    QtWaylandClient::QWaylandDisplay *display()
    {
        return static_cast<QtWaylandClient::QWaylandIntegration *>(
                       QGuiApplicationPrivate::platformIntegration())
                ->display();
    }
private slots:
    void cleanup()
    {
        display()->flushRequests();
        dispatch();
        exec([this] { removeAll<TestGlobal>(); });
        QTRY_COMPARE(display()->globals().size(), 0);
    }
    void createWithoutGlobal();
    void createWithGlobalAutomatic();
    void createWithGlobalManual();
    void globalBecomesAvailable();
    void globalRemoved();
};

void tst_clientextension::createWithoutGlobal()
{
    TestExtension extension;
    QSignalSpy spy(&extension, &QWaylandClientExtension::activeChanged);
    QVERIFY(spy.isValid());
    QVERIFY(!extension.isActive());
    QCOMPARE(spy.size(), 0);
    extension.initialize();
    QVERIFY(!extension.isActive());
    QCOMPARE(spy.size(), 0);
}

void tst_clientextension::createWithGlobalAutomatic()
{
    exec([this] { add<TestGlobal>(); });
    QTRY_COMPARE(display()->globals().size(), 1);
    TestExtension extension;
    QSignalSpy spy(&extension, &QWaylandClientExtension::activeChanged);
    QVERIFY(spy.isValid());
    QTRY_VERIFY(extension.isActive());
    QCOMPARE(spy.size(), 1);
}

void tst_clientextension::createWithGlobalManual()
{
    exec([this] { add<TestGlobal>(); });
    QTRY_COMPARE(display()->globals().size(), 1);
    // Wait for the display to have the global
    TestExtension extension;
    QSignalSpy spy(&extension, &QWaylandClientExtension::activeChanged);
    QVERIFY(spy.isValid());
    extension.initialize();
    QVERIFY(extension.isActive());
    QCOMPARE(spy.size(), 1);
}

void tst_clientextension::globalBecomesAvailable()
{
    TestExtension extension;
    QSignalSpy spy(&extension, &QWaylandClientExtension::activeChanged);
    QVERIFY(spy.isValid());
    exec([this] { add<TestGlobal>(); });
    QTRY_VERIFY(extension.isActive());
    QCOMPARE(spy.size(), 1);
}

void tst_clientextension::globalRemoved()
{
    exec([this] { add<TestGlobal>(); });
    TestExtension extension;
    QTRY_VERIFY(extension.isActive());
    QSignalSpy spy(&extension, &QWaylandClientExtension::activeChanged);
    QVERIFY(spy.isValid());
    QCOMPOSITOR_TRY_COMPARE(get<TestGlobal>()->resourceMap().size(), 1);

    exec([this] { removeAll<TestGlobal>(); });
    QTRY_VERIFY(!extension.isActive());
    QCOMPARE(spy.size(), 1);
}

int main(int argc, char **argv)
{
    QTemporaryDir tmpRuntimeDir;
    qputenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit());
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_DONT_CHECK_SHELL_INTEGRATION", "1");

    tst_clientextension tc;
    QGuiApplication app(argc, argv);
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_clientextension.moc"
