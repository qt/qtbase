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
    QCOMPARE(server.sslConfiguration().keyingMaterial().first().value().size(), 0);
    QVERIFY(server.listen(QHostAddress::LocalHost));

    QSslSocket client;
    auto clientCfg = QSslConfiguration::defaultConfiguration();
    clientCfg.setKeyingMaterial(material);
    client.setSslConfiguration(clientCfg);
    QCOMPARE(client.sslConfiguration().keyingMaterial().first().value().size(), 0);
    QObject::connect(&client, &QSslSocket::sslErrors, &client, [&client]{
        client.ignoreSslErrors();
    });

    QSignalSpy clientConnectedSpy(&client, &QSslSocket::encrypted);
    client.connectToHostEncrypted(QHostAddress(QHostAddress::LocalHost).toString(), server.serverPort());
    QTRY_VERIFY(client.isEncrypted());
    QCOMPARE(client.sslConfiguration().keyingMaterial().size(), 3);
    QCOMPARE(client.sslConfiguration().keyingMaterial().first().value().size(), 5);
    QCOMPARE(client.sslConfiguration().keyingMaterial().last().value().size(), 5);

    QTRY_VERIFY(server.hasPendingConnections());
    QTcpSocket *pending = server.nextPendingConnection();
    QVERIFY(pending);
    auto *serverSocket = qobject_cast<QSslSocket *>(pending);
    QVERIFY(serverSocket);
    QVERIFY(serverSocket->isEncrypted());

    const auto serverMaterial = serverSocket->sslConfiguration().keyingMaterial();
    const auto clientMaterial = client.sslConfiguration().keyingMaterial();
    QCOMPARE(serverMaterial.size(), 3);
    QCOMPARE(serverMaterial.first().value().size(), 5);
    QCOMPARE(serverMaterial.last().value().size(), 5);
    QCOMPARE(serverMaterial.first(), clientMaterial.first());
    QCOMPARE(serverMaterial.last(), clientMaterial.last());
    QCOMPARE_NE(serverMaterial.first(), clientMaterial.last());

    QVERIFY(!client.sslConfiguration().keyingMaterial(QSslKeyingMaterial(QByteArray("label2"), 5)));

    const auto data1_5 = client.sslConfiguration().keyingMaterial(label1_5);
    QVERIFY(data1_5);
    QCOMPARE(serverMaterial.first(), data1_5.value());

    const auto data1_5_ctx1 = client.sslConfiguration().keyingMaterial(label1_5_ctx1);
    QVERIFY(data1_5_ctx1);
    QCOMPARE(serverMaterial.last(), data1_5_ctx1.value());

    const auto data1_6 = client.sslConfiguration().keyingMaterial(label1_6);
    QVERIFY(data1_6);
    QCOMPARE_NE(data1_6.value(), data1_5.value());
    QCOMPARE_NE(data1_6.value(), data1_5_ctx1.value());
    QCOMPARE_NE(data1_5.value(), data1_5_ctx1.value());
}

#endif // Feature 'ssl'.

QTEST_MAIN(tst_QSslKeyingMaterial)
#include "tst_qsslkeyingmaterial.moc"
