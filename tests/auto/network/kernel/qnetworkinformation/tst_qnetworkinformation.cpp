// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtNetwork/private/qnetworkinformation_p.h>
#include <QtNetwork/qnetworkinformation.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <limits>
#include <memory>

class MockFactory;
class tst_QNetworkInformation : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void supportedFeatures();
    void reachability();
    void behindCaptivePortal();
    void transportMedium();
    void isMetered();
    void cleanupTestCase();

private:
    std::unique_ptr<MockFactory> mockFactory;
};

static const QString mockName = QStringLiteral("mock");
class MockBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    MockBackend()
    {
        Q_ASSERT(!instance);
        instance = this;
        setReachability(QNetworkInformation::Reachability::Online);
        setNewBehindCaptivePortal(false);
    }
    ~MockBackend() { instance = nullptr; }

    QString name() const override { return mockName; }

    QNetworkInformation::Features featuresSupported() const override
    {
        return featuresSupportedStatic();
    }

    static void setNewReachability(QNetworkInformation::Reachability value)
    {
        Q_ASSERT(instance);
        instance->setReachability(value);
    }

    static void setNewBehindCaptivePortal(bool value)
    {
        Q_ASSERT(instance);
        instance->setBehindCaptivePortal(value);
    }

    static void setNewTransportMedium(QNetworkInformation::TransportMedium medium)
    {
        Q_ASSERT(instance);
        instance->setTransportMedium(medium);
    }

    static void setNewMetered(bool metered)
    {
        Q_ASSERT(instance);
        instance->setMetered(metered);
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return { QNetworkInformation::Feature::Reachability
                 | QNetworkInformation::Feature::CaptivePortal
                 | QNetworkInformation::Feature::TransportMedium
                 | QNetworkInformation::Feature::Metered };
    }

private:
    static inline MockBackend *instance = nullptr;
};

class MockFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
public:
    QString name() const override { return mockName; }
    QNetworkInformationBackend *
    create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        return new MockBackend();
    }
    QNetworkInformation::Features featuresSupported() const override
    {
        return MockBackend::featuresSupportedStatic();
    }
};

void tst_QNetworkInformation::initTestCase()
{
    auto prevBackends = QNetworkInformation::availableBackends();
    qDebug() << "available backends:" << prevBackends;
    // Creating the factory registers it as a backend
    mockFactory = std::make_unique<MockFactory>();
    auto backends = QNetworkInformation::availableBackends();
    QVERIFY(backends.size() > prevBackends.size());
    QVERIFY(backends.contains(u"mock"));
    QVERIFY(QNetworkInformation::loadBackendByName(u"mock"));
    QVERIFY(QNetworkInformation::loadBackendByName(u"mock"));
    QVERIFY(QNetworkInformation::loadBackendByName(u"mOcK"));
    QVERIFY(!QNetworkInformation::loadBackendByName(u"mocks"));
}

void tst_QNetworkInformation::cleanupTestCase()
{
    // Make sure the factory gets unregistered on destruction:
    mockFactory.reset();
    auto backends = QNetworkInformation::availableBackends();
    QVERIFY(!backends.contains(u"mock"));
}

void tst_QNetworkInformation::supportedFeatures()
{
    auto info = QNetworkInformation::instance();

    auto allFeatures = QNetworkInformation::Features(QNetworkInformation::Feature::CaptivePortal
                                                     | QNetworkInformation::Feature::Reachability
                                                     | QNetworkInformation::Feature::TransportMedium
                                                     | QNetworkInformation::Feature::Metered);

    QCOMPARE(info->supportedFeatures(), allFeatures);

    QVERIFY(info->supports(allFeatures));
    QVERIFY(info->supports(QNetworkInformation::Feature::CaptivePortal));
    QVERIFY(info->supports(QNetworkInformation::Feature::Reachability));
    QVERIFY(info->supports(QNetworkInformation::Feature::TransportMedium));
    QVERIFY(info->supports(QNetworkInformation::Feature::Metered));
}

