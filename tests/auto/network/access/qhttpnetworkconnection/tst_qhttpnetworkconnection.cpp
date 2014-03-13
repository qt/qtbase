/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include "private/qhttpnetworkconnection_p.h"
#include "private/qnoncontiguousbytedevice_p.h"
#include <QAuthenticator>

#include "../../../network-settings.h"

class tst_QHttpNetworkConnection: public QObject
{
    Q_OBJECT

public:
    tst_QHttpNetworkConnection();

public Q_SLOTS:
    void finishedReply();
    void finishedWithError(QNetworkReply::NetworkError errorCode, const QString &detail);
    void challenge401(const QHttpNetworkRequest &request, QAuthenticator *authenticator);
#ifndef QT_NO_SSL
    void sslErrors(const QList<QSslError> &errors);
#endif
private:
    bool finishedCalled;
    bool finishedWithErrorCalled;
    QNetworkReply::NetworkError netErrorCode;

private Q_SLOTS:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void options_data();
    void options();
    void get_data();
    void get();
    void head_data();
    void head();
    void post_data();
    void post();
    void put_data();
    void put();
    void _delete_data();
    void _delete();
    void trace_data();
    void trace();
    void _connect_data();
    void _connect();
#ifndef QT_NO_COMPRESS
    void compression_data();
    void compression();
#endif
#ifndef QT_NO_SSL
    void ignoresslerror_data();
    void ignoresslerror();
#endif
#ifdef QT_NO_SSL
    void nossl_data();
    void nossl();
#endif
    void get401_data();
    void get401();

    void getMultiple_data();
    void getMultiple();
    void getMultipleWithPipeliningAndMultiplePriorities();
    void getMultipleWithPriorities();

    void getEmptyWithPipelining();

    void getAndEverythingShouldBePipelined();

    void getAndThenDeleteObject();
    void getAndThenDeleteObject_data();
};

tst_QHttpNetworkConnection::tst_QHttpNetworkConnection()
{
}

void tst_QHttpNetworkConnection::initTestCase()
{
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

void tst_QHttpNetworkConnection::cleanupTestCase()
{
}

void tst_QHttpNetworkConnection::init()
{
}

void tst_QHttpNetworkConnection::cleanup()
{
}

void tst_QHttpNetworkConnection::options_data()
{
    // not tested yet
}

void tst_QHttpNetworkConnection::options()
{
    QEXPECT_FAIL("", "not tested yet", Continue);
    QVERIFY(false);
}

void tst_QHttpNetworkConnection::head_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("statusString");
    QTest::addColumn<int>("contentLength");

    QTest::newRow("success-internal") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfc3252.txt" << ushort(80) << false << 200 << "OK" << 25962;

    QTest::newRow("failure-path") << "http://" << QtNetworkSettings::serverName() << "/t" << ushort(80) << false << 404 << "Not Found" << -1;
    QTest::newRow("failure-protocol") << "" << QtNetworkSettings::serverName() << "/qtest/rfc3252.txt" << ushort(80) << false << 400 << "Bad Request" << -1;
}

void tst_QHttpNetworkConnection::head()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(int, statusCode);
    QFETCH(QString, statusString);
    QFETCH(int, contentLength);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);

    QHttpNetworkRequest request(protocol + host + path, QHttpNetworkRequest::Head);
    QHttpNetworkReply *reply = connection.sendRequest(request);

    QTime stopWatch;
    stopWatch.start();
    do {
        QCoreApplication::instance()->processEvents();
        if (stopWatch.elapsed() >= 30000)
            break;
    } while (!reply->isFinished());

    QCOMPARE(reply->statusCode(), statusCode);
    QCOMPARE(reply->reasonPhrase(), statusString);
    // only check it if it is set and expected
    if (reply->contentLength() != -1 && contentLength != -1)
        QCOMPARE(reply->contentLength(), qint64(contentLength));

    QVERIFY(reply->isFinished());

    delete reply;
}

