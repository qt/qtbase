/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/QtTest>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/QNetworkConfigurationManager>
#endif

#include <QtCore/QDebug>

#ifndef QT_NO_BEARERMANAGEMENT
Q_DECLARE_METATYPE(QNetworkAccessManager::NetworkAccessibility)
#endif

class tst_QNetworkAccessManager : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkAccessManager();

private slots:
    void networkAccessible();
    void alwaysCacheRequest();
};

tst_QNetworkAccessManager::tst_QNetworkAccessManager()
{
}

void tst_QNetworkAccessManager::networkAccessible()
{
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkAccessManager manager;

    qRegisterMetaType<QNetworkAccessManager::NetworkAccessibility>("QNetworkAccessManager::NetworkAccessibility");

    QSignalSpy spy(&manager,
                   SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));

    // if there is no session, we cannot know in which state we are in
    QNetworkAccessManager::NetworkAccessibility initialAccessibility =
            manager.networkAccessible();

    if (initialAccessibility == QNetworkAccessManager::UnknownAccessibility)
          QSKIP("Unknown accessibility", SkipAll);

    QCOMPARE(manager.networkAccessible(), initialAccessibility);

    manager.setNetworkAccessible(QNetworkAccessManager::NotAccessible);

    int expectedCount = (initialAccessibility == QNetworkAccessManager::Accessible) ? 1 : 0;
    QCOMPARE(spy.count(), expectedCount);
    if (expectedCount > 0)
        QCOMPARE(spy.takeFirst().at(0).value<QNetworkAccessManager::NetworkAccessibility>(),
                 QNetworkAccessManager::NotAccessible);
    QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::NotAccessible);

    // When network is not accessible, all requests fail
    QNetworkReply *reply = manager.get(QNetworkRequest(QUrl("http://www.example.org")));
    QSignalSpy finishedSpy(reply, &QNetworkReply::finished);
    QSignalSpy errorSpy(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error));
    QVERIFY(finishedSpy.wait());
    QCOMPARE(reply->isFinished(), true);
    QCOMPARE(reply->errorString(), QStringLiteral("Network access is disabled."));
    QCOMPARE(errorSpy.count(), 1);

    manager.setNetworkAccessible(QNetworkAccessManager::Accessible);

    QCOMPARE(spy.count(), expectedCount);
    if (expectedCount > 0)
        QCOMPARE(spy.takeFirst().at(0).value<QNetworkAccessManager::NetworkAccessibility>(),
                 initialAccessibility);
    QCOMPARE(manager.networkAccessible(), initialAccessibility);

    QNetworkConfigurationManager configManager;
    QNetworkConfiguration defaultConfig = configManager.defaultConfiguration();
    if (defaultConfig.isValid()) {
        manager.setConfiguration(defaultConfig);

        QCOMPARE(spy.count(), 0);

        if (defaultConfig.state().testFlag(QNetworkConfiguration::Active))
            QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::Accessible);
        else
            QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::NotAccessible);

        manager.setNetworkAccessible(QNetworkAccessManager::NotAccessible);

        if (defaultConfig.state().testFlag(QNetworkConfiguration::Active)) {
            QCOMPARE(spy.count(), 1);
            QCOMPARE(QNetworkAccessManager::NetworkAccessibility(spy.takeFirst().at(0).toInt()),
                     QNetworkAccessManager::NotAccessible);
        } else {
            QCOMPARE(spy.count(), 0);
        }
    }
    QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::NotAccessible);
#endif
}

void tst_QNetworkAccessManager::alwaysCacheRequest()
{
    QNetworkAccessManager manager;

    QNetworkRequest req;
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
    QNetworkReply *reply = manager.get(req);
    reply->close();
    delete reply;
}

QTEST_MAIN(tst_QNetworkAccessManager)
#include "tst_qnetworkaccessmanager.moc"
