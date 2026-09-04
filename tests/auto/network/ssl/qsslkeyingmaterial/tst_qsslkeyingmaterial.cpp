// Copyright (C) 2026 Governikus GmbH & Co. KG.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtNetwork/qtnetworkglobal.h>

#if QT_CONFIG(ssl)
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslKeyingMaterial>
#include <QSslServer>
#include <QSslSocket>
#endif // ssl

#include <QSignalSpy>

class tst_QSslKeyingMaterial : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void construction();
    void cloning();
    void sharedConfigurationPrivate();
#if QT_CONFIG(ssl)
    void initTestCase();
    void exporterProducesSameMaterialOnBothSides();

private:
    QSslConfiguration serverConfig();
    QString testDataDir;
#endif // Feature 'ssl'.
};


void tst_QSslKeyingMaterial::construction()
{
    QSslKeyingMaterial def;
    QVERIFY(def.label().isNull());
    QVERIFY(def.context().isNull());
    QVERIFY(def.value().isNull());
    QCOMPARE(def.requestedSize(), 0);
    QVERIFY(!def.isValid());

    QSslKeyingMaterial entry(QByteArray(), 1);
    QCOMPARE(entry.label(), QByteArray());
    QVERIFY(entry.context().isNull());
    QCOMPARE(entry.requestedSize(), 1);
    QVERIFY(entry.value().isNull());
    QVERIFY(!entry.isValid());

    entry = QSslKeyingMaterial(QByteArray("dummy"), 0);
    QCOMPARE(entry.label(), QByteArray("dummy"));
    QVERIFY(entry.context().isNull());
    QCOMPARE(entry.requestedSize(), 0);
    QVERIFY(entry.value().isNull());
    QVERIFY(!entry.isValid());

    entry = QSslKeyingMaterial(QByteArray("dummy"), 256);
    QCOMPARE(entry.label(), QByteArray("dummy"));
    QVERIFY(entry.context().isNull());
    QCOMPARE(entry.requestedSize(), 256);
    QVERIFY(entry.value().isNull());
    QVERIFY(entry.isValid());

    entry = QSslKeyingMaterial(QByteArray("dummy"), 256, QByteArray(""));
    QCOMPARE(entry.label(), QByteArray("dummy"));
    QVERIFY(!entry.context().isNull());
    QVERIFY(entry.context().isEmpty());
    QCOMPARE(entry.requestedSize(), 256);
    QVERIFY(entry.value().isNull());
    QVERIFY(entry.isValid());

    entry = QSslKeyingMaterial(QByteArray("dummy"), 256, QByteArray("ctx"));
    QCOMPARE(entry.label(), QByteArray("dummy"));
    QCOMPARE(entry.context(), QByteArray("ctx"));
    QCOMPARE(entry.requestedSize(), 256);
    QVERIFY(entry.value().isNull());
    QVERIFY(entry.isValid());

    QSslKeyingMaterial entry2(QByteArray("dummy"), 0);
    QCOMPARE_NE(entry, entry2);
    entry2 = QSslKeyingMaterial(QByteArray("dummy"), 256, QByteArray("ctx"));
    QCOMPARE(entry, entry2);
    entry2 = QSslKeyingMaterial(QByteArray("dummy"), 0, QByteArray("ctx"));
    QCOMPARE_NE(entry, entry2);

    QSslKeyingMaterial empty("mylabel", 10, "");
    empty.m_value = "payload";

    QSslKeyingMaterial null("mylabel", 10, {});
    null.m_value = "wrong";

    QSslConfiguration conf;
    conf.setKeyingMaterial({ null, empty });
    QCOMPARE(conf.takeKeyingMaterial(empty), empty);
}