void tst_QHttpNetworkConnection::get_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("statusString");
    QTest::addColumn<int>("contentLength");
    QTest::addColumn<int>("downloadSize");

    QTest::newRow("success-internal") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfc3252.txt" << ushort(80) << false << 200 << "OK" << 25962 << 25962;

    QTest::newRow("failure-path") << "http://" << QtNetworkSettings::serverName() << "/t" << ushort(80) << false << 404 << "Not Found" << -1 << -1;
    QTest::newRow("failure-protocol") << "" << QtNetworkSettings::serverName() << "/qtest/rfc3252.txt" << ushort(80) << false << 400 << "Bad Request" << -1 << -1;
}

void tst_QHttpNetworkConnection::get()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(int, statusCode);
    QFETCH(QString, statusString);
    QFETCH(int, contentLength);
    QFETCH(int, downloadSize);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);

    QHttpNetworkRequest request(protocol + host + path);
    QHttpNetworkReply *reply = connection.sendRequest(request);

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (reply->bytesAvailable())
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }

    QCOMPARE(reply->statusCode(), statusCode);
    QCOMPARE(reply->reasonPhrase(), statusString);
    // only check it if it is set and expected
    if (reply->contentLength() != -1 && contentLength != -1)
        QCOMPARE(reply->contentLength(), qint64(contentLength));

    stopWatch.start();
    QByteArray ba;
    do {
        QCoreApplication::instance()->processEvents();
        while (reply->bytesAvailable())
            ba += reply->readAny();
        if (stopWatch.elapsed() >= 30000)
            break;
    } while (!reply->isFinished());

    QVERIFY(reply->isFinished());
    //do not require server generated error pages to be a fixed size
    if (downloadSize != -1)
        QCOMPARE(ba.size(), downloadSize);
    //but check against content length if it was sent
    if (reply->contentLength() != -1)
        QCOMPARE(ba.size(), (int)reply->contentLength());

    delete reply;
}

void tst_QHttpNetworkConnection::finishedReply()
{
    finishedCalled = true;
}

void tst_QHttpNetworkConnection::finishedWithError(QNetworkReply::NetworkError errorCode, const QString &detail)
{
    Q_UNUSED(detail)
    finishedWithErrorCalled = true;
    netErrorCode = errorCode;
}

void tst_QHttpNetworkConnection::put_data()
{

    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<QString>("data");
    QTest::addColumn<bool>("succeed");

    QTest::newRow("success-internal") << "http://" << QtNetworkSettings::serverName() << "/dav/file1.txt" << ushort(80) << false << "Hello World\nEnd of file\n"<<true;
    QTest::newRow("fail-internal") << "http://" << QtNetworkSettings::serverName() << "/dav2/file1.txt" << ushort(80) << false << "Hello World\nEnd of file\n"<<false;
    QTest::newRow("fail-host") << "http://" << "invalid.test.qt-project.org" << "/dav2/file1.txt" << ushort(80) << false << "Hello World\nEnd of file\n"<<false;
}

void tst_QHttpNetworkConnection::put()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(QString, data);
    QFETCH(bool, succeed);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);

    QHttpNetworkRequest request(protocol + host + path, QHttpNetworkRequest::Put);

    QByteArray array = data.toLatin1();
    QNonContiguousByteDevice *bd = QNonContiguousByteDeviceFactory::create(&array);
    bd->setParent(this);
    request.setUploadByteDevice(bd);

    finishedCalled = false;
    finishedWithErrorCalled = false;

    QHttpNetworkReply *reply = connection.sendRequest(request);
    connect(reply, SIGNAL(finished()), SLOT(finishedReply()));
    connect(reply, SIGNAL(finishedWithError(QNetworkReply::NetworkError,QString)),
        SLOT(finishedWithError(QNetworkReply::NetworkError,QString)));

    QTime stopWatch;
    stopWatch.start();
    do {
        QCoreApplication::instance()->processEvents();
        if (stopWatch.elapsed() >= 30000)
            break;
    } while (!reply->isFinished() && !finishedCalled && !finishedWithErrorCalled);

    if (reply->isFinished()) {
        QByteArray ba;
        while (reply->bytesAvailable())
            ba += reply->readAny();
    } else if(finishedWithErrorCalled) {
        if(!succeed) {
            delete reply;
            return;
        } else {
            QFAIL("Error in PUT");
        }
    } else {
        QFAIL("PUT timed out");
    }

    int status = reply->statusCode();
    if (status != 200 && status != 201 && status != 204) {
        if (succeed) {
            qDebug()<<"PUT failed, Status Code:" <<status;
            QFAIL("Error in PUT");
        }
    } else {
        if (!succeed) {
            qDebug()<<"PUT Should fail, Status Code:" <<status;
            QFAIL("Error in PUT");
        }
    }
    delete reply;
}