void tst_QNetworkInformation::reachability()
{
    auto info = QNetworkInformation::instance();
    QNetworkInformation::Reachability boundIsOnline = QNetworkInformation::Reachability::Unknown;
    bool signalEmitted = false;

    connect(info, &QNetworkInformation::reachabilityChanged, this, [&, info]() {
        signalEmitted = true;
        boundIsOnline = info->reachability();
    });
    QCOMPARE(info->reachability(), QNetworkInformation::Reachability::Online);
    MockBackend::setNewReachability(QNetworkInformation::Reachability::Disconnected);
    QCoreApplication::processEvents();
    QVERIFY(signalEmitted);
    QCOMPARE(info->reachability(), QNetworkInformation::Reachability::Disconnected);
    QCOMPARE(boundIsOnline, QNetworkInformation::Reachability::Disconnected);

    // Set the same value again, signal should not be emitted again
    signalEmitted = false;
    MockBackend::setNewReachability(QNetworkInformation::Reachability::Disconnected);
    QCoreApplication::processEvents();
    QVERIFY(!signalEmitted);

    MockBackend::setNewReachability(QNetworkInformation::Reachability::Local);
    QCOMPARE(info->reachability(), QNetworkInformation::Reachability::Local);
    QCOMPARE(boundIsOnline, QNetworkInformation::Reachability::Local);
    MockBackend::setNewReachability(QNetworkInformation::Reachability::Site);
    QCOMPARE(info->reachability(), QNetworkInformation::Reachability::Site);
    QCOMPARE(boundIsOnline, QNetworkInformation::Reachability::Site);
}

void tst_QNetworkInformation::behindCaptivePortal()
{
    auto info = QNetworkInformation::instance();
    bool behindPortal = false;
    bool signalEmitted = false;

    connect(info, &QNetworkInformation::isBehindCaptivePortalChanged, this,
            [&, info](bool state) {
                signalEmitted = true;
                QCOMPARE(state, info->isBehindCaptivePortal());
                behindPortal = info->isBehindCaptivePortal();
            });
    QVERIFY(!info->isBehindCaptivePortal());
    MockBackend::setNewBehindCaptivePortal(true);
    QCoreApplication::processEvents();
    QVERIFY(signalEmitted);
    QVERIFY(info->isBehindCaptivePortal());
    QVERIFY(behindPortal);

    // Set the same value again, signal should not be emitted again
    signalEmitted = false;
    MockBackend::setNewBehindCaptivePortal(true);
    QCoreApplication::processEvents();
    QVERIFY(!signalEmitted);
}

void tst_QNetworkInformation::transportMedium()
{
    auto info = QNetworkInformation::instance();
    using TransportMedium = QNetworkInformation::TransportMedium;
    TransportMedium medium = TransportMedium::Unknown;
    bool signalEmitted = false;

    connect(info, &QNetworkInformation::transportMediumChanged, this, [&, info](TransportMedium tm) {
        signalEmitted = true;
        QCOMPARE(tm, info->transportMedium());
        medium = info->transportMedium();
    });
    QCOMPARE(info->transportMedium(), TransportMedium::Unknown); // Default is unknown

    auto transportMediumEnum = QMetaEnum::fromType<TransportMedium>();
    auto mediumCount = transportMediumEnum.keyCount();
    // Verify index 0 is Unknown and skip it in the loop, it's the default.
    QCOMPARE(TransportMedium(transportMediumEnum.value(0)), TransportMedium::Unknown);
    for (int i = 1; i < mediumCount; ++i) {
        signalEmitted = false;
        TransportMedium m = TransportMedium(transportMediumEnum.value(i));
        MockBackend::setNewTransportMedium(m);
        QCoreApplication::processEvents();
        QVERIFY(signalEmitted);
        QCOMPARE(info->transportMedium(), m);
        QCOMPARE(medium, m);
    }

    // Set the current value again, signal should not be emitted again
    signalEmitted = false;
    MockBackend::setNewTransportMedium(medium);
    QCoreApplication::processEvents();
    QVERIFY(!signalEmitted);
}

void tst_QNetworkInformation::isMetered()
{
    auto info = QNetworkInformation::instance();

    QSignalSpy spy(info, &QNetworkInformation::isMeteredChanged);
    QVERIFY(!info->isMetered());
    MockBackend::setNewMetered(true);
    QCOMPARE(spy.size(), 1);
    QVERIFY(info->isMetered());
    QVERIFY(spy[0][0].toBool());
    spy.clear();

    // Set the same value again, signal should not be emitted again
    MockBackend::setNewMetered(true);
    QCOMPARE(spy.size(), 0);
}

QTEST_MAIN(tst_QNetworkInformation);
#include "tst_qnetworkinformation.moc"