void tst_QSslKeyingMaterial::cloning()
{
    const QSslKeyingMaterial def;
    QCOMPARE(def.clone(), def);
    QVERIFY(!def.clone().isValid());

    QSslKeyingMaterial entry("mylabel", 32, "ctx");
    entry.m_value = "payload";

    QSslKeyingMaterial clone = entry.clone();
    QCOMPARE(clone.label(), entry.label());
    QCOMPARE(clone.context(), entry.context());
    QCOMPARE(clone.requestedSize(), entry.requestedSize());
    QVERIFY(clone.isValid());
    // The value is never part of a clone, and cloning leaves the original alone:
    QVERIFY(clone.value().isNull());
    QCOMPARE_NE(entry.clone(), entry);
    QCOMPARE(entry.value(), QByteArray("payload"));

    // A null context must not become an empty one, or the other way around:
    const QSslKeyingMaterial nullContext("mylabel", 10, {});
    QVERIFY(nullContext.clone().context().isNull());
    const QSslKeyingMaterial emptyContext("mylabel", 10, "");
    QVERIFY(!emptyContext.clone().context().isNull());
    QVERIFY(emptyContext.clone().context().isEmpty());
    QCOMPARE_NE(nullContext.clone(), emptyContext.clone());

    // Resetting an entry, the way the TLS backend does on socket re-use:
    QSslKeyingMaterial reused("mylabel", 8, "ctx");
    reused.m_value = "payload";
    reused = reused.clone();
    QCOMPARE(reused.label(), QByteArray("mylabel"));
    QCOMPARE(reused.context(), QByteArray("ctx"));
    QCOMPARE(reused.requestedSize(), 8);
    QVERIFY(reused.value().isNull());
    // ... and with the value gone, an entry and its clone are indistinguishable:
    QCOMPARE(reused.clone(), reused);
}

// Taking the values is a mutation, so it detaches a QSslConfiguration that
// shares its private with another one, rather than reaching into the other:
void tst_QSslKeyingMaterial::sharedConfigurationPrivate()
{
    QSslKeyingMaterial entry("mylabel", 7, "ctx");
    entry.m_value = "payload";

    QSslConfiguration first;
    first.setKeyingMaterial({ entry });
    QSslConfiguration second = first;

    const auto taken = first.takeKeyingMaterial();
    QCOMPARE(taken.size(), 1);
    QCOMPARE(taken.first().value(), QByteArray("payload"));
    QVERIFY(first.takeKeyingMaterial().first().value().isNull());
    // 'second' was left with the original private, values and all:
    QCOMPARE(second.takeKeyingMaterial().first().value(), QByteArray("payload"));

    // The same for the single-request overload:
    QSslConfiguration third;
    third.setKeyingMaterial({ entry });
    QSslConfiguration fourth = third;

    const auto match = third.takeKeyingMaterial(entry.clone());
    QVERIFY(match);
    QCOMPARE(match->value(), QByteArray("payload"));
    QVERIFY(third.takeKeyingMaterial(entry.clone())->value().isNull());
    QCOMPARE(fourth.takeKeyingMaterial(entry.clone())->value(), QByteArray("payload"));
}

#if QT_CONFIG(ssl)

QSslConfiguration tst_QSslKeyingMaterial::serverConfig()
{
    QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();

    QFile keyFile(testDataDir + "certs/selfsigned-server.key");
    if (keyFile.open(QIODevice::ReadOnly))
        cfg.setPrivateKey(QSslKey(keyFile.readAll(), QSsl::Rsa));

    const auto certs =
        QSslCertificate::fromPath(testDataDir + "certs/selfsigned-server.crt");
    cfg.setLocalCertificate(certs.first());

    return cfg;
}