void tst_QHttpNetworkConnection::post_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<QString>("data");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("statusString");
    QTest::addColumn<int>("contentLength");
    QTest::addColumn<int>("downloadSize");

    QTest::newRow("success-internal") << "http://" << QtNetworkSettings::serverName() << "/qtest/cgi-bin/echo.cgi" << ushort(80) << false << "7 bytes" << 200 << "OK" << 7 << 7;
    QTest::newRow("failure-internal") << "http://" << QtNetworkSettings::serverName() << "/t" << ushort(80) << false << "Hello World" << 404 << "Not Found" << -1 << -1;
}

void tst_QHttpNetworkConnection::post()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(QString, data);
    QFETCH(int, statusCode);
    QFETCH(QString, statusString);
    QFETCH(int, contentLength);
    QFETCH(int, downloadSize);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);

    QHttpNetworkRequest request(protocol + host + path, QHttpNetworkRequest::Post);

    QByteArray array = data.toLatin1();
    QNonContiguousByteDevice *bd = QNonContiguousByteDeviceFactory::create(&array);
    bd->setParent(this);
    request.setUploadByteDevice(bd);

    QHttpNetworkReply *reply = connection.sendRequest(request);

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (reply->bytesAvailable())
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }

    QCOMPARE(reply->statusCode(), statusCode);
    QCOMPARE(reply->reasonPhrase(), statusString);

    qint64 cLen = reply->contentLength();
    if (contentLength != -1) {
        // only check the content length if test expected it to be set
        if (cLen==-1) {
            // HTTP 1.1 server may respond with chunked encoding and in that
            // case contentLength is not present in reply -> verify that it is the case
            QByteArray transferEnc = reply->headerField("Transfer-Encoding");
            QCOMPARE(transferEnc, QByteArray("chunked"));
        } else {
            QCOMPARE(cLen, qint64(contentLength));
        }
    }

    stopWatch.start();
    QByteArray ba;
    do {
        QCoreApplication::instance()->processEvents();
        while (reply->bytesAvailable())
            ba += reply->readAny();
        if (stopWatch.elapsed() >= 30000)
            break;
    } while (!reply->isFinished());

    QVERIFY(reply->isFinished());
    //don't require fixed size for generated error pages
    if (downloadSize != -1)
        QCOMPARE(ba.size(), downloadSize);
    //but do compare with content length if possible
    if (cLen != -1)
        QCOMPARE(ba.size(), (int)cLen);

    delete reply;
}

void tst_QHttpNetworkConnection::_delete_data()
{
    // not tested yet
}

void tst_QHttpNetworkConnection::_delete()
{
    QEXPECT_FAIL("", "not tested yet", Continue);
    QVERIFY(false);
}

void tst_QHttpNetworkConnection::trace_data()
{
    // not tested yet
}

void tst_QHttpNetworkConnection::trace()
{
    QEXPECT_FAIL("", "not tested yet", Continue);
    QVERIFY(false);
}

void tst_QHttpNetworkConnection::_connect_data()
{
    // not tested yet
}

void tst_QHttpNetworkConnection::_connect()
{
    QEXPECT_FAIL("", "not tested yet", Continue);
    QVERIFY(false);
}

