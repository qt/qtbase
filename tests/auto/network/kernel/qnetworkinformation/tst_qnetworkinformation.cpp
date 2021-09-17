/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtNetwork/private/qnetworkinformation_p.h>
#include <QtNetwork/qnetworkinformation.h>
#include <QtTest/qtest.h>

#include <limits>
#include <memory>

class MockFactory;
class tst_QNetworkInformation : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void reachability();
    void behindCaptivePortal();
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

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        return { QNetworkInformation::Feature::Reachability,
                 QNetworkInformation::Feature::CaptivePortal };
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
    QVERIFY(QNetworkInformation::load(u"mock"));
    QVERIFY(QNetworkInformation::load(u"mock"));
    QVERIFY(QNetworkInformation::load(u"mOcK"));
    QVERIFY(!QNetworkInformation::load(u"mocks"));
}

void tst_QNetworkInformation::cleanupTestCase()
{
    // Make sure the factory gets unregistered on destruction:
    mockFactory.reset();
    auto backends = QNetworkInformation::availableBackends();
    QVERIFY(!backends.contains(u"mock"));
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

QTEST_MAIN(tst_QNetworkInformation);
#include "tst_qnetworkinformation.moc"