void tst_QSslKeyingMaterial::initTestCase()
{
    // At the moment only OpenSSL backend properly supports
    // tst_QSslKeyingMaterial.
    if (QSslSocket::activeBackend() != QStringLiteral("openssl"))
        QSKIP("The active TLS backend does not support QSslKeyingMaterial");

    testDataDir = QFileInfo(QFINDTESTDATA("certs")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
    if (!testDataDir.endsWith(QLatin1String("/")))
        testDataDir += QLatin1String("/");
}

void tst_QSslKeyingMaterial::exporterProducesSameMaterialOnBothSides()
{
    QList<QSslKeyingMaterial> material;
    const QSslKeyingMaterial label1_5(QByteArray("label1"), 5);
    const QSslKeyingMaterial label1_6(QByteArray("label1"), 6);
    const QSslKeyingMaterial label1_5_ctx1(QByteArray("label1"), 5, QByteArray("ctx1"));
    material << label1_5;
    material << label1_6;
    material << label1_5_ctx1;

    QSslServer server;
    auto serverCfg = serverConfig();
    serverCfg.setKeyingMaterial(material);
    server.setSslConfiguration(serverCfg);
    QCOMPARE(server.sslConfiguration().takeKeyingMaterial().first().value().size(), 0);
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QSslSocket client;
    auto clientCfg = QSslConfiguration::defaultConfiguration();
    clientCfg.setKeyingMaterial(material);
    client.setSslConfiguration(clientCfg);
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial().first().value().size(), 0);
    QObject::connect(&client, &QSslSocket::sslErrors, &client, [&client]{
        client.ignoreSslErrors();
    });
    QList<QSslKeyingMaterial> keyingMaterialOnEncrypted;
    QObject::connect(&client, &QSslSocket::encrypted, &client, [&client, &keyingMaterialOnEncrypted]{
        keyingMaterialOnEncrypted = client.sslConfiguration().takeKeyingMaterial();
    });

    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    QTRY_VERIFY(client.isEncrypted());
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial().size(), 3);
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial().first().value().size(), 5);
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial().last().value().size(), 5);
    // Make sure that what is available immediately on 'encrypted' signal matches what we have now:
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial(), keyingMaterialOnEncrypted);

    QTRY_VERIFY(server.hasPendingConnections());
    QTcpSocket *pending = server.nextPendingConnection();
    QVERIFY(pending);
    auto *serverSocket = qobject_cast<QSslSocket *>(pending);
    QVERIFY(serverSocket);
    QVERIFY(serverSocket->isEncrypted());

    const auto serverMaterial = serverSocket->sslConfiguration().takeKeyingMaterial();
    const auto clientMaterial = client.sslConfiguration().takeKeyingMaterial();
    QCOMPARE(serverMaterial.size(), 3);
    QCOMPARE(serverMaterial.first().value().size(), 5);
    QCOMPARE(serverMaterial.last().value().size(), 5);
    QCOMPARE(serverMaterial.first(), clientMaterial.first());
    QCOMPARE(serverMaterial.last(), clientMaterial.last());
    QCOMPARE_NE(serverMaterial.first(), clientMaterial.last());

    QVERIFY(!client.sslConfiguration().takeKeyingMaterial(QSslKeyingMaterial(QByteArray("label2"), 5)));

    const auto data1_5 = client.sslConfiguration().takeKeyingMaterial(label1_5);
    QVERIFY(data1_5);
    QCOMPARE(serverMaterial.first(), data1_5.value());

    const auto data1_5_ctx1 = client.sslConfiguration().takeKeyingMaterial(label1_5_ctx1);
    QVERIFY(data1_5_ctx1);
    QCOMPARE(serverMaterial.last(), data1_5_ctx1.value());

    const auto data1_6 = client.sslConfiguration().takeKeyingMaterial(label1_6);
    QVERIFY(data1_6);
    QCOMPARE_NE(data1_6.value(), data1_5.value());
    QCOMPARE_NE(data1_6.value(), data1_5_ctx1.value());
    QCOMPARE_NE(data1_5.value(), data1_5_ctx1.value());

    // The list overload hands the values over as well, leaving the requests:
    QSslConfiguration exported = client.sslConfiguration();
    const auto taken = exported.takeKeyingMaterial();
    QCOMPARE(taken.size(), 3);
    QCOMPARE(taken.first().value().size(), 5);
    QVERIFY(exported.takeKeyingMaterial().first().value().isEmpty());
    // Taking from a copy of the configuration leaves the socket's own alone:
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial().first().value().size(), 5);

    // Writing the valueless entries back drops the socket's copy, which is how
    // an application ends up holding the only one:
    QSslConfiguration purged = client.sslConfiguration();
    QVERIFY(purged.takeKeyingMaterial(label1_5));
    client.setSslConfiguration(purged);
    QVERIFY(client.sslConfiguration().takeKeyingMaterial(label1_5)->value().isEmpty());
    // ... and only the entry that was taken:
    QCOMPARE(client.sslConfiguration().takeKeyingMaterial(label1_6)->value().size(), 6);

    // And finally, make sure it is cleared on socket re-use:
    client.disconnectFromHost();
    QTRY_COMPARE(client.state(), QAbstractSocket::UnconnectedState);
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.sslConfiguration().takeKeyingMaterial().front().value().isEmpty());
}

#endif // Feature 'ssl'.

QTEST_MAIN(tst_QSslKeyingMaterial)
#include "tst_qsslkeyingmaterial.moc"