void tst_QHttpNetworkConnection::challenge401(const QHttpNetworkRequest &request,
                                                        QAuthenticator *authenticator)
{
    Q_UNUSED(request)

    QHttpNetworkReply *reply = qobject_cast<QHttpNetworkReply*>(sender());
    if (reply) {
        QHttpNetworkConnection *c = reply->connection();

        QVariant val = c->property("setCredentials");
        if (val.toBool()) {
            QVariant user = c->property("username");
            QVariant password = c->property("password");
            authenticator->setUser(user.toString());
            authenticator->setPassword(password.toString());
            c->setProperty("setCredentials", false);
        }
    }
}

void tst_QHttpNetworkConnection::get401_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<bool>("setCredentials");
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("statusCode");

    QTest::newRow("no-credentials") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfcs-auth/index.html" << ushort(80) << false << false << "" << ""<<401;
    QTest::newRow("invalid-credentials") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfcs-auth/index.html" << ushort(80) << false << true << "test" << "test"<<401;
    QTest::newRow("valid-credentials") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfcs-auth/index.html" << ushort(80) << false << true << "httptest" << "httptest"<<200;
    QTest::newRow("digest-authentication-invalid") << "http://" << QtNetworkSettings::serverName() << "/qtest/auth-digest/index.html" << ushort(80) << false << true << "wrong" << "wrong"<<401;
    QTest::newRow("digest-authentication-valid") << "http://" << QtNetworkSettings::serverName() << "/qtest/auth-digest/index.html" << ushort(80) << false << true << "httptest" << "httptest"<<200;
}

void tst_QHttpNetworkConnection::get401()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(bool, setCredentials);
    QFETCH(QString, username);
    QFETCH(QString, password);
    QFETCH(int, statusCode);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);
    connection.setProperty("setCredentials", setCredentials);
    connection.setProperty("username", username);
    connection.setProperty("password", password);

    QHttpNetworkRequest request(protocol + host + path);
    QHttpNetworkReply *reply = connection.sendRequest(request);
    connect(reply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                           SLOT(challenge401(QHttpNetworkRequest,QAuthenticator*)));

    finishedCalled = false;
    finishedWithErrorCalled = false;

    connect(reply, SIGNAL(finished()), SLOT(finishedReply()));
    connect(reply, SIGNAL(finishedWithError(QNetworkReply::NetworkError,QString)),
        SLOT(finishedWithError(QNetworkReply::NetworkError,QString)));

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (finishedCalled)
            break;
        if (finishedWithErrorCalled)
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }
    QCOMPARE(reply->statusCode(), statusCode);
    delete reply;
}

#ifndef QT_NO_COMPRESS
void tst_QHttpNetworkConnection::compression_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QString>("statusString");
    QTest::addColumn<int>("contentLength");
    QTest::addColumn<int>("downloadSize");
    QTest::addColumn<bool>("autoCompress");
    QTest::addColumn<QString>("contentCoding");

    QTest::newRow("success-autogzip-temp") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfcs/rfc2616.html" << ushort(80) << false << 200 << "OK" << -1 << 418321 << true << "";
    QTest::newRow("success-nogzip-temp") << "http://" << QtNetworkSettings::serverName() << "/qtest/rfcs/rfc2616.html" << ushort(80) << false << 200 << "OK" << 418321 << 418321 << false << "identity";
    QTest::newRow("success-manualgzip-temp") << "http://" << QtNetworkSettings::serverName() << "/qtest/deflate/rfc2616.html" << ushort(80) << false << 200 << "OK" << 119124 << 119124 << false << "gzip";

}

void tst_QHttpNetworkConnection::compression()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(int, statusCode);
    QFETCH(QString, statusString);
    QFETCH(int, contentLength);
    QFETCH(int, downloadSize);
    QFETCH(bool, autoCompress);
    QFETCH(QString, contentCoding);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    QCOMPARE(connection.isSsl(), encrypt);

    QHttpNetworkRequest request(protocol + host + path);
    if (!autoCompress)
        request.setHeaderField("Accept-Encoding", contentCoding.toLatin1());
    QHttpNetworkReply *reply = connection.sendRequest(request);
    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (reply->bytesAvailable())
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }

    QCOMPARE(reply->statusCode(), statusCode);
    QCOMPARE(reply->reasonPhrase(), statusString);
    bool isLengthOk = (reply->contentLength() == qint64(contentLength)
                      || reply->contentLength() == qint64(downloadSize)
                      || reply->contentLength() == -1); //apache2 does not send content-length for compressed pages

    QVERIFY(isLengthOk);

    stopWatch.start();
    QByteArray ba;
    do {
        QCoreApplication::instance()->processEvents();
        while (reply->bytesAvailable())
            ba += reply->readAny();
        if (stopWatch.elapsed() >= 30000)
            break;
    } while (!reply->isFinished());

    QVERIFY(reply->isFinished());
    QCOMPARE(ba.size(), downloadSize);

    delete reply;
}
#endif

#ifndef QT_NO_SSL
void tst_QHttpNetworkConnection::sslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors)

    QHttpNetworkReply *reply = qobject_cast<QHttpNetworkReply*>(sender());
    if (reply) {
        QHttpNetworkConnection *connection = reply->connection();

        QVariant val = connection->property("ignoreFromSignal");
        if (val.toBool())
            connection->ignoreSslErrors();
        finishedWithErrorCalled = true;
    }
}

void tst_QHttpNetworkConnection::ignoresslerror_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<bool>("ignoreInit");
    QTest::addColumn<bool>("ignoreFromSignal");
    QTest::addColumn<int>("statusCode");

    // This test will work only if the website has ssl errors.
    // fluke's certificate is signed by a non-standard authority.
    // Since we don't introduce that CA into the SSL verification chain,
    // connecting should fail.
    QTest::newRow("success-init") << "https://" << QtNetworkSettings::serverName() << "/" << ushort(443) << true << true << false << 200;
    QTest::newRow("success-fromSignal") << "https://" << QtNetworkSettings::serverName() << "/" << ushort(443) << true << false << true << 200;
    QTest::newRow("failure") << "https://" << QtNetworkSettings::serverName() << "/" << ushort(443) << true << false << false << 100;
}

void tst_QHttpNetworkConnection::ignoresslerror()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(bool, ignoreInit);
    QFETCH(bool, ignoreFromSignal);
    QFETCH(int, statusCode);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);
    if (ignoreInit)
        connection.ignoreSslErrors();
    QCOMPARE(connection.isSsl(), encrypt);
    connection.setProperty("ignoreFromSignal", ignoreFromSignal);

    QHttpNetworkRequest request(protocol + host + path);
    QHttpNetworkReply *reply = connection.sendRequest(request);
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
        SLOT(sslErrors(QList<QSslError>)));

    finishedWithErrorCalled = false;

    connect(reply, SIGNAL(finished()), SLOT(finishedReply()));

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (reply->bytesAvailable())
            break;
        if (statusCode == 100 && finishedWithErrorCalled)
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }
    QCOMPARE(reply->statusCode(), statusCode);
    delete reply;
}
#endif

#ifdef QT_NO_SSL
void tst_QHttpNetworkConnection::nossl_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<ushort>("port");
    QTest::addColumn<bool>("encrypt");
    QTest::addColumn<QNetworkReply::NetworkError>("networkError");

    QTest::newRow("protocol-error") << "https://" << QtNetworkSettings::serverName() << "/" << ushort(443) << true <<QNetworkReply::ProtocolUnknownError;
}

void tst_QHttpNetworkConnection::nossl()
{
    QFETCH(QString, protocol);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(ushort, port);
    QFETCH(bool, encrypt);
    QFETCH(QNetworkReply::NetworkError, networkError);

    QHttpNetworkConnection connection(host, port, encrypt);
    QCOMPARE(connection.port(), port);
    QCOMPARE(connection.hostName(), host);

    QHttpNetworkRequest request(protocol + host + path);
    QHttpNetworkReply *reply = connection.sendRequest(request);

    finishedWithErrorCalled = false;
    netErrorCode = QNetworkReply::NoError;

    connect(reply, SIGNAL(finished()), SLOT(finishedReply()));
    connect(reply, SIGNAL(finishedWithError(QNetworkReply::NetworkError,QString)),
        SLOT(finishedWithError(QNetworkReply::NetworkError,QString)));

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (finishedWithErrorCalled)
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }
    QCOMPARE(netErrorCode, networkError);
    delete reply;
}
#endif


void tst_QHttpNetworkConnection::getMultiple_data()
{
    QTest::addColumn<quint16>("connectionCount");
    QTest::addColumn<bool>("pipeliningAllowed");
    // send 100 requests. apache will usually force-close after 100 requests in a single tcp connection
    QTest::addColumn<int>("requestCount");

    QTest::newRow("6 connections, no pipelining, 100 requests")  << quint16(6) << false << 100;
    QTest::newRow("1 connection, no pipelining, 100 requests")  << quint16(1) << false << 100;
    QTest::newRow("6 connections, pipelining allowed, 100 requests")  << quint16(6) << true << 100;
    QTest::newRow("1 connection, pipelining allowed, 100 requests")  << quint16(1) << true << 100;
}

void tst_QHttpNetworkConnection::getMultiple()
{
    QFETCH(quint16, connectionCount);
    QFETCH(bool, pipeliningAllowed);
    QFETCH(int, requestCount);

    QHttpNetworkConnection connection(connectionCount, QtNetworkSettings::serverName());

    QList<QHttpNetworkRequest*> requests;
    QList<QHttpNetworkReply*> replies;

    for (int i = 0; i < requestCount; i++) {
        // depending on what you use the results will vary.
        // for the "real" results, use a URL that has "internet latency" for you. Then (6 connections, pipelining) will win.
        // for LAN latency, you will possibly get that (1 connection, no pipelining) is the fastest
        QHttpNetworkRequest *request = new QHttpNetworkRequest("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt");
        if (pipeliningAllowed)
            request->setPipeliningAllowed(true);
        requests.append(request);
        QHttpNetworkReply *reply = connection.sendRequest(*request);
        replies.append(reply);
    }

    QTime stopWatch;
    stopWatch.start();
    int finishedCount = 0;
    do {
        QCoreApplication::instance()->processEvents();
        if (stopWatch.elapsed() >= 60000)
            break;

        finishedCount = 0;
        for (int i = 0; i < replies.length(); i++)
            if (replies.at(i)->isFinished())
                finishedCount++;

    } while (finishedCount != replies.length());

    // redundant
    for (int i = 0; i < replies.length(); i++)
        QVERIFY(replies.at(i)->isFinished());

    qDebug() << "===" << stopWatch.elapsed() << "msec ===";

    qDeleteAll(requests);
    qDeleteAll(replies);
}

void tst_QHttpNetworkConnection::getMultipleWithPipeliningAndMultiplePriorities()
{
    quint16 requestCount = 100;

    // use 2 connections.
    QHttpNetworkConnection connection(2, QtNetworkSettings::serverName());

    QList<QHttpNetworkRequest*> requests;
    QList<QHttpNetworkReply*> replies;

    for (int i = 0; i < requestCount; i++) {
        QHttpNetworkRequest *request = 0;
        if (i % 3)
            request = new QHttpNetworkRequest("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt", QHttpNetworkRequest::Get);
        else
            request = new QHttpNetworkRequest("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt", QHttpNetworkRequest::Head);

        if (i % 2 || i % 3)
            request->setPipeliningAllowed(true);

        if (i % 3)
            request->setPriority(QHttpNetworkRequest::HighPriority);
        else if (i % 5)
            request->setPriority(QHttpNetworkRequest::NormalPriority);
        else if (i % 7)
            request->setPriority(QHttpNetworkRequest::LowPriority);

        requests.append(request);
        QHttpNetworkReply *reply = connection.sendRequest(*request);
        replies.append(reply);
    }

    QTime stopWatch;
    stopWatch.start();
    int finishedCount = 0;
    do {
        QCoreApplication::instance()->processEvents();
        if (stopWatch.elapsed() >= 60000)
            break;

        finishedCount = 0;
        for (int i = 0; i < replies.length(); i++)
            if (replies.at(i)->isFinished())
                finishedCount++;

    } while (finishedCount != replies.length());

    int pipelinedCount = 0;
    for (int i = 0; i < replies.length(); i++) {
        QVERIFY(replies.at(i)->isFinished());
        QVERIFY (!(replies.at(i)->request().isPipeliningAllowed() == false
            && replies.at(i)->isPipeliningUsed()));

        if (replies.at(i)->isPipeliningUsed())
            pipelinedCount++;
    }

    // We allow pipelining for every 2nd,3rd,4th,6th,8th,9th,10th etc request.
    // Assume that half of the requests had been pipelined.
    // (this is a very relaxed condition, when last measured 79 of 100
    // requests had been pipelined)
    QVERIFY(pipelinedCount >= requestCount / 2);

    qDebug() << "===" << stopWatch.elapsed() << "msec ===";

    qDeleteAll(requests);
    qDeleteAll(replies);
}

class GetMultipleWithPrioritiesReceiver : public QObject
{
    Q_OBJECT
public:
    int highPrioReceived;
    int lowPrioReceived;
    int requestCount;
    GetMultipleWithPrioritiesReceiver(int rq) : highPrioReceived(0), lowPrioReceived(0), requestCount(rq) { }
public Q_SLOTS:
    void finishedSlot() {
        QHttpNetworkReply *reply = (QHttpNetworkReply*) sender();
        if (reply->request().priority() == QHttpNetworkRequest::HighPriority)
            highPrioReceived++;
        else if (reply->request().priority() == QHttpNetworkRequest::LowPriority)
            lowPrioReceived++;
        else
            QFAIL("Wrong priority!?");

        QVERIFY(highPrioReceived + 7 >= lowPrioReceived);

        if (highPrioReceived + lowPrioReceived == requestCount)
            QTestEventLoop::instance().exitLoop();
    }
};

void tst_QHttpNetworkConnection::getMultipleWithPriorities()
{
    quint16 requestCount = 100;
    // use 2 connections.
    QHttpNetworkConnection connection(2, QtNetworkSettings::serverName());
    GetMultipleWithPrioritiesReceiver receiver(requestCount);
    QUrl url("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt");
    QList<QHttpNetworkRequest*> requests;
    QList<QHttpNetworkReply*> replies;

    for (int i = 0; i < requestCount; i++) {
        QHttpNetworkRequest *request = 0;
        if (i % 3)
            request = new QHttpNetworkRequest(url, QHttpNetworkRequest::Get);
        else
            request = new QHttpNetworkRequest(url, QHttpNetworkRequest::Head);

        if (i % 2)
            request->setPriority(QHttpNetworkRequest::HighPriority);
        else
            request->setPriority(QHttpNetworkRequest::LowPriority);

        requests.append(request);
        QHttpNetworkReply *reply = connection.sendRequest(*request);
        connect(reply, SIGNAL(finished()), &receiver, SLOT(finishedSlot()));
        replies.append(reply);
    }

    QTestEventLoop::instance().enterLoop(40);
    QVERIFY(!QTestEventLoop::instance().timeout());

    qDeleteAll(requests);
    qDeleteAll(replies);
}


class GetEmptyWithPipeliningReceiver : public QObject
{
    Q_OBJECT
public:
    int receivedCount;
    int requestCount;
    GetEmptyWithPipeliningReceiver(int rq) : receivedCount(0),requestCount(rq) { }
public Q_SLOTS:
    void finishedSlot() {
        QHttpNetworkReply *reply = (QHttpNetworkReply*) sender();
        Q_UNUSED(reply);
        receivedCount++;

        if (receivedCount == requestCount)
            QTestEventLoop::instance().exitLoop();
    }
};

void tst_QHttpNetworkConnection::getEmptyWithPipelining()
{
    quint16 requestCount = 50;
    // use 2 connections.
    QHttpNetworkConnection connection(2, QtNetworkSettings::serverName());
    GetEmptyWithPipeliningReceiver receiver(requestCount);

    QUrl url("http://" + QtNetworkSettings::serverName() + "/cgi-bin/echo.cgi"); // a get on this = getting an empty file
    QList<QHttpNetworkRequest*> requests;
    QList<QHttpNetworkReply*> replies;

    for (int i = 0; i < requestCount; i++) {
        QHttpNetworkRequest *request = 0;
        request = new QHttpNetworkRequest(url, QHttpNetworkRequest::Get);
        request->setPipeliningAllowed(true);

        requests.append(request);
        QHttpNetworkReply *reply = connection.sendRequest(*request);
        connect(reply, SIGNAL(finished()), &receiver, SLOT(finishedSlot()));
        replies.append(reply);
    }

    QTestEventLoop::instance().enterLoop(20);
    QVERIFY(!QTestEventLoop::instance().timeout());

    qDeleteAll(requests);
    qDeleteAll(replies);
}

class GetAndEverythingShouldBePipelinedReceiver : public QObject
{
    Q_OBJECT
public:
    int receivedCount;
    int requestCount;
    GetAndEverythingShouldBePipelinedReceiver(int rq) : receivedCount(0),requestCount(rq) { }
public Q_SLOTS:
    void finishedSlot() {
        QHttpNetworkReply *reply = (QHttpNetworkReply*) sender();
        Q_UNUSED(reply);
        receivedCount++;

        if (receivedCount == requestCount)
            QTestEventLoop::instance().exitLoop();
    }
};

void tst_QHttpNetworkConnection::getAndEverythingShouldBePipelined()
{
    quint16 requestCount = 100;
    // use 1 connection.
    QHttpNetworkConnection connection(1, QtNetworkSettings::serverName());
    QUrl url("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt");
    QList<QHttpNetworkRequest*> requests;
    QList<QHttpNetworkReply*> replies;

    GetAndEverythingShouldBePipelinedReceiver receiver(requestCount);

    for (int i = 0; i < requestCount; i++) {
        QHttpNetworkRequest *request = 0;
        request = new QHttpNetworkRequest(url, QHttpNetworkRequest::Get);
        request->setPipeliningAllowed(true);
        requests.append(request);
        QHttpNetworkReply *reply = connection.sendRequest(*request);
        connect(reply, SIGNAL(finished()), &receiver, SLOT(finishedSlot()));
        replies.append(reply);
    }
    QTestEventLoop::instance().enterLoop(40);
    QVERIFY(!QTestEventLoop::instance().timeout());

    qDeleteAll(requests);
    qDeleteAll(replies);

}


void tst_QHttpNetworkConnection::getAndThenDeleteObject_data()
{
    QTest::addColumn<bool>("replyFirst");

    QTest::newRow("delete-reply-first") << true;
    QTest::newRow("delete-connection-first") << false;
}

void tst_QHttpNetworkConnection::getAndThenDeleteObject()
{
    // yes, this will leak if the testcase fails. I don't care. It must not fail then :P
    QHttpNetworkConnection *connection = new QHttpNetworkConnection(QtNetworkSettings::serverName());
    QHttpNetworkRequest request("http://" + QtNetworkSettings::serverName() + "/qtest/bigfile");
    QHttpNetworkReply *reply = connection->sendRequest(request);
    reply->setDownstreamLimited(true);

    QTime stopWatch;
    stopWatch.start();
    forever {
        QCoreApplication::instance()->processEvents();
        if (reply->bytesAvailable())
            break;
        if (stopWatch.elapsed() >= 30000)
            break;
    }

    QVERIFY(reply->bytesAvailable());
    QCOMPARE(reply->statusCode() ,200);
    QVERIFY(!reply->isFinished()); // must not be finished

    QFETCH(bool, replyFirst);

    if (replyFirst) {
        delete reply;
        delete connection;
    } else {
        delete connection;
        delete reply;
    }
}



QTEST_MAIN(tst_QHttpNetworkConnection)
#include "tst_qhttpnetworkconnection.moc"
