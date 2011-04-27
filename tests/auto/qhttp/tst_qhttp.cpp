/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qbuffer.h>
#include <qcoreapplication.h>
#include <qfile.h>
#include <qhostinfo.h>
#include <qhttp.h>
#include <qlist.h>
#include <qpointer.h>
#include <qtcpsocket.h>
#include <qtcpserver.h>
#include <qauthenticator.h>
#include <QNetworkProxy>
#ifndef QT_NO_OPENSSL
# include <qsslsocket.h>
#endif

#include "../network-settings.h"

//TESTED_CLASS=
//TESTED_FILES=

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
// And underlying Open C have application private dir in default search path
#define SRCDIR ""
#endif

Q_DECLARE_METATYPE(QHttpResponseHeader)

class tst_QHttp : public QObject
{
    Q_OBJECT

public:
    tst_QHttp();
    virtual ~tst_QHttp();


public slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
private slots:
    void constructing();
    void invalidRequests();
    void get_data();
    void get();
    void head_data();
    void head();
    void post_data();
    void post();
    void request_data();
    void request();
    void authorization_data();
    void authorization();
    void proxy_data();
    void proxy();
    void proxy2();
    void proxy3();
    void postAuthNtlm();
    void proxyAndSsl();
    void cachingProxyAndSsl();
    void reconnect();
    void setSocket();
    void unexpectedRemoteClose();
    void pctEncodedPath();
    void caseInsensitiveKeys();
    void emptyBodyInReply();
    void abortInReadyRead();
    void abortInResponseHeaderReceived();
    void nestedEventLoop();
    void connectionClose();

protected slots:
    void stateChanged( int );
    void responseHeaderReceived( const QHttpResponseHeader & );
    void readyRead( const QHttpResponseHeader& );
    void dataSendProgress( int, int );
    void dataReadProgress( int , int );

    void requestStarted( int );
    void requestFinished( int, bool );
    void done( bool );

    void reconnect_state(int state);
    void proxy2_slot();
    void nestedEventLoop_slot(int id);

    void abortSender();
    void proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth);

private:
    QHttp *newHttp(bool withAuth = false);
    void addRequest( QHttpRequestHeader, int );
    bool headerAreEqual( const QHttpHeader&, const QHttpHeader& );

    QHttp *http;

    struct RequestResult
    {
    QHttpRequestHeader req;
    QHttpResponseHeader resp;
    int success;
    };
    QMap< int, RequestResult > resultMap;
    typedef QMap<int,RequestResult>::Iterator ResMapIt;
    QList<int> ids; // helper to make sure that all expected signals are emitted

    int current_id;
    int cur_state;
    int done_success;

    QByteArray readyRead_ba;

    int bytesTotalSend;
    int bytesDoneSend;
    int bytesTotalRead;
    int bytesDoneRead;

    int getId;
    int headId;
    int postId;

    int reconnect_state_connect_count;

    bool connectionWithAuth;
    bool proxyAuthCalled;
};

class ClosingServer: public QTcpServer
{
    Q_OBJECT
public:
    ClosingServer()
    {
        connect(this, SIGNAL(newConnection()), SLOT(handleConnection()));
        listen();
    }

public slots:
    void handleConnection()
    {
        delete nextPendingConnection();
    }
};

//#define DUMP_SIGNALS

const int bytesTotal_init = -10;
const int bytesDone_init = -10;

tst_QHttp::tst_QHttp()
{
    Q_SET_DEFAULT_IAP
}

tst_QHttp::~tst_QHttp()
{
}

void tst_QHttp::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");

    QTest::newRow("WithoutProxy") << false << 0;
    QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy);
}

void tst_QHttp::initTestCase()
{
}

void tst_QHttp::cleanupTestCase()
{
}

void tst_QHttp::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        }
    }

    http = 0;

    resultMap.clear();
    ids.clear();

    current_id = 0;
    cur_state = QHttp::Unconnected;
    done_success = -1;

    readyRead_ba = QByteArray();

    getId = -1;
    headId = -1;
    postId = -1;
}

void tst_QHttp::cleanup()
{
    delete http;
    http = 0;

    QCoreApplication::processEvents();

    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
    }
}

void tst_QHttp::constructing()
{
    //QHeader
    {
        QHttpRequestHeader header;
        header.addValue("key1", "val1");
        header.addValue("key2", "val2");
        header.addValue("key1", "val3");
        QCOMPARE(header.values().size(), 3);
        QCOMPARE(header.allValues("key1").size(), 2);
        QVERIFY(header.hasKey("key2"));
        QCOMPARE(header.keys().count(), 2);

    }

    {
        QHttpResponseHeader header(200);
    }
}

void tst_QHttp::invalidRequests()
{
    QHttp http;
    http.setHost("localhost");  // we will not actually connect

    QTest::ignoreMessage(QtWarningMsg, "QHttp: empty path requested is invalid -- using '/'");
    http.get(QString());

    QTest::ignoreMessage(QtWarningMsg, "QHttp: empty path requested is invalid -- using '/'");
    http.head(QString());

    QTest::ignoreMessage(QtWarningMsg, "QHttp: empty path requested is invalid -- using '/'");
    http.post(QString(), QByteArray());

    QTest::ignoreMessage(QtWarningMsg, "QHttp: empty path requested is invalid -- using '/'");
    http.request(QHttpRequestHeader("PROPFIND", QString()));
}

void tst_QHttp::get_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("success");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<QByteArray>("res");
    QTest::addColumn<bool>("useIODevice");

    // ### move this into external testdata
    QFile file( SRCDIR "rfc3252.txt" );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    QByteArray rfc3252 = file.readAll();
    file.close();

    file.setFileName( SRCDIR "trolltech" );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    QByteArray trolltech = file.readAll();
    file.close();

    // test the two get() modes in one routine
    for ( int i=0; i<2; i++ ) {
	QTest::newRow(QString("path_01_%1").arg(i).toLatin1()) << QtNetworkSettings::serverName() << 80u
	    << QString("/qtest/rfc3252.txt") << 1 << 200 << rfc3252 << (bool)(i==1);
	QTest::newRow( QString("path_02_%1").arg(i).toLatin1() ) << QString("www.ietf.org") << 80u
	    << QString("/rfc/rfc3252.txt") << 1 << 200 << rfc3252 << (bool)(i==1);

	QTest::newRow( QString("uri_01_%1").arg(i).toLatin1() ) << QtNetworkSettings::serverName() << 80u
	    << QString("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt") << 1 << 200 << rfc3252 << (bool)(i==1);
	QTest::newRow( QString("uri_02_%1").arg(i).toLatin1() ) << "www.ietf.org" << 80u
	    << QString("http://www.ietf.org/rfc/rfc3252.txt") << 1 << 200 << rfc3252 << (bool)(i==1);

	QTest::newRow( QString("fail_01_%1").arg(i).toLatin1() ) << QString("this-host-will-not-exist.") << 80u
	    << QString("/qtest/rfc3252.txt") << 0 << 0 << QByteArray() << (bool)(i==1);

	QTest::newRow( QString("failprot_01_%1").arg(i).toLatin1() ) << QtNetworkSettings::serverName() << 80u
	    << QString("/t") << 1 << 404 << QByteArray() << (bool)(i==1);
	QTest::newRow( QString("failprot_02_%1").arg(i).toLatin1() ) << QtNetworkSettings::serverName() << 80u
	    << QString("qtest/rfc3252.txt") << 1 << 400 << QByteArray() << (bool)(i==1);

  // qt.nokia.com/doc uses transfer-encoding=chunked
    /* qt.nokia.com/doc no longer seams to be using chuncked encodig.
    QTest::newRow( QString("chunked_01_%1").arg(i).toLatin1() ) << QString("test.troll.no") << 80u
	    << QString("/") << 1 << 200 << trolltech << (bool)(i==1);
    */
	QTest::newRow( QString("chunked_02_%1").arg(i).toLatin1() ) << QtNetworkSettings::serverName() << 80u
	    << QString("/qtest/cgi-bin/rfc.cgi") << 1 << 200 << rfc3252 << (bool)(i==1);
    }
}

void tst_QHttp::get()
{
    // for the overload that takes a QIODevice
    QByteArray buf_ba;
    QBuffer buf( &buf_ba );

    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, path );
    QFETCH( bool, useIODevice );

    http = newHttp();
    QCOMPARE( http->currentId(), 0 );
    QCOMPARE( (int)http->state(), (int)QHttp::Unconnected );

    addRequest( QHttpRequestHeader(), http->setHost( host, port ) );
    if ( useIODevice ) {
    buf.open( QIODevice::WriteOnly );
    getId = http->get( path, &buf );
    } else {
    getId = http->get( path );
    }
    addRequest( QHttpRequestHeader(), getId );

    QTestEventLoop::instance().enterLoop( 50 );

    if ( QTestEventLoop::instance().timeout() )
    QFAIL( "Network operation timed out" );

    ResMapIt res = resultMap.find( getId );
    QVERIFY( res != resultMap.end() );
    if ( res.value().success!=1 && host=="www.ietf.org" ) {
    // The nightly tests fail from time to time. In order to make them more
    // stable, add some debug output that might help locate the problem (I
    // can't reproduce the problem in the non-nightly builds).
    qDebug( "Error %d: %s", http->error(), http->errorString().toLatin1().constData() );
    }
    QTEST( res.value().success, "success" );
    if ( res.value().success ) {
    QTEST( res.value().resp.statusCode(), "statusCode" );

    QFETCH( QByteArray, res );
    if ( res.count() > 0 ) {
        if ( useIODevice ) {
        QCOMPARE(buf_ba, res);
        if ( bytesDoneRead != bytesDone_init )
            QVERIFY( (int)buf_ba.size() == bytesDoneRead );
        } else {
        QCOMPARE(readyRead_ba, res);
        if ( bytesDoneRead != bytesDone_init )
            QVERIFY( (int)readyRead_ba.size() == bytesDoneRead );
        }
    }
    QVERIFY( bytesTotalRead != bytesTotal_init );
    if ( bytesTotalRead > 0 )
        QVERIFY( bytesDoneRead == bytesTotalRead );
    } else {
    QVERIFY( !res.value().resp.isValid() );
    }
}

void tst_QHttp::head_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("success");
    QTest::addColumn<int>("statusCode");
    QTest::addColumn<uint>("contentLength");

    QTest::newRow( "path_01" ) << QtNetworkSettings::serverName() << 80u
	<< QString("/qtest/rfc3252.txt") << 1 << 200 << 25962u;

    QTest::newRow( "path_02" ) << QString("www.ietf.org") << 80u
	<< QString("/rfc/rfc3252.txt") << 1 << 200 << 25962u;

    QTest::newRow( "uri_01" ) << QtNetworkSettings::serverName() << 80u
	<< QString("http://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt") << 1 << 200 << 25962u;

    QTest::newRow( "uri_02" ) << QString("www.ietf.org") << 80u
    << QString("http://www.ietf.org/rfc/rfc3252.txt") << 1 << 200 << 25962u;

    QTest::newRow( "fail_01" ) << QString("this-host-will-not-exist.") << 80u
    << QString("/qtest/rfc3252.txt") << 0 << 0 << 0u;

    QTest::newRow( "failprot_01" ) << QtNetworkSettings::serverName() << 80u
	<< QString("/t") << 1 << 404 << 0u;

    QTest::newRow( "failprot_02" ) << QtNetworkSettings::serverName() << 80u
	<< QString("qtest/rfc3252.txt") << 1 << 400 << 0u;

    /* qt.nokia.com/doc no longer seams to be using chuncked encodig.
    QTest::newRow( "chunked_01" ) << QString("qt.nokia.com/doc") << 80u
	<< QString("/index.html") << 1 << 200 << 0u;
    */
    QTest::newRow( "chunked_02" ) << QtNetworkSettings::serverName() << 80u
	<< QString("/qtest/cgi-bin/rfc.cgi") << 1 << 200 << 0u;
}

void tst_QHttp::head()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, path );

    http = newHttp();
    QCOMPARE( http->currentId(), 0 );
    QCOMPARE( (int)http->state(), (int)QHttp::Unconnected );

    addRequest( QHttpRequestHeader(), http->setHost( host, port ) );
    headId = http->head( path );
    addRequest( QHttpRequestHeader(), headId );

    QTestEventLoop::instance().enterLoop( 30 );
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( "Network operation timed out" );

    ResMapIt res = resultMap.find( headId );
    QVERIFY( res != resultMap.end() );
    if ( res.value().success!=1 && host=="www.ietf.org" ) {
        // The nightly tests fail from time to time. In order to make them more
        // stable, add some debug output that might help locate the problem (I
        // can't reproduce the problem in the non-nightly builds).
        qDebug( "Error %d: %s", http->error(), http->errorString().toLatin1().constData() );
    }
    QTEST( res.value().success, "success" );
    if ( res.value().success ) {
        QTEST( res.value().resp.statusCode(), "statusCode" );
        QTEST( res.value().resp.contentLength(), "contentLength" );

        QCOMPARE( (uint)readyRead_ba.size(), 0u );
        QVERIFY( bytesTotalRead == bytesTotal_init );
        QVERIFY( bytesDoneRead == bytesDone_init );
    } else {
        QVERIFY( !res.value().resp.isValid() );
    }
}

void tst_QHttp::post_data()
{
	QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("useIODevice");
    QTest::addColumn<bool>("useProxy");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<bool>("ssl");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QByteArray>("result");

    QByteArray md5sum;
    md5sum = "d41d8cd98f00b204e9800998ecf8427e";
    QTest::newRow("empty-data")
        << QString() << false << false
        << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi" << md5sum;
    QTest::newRow("empty-device")
        << QString() << true << false
        << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi" << md5sum;
    QTest::newRow("proxy-empty-data")
        << QString() << false << true
        << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi" << md5sum;

    md5sum = "b3e32ac459b99d3f59318f3ac31e4bee";
    QTest::newRow("data") << "rfc3252.txt" << false << false
                          << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi"
                          << md5sum;
    QTest::newRow("device") << "rfc3252.txt" << true << false
                          << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi"
                          << md5sum;
    QTest::newRow("proxy-data") << "rfc3252.txt" << false << true
                                << QtNetworkSettings::serverName() << 80 << false << "/qtest/cgi-bin/md5sum.cgi"
                                << md5sum;

#ifndef QT_NO_OPENSSL
    md5sum = "d41d8cd98f00b204e9800998ecf8427e";
    QTest::newRow("empty-data-ssl")
        << QString() << false << false
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi" << md5sum;
    QTest::newRow("empty-device-ssl")
        << QString() << true << false
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi" << md5sum;
    QTest::newRow("proxy-empty-data-ssl")
        << QString() << false << true
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi" << md5sum;
    md5sum = "b3e32ac459b99d3f59318f3ac31e4bee";
    QTest::newRow("data-ssl") << "rfc3252.txt" << false << false
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi"
        << md5sum;
    QTest::newRow("device-ssl") << "rfc3252.txt" << true << false
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi"
        << md5sum;
    QTest::newRow("proxy-data-ssl") << "rfc3252.txt" << false << true
        << QtNetworkSettings::serverName() << 443 << true << "/qtest/cgi-bin/md5sum.cgi"
        << md5sum;
#endif

    // the following test won't work. See task 185996
/*
    QTest::newRow("proxy-device") << "rfc3252.txt" << true << true
                                  << QtNetworkSettings::serverName() << 80 << "/qtest/cgi-bin/md5sum.cgi"
                                  << md5sum;
*/
}

void tst_QHttp::post()
{
	QFETCH(QString, source);
    QFETCH(bool, useIODevice);
    QFETCH(bool, useProxy);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(bool, ssl);
    QFETCH(QString, path);

    http = newHttp(useProxy);
#ifndef QT_NO_OPENSSL
    QObject::connect(http, SIGNAL(sslErrors(const QList<QSslError> &)),
        http, SLOT(ignoreSslErrors()));
#endif
    QCOMPARE(http->currentId(), 0);
    QCOMPARE((int)http->state(), (int)QHttp::Unconnected);
    if (useProxy)
        addRequest(QHttpRequestHeader(), http->setProxy(QtNetworkSettings::serverName(), 3129));
    addRequest(QHttpRequestHeader(), http->setHost(host, (ssl ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp), port));

    // add the POST request
    QFile file(SRCDIR + source);
    QBuffer emptyBuffer;
    QIODevice *dev;
    if (!source.isEmpty()) {
        QVERIFY(file.open(QIODevice::ReadOnly));
        dev = &file;
    } else {
        emptyBuffer.open(QIODevice::ReadOnly);
        dev = &emptyBuffer;
    }

    if (useIODevice)
        postId = http->post(path, dev);
    else
        postId = http->post(path, dev->readAll());
    addRequest(QHttpRequestHeader(), postId);

    // run request
    connect(http, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    QTestEventLoop::instance().enterLoop( 30 );

    if ( QTestEventLoop::instance().timeout() )
    QFAIL( "Network operation timed out" );

    ResMapIt res = resultMap.find(postId);
    QVERIFY(res != resultMap.end());
    QVERIFY(res.value().success);
    QCOMPARE(res.value().resp.statusCode(), 200);
    QTEST(readyRead_ba.trimmed(), "result");
}

void tst_QHttp::request_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("useIODevice");
    QTest::addColumn<bool>("useProxy");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QByteArray>("result");

    QFile source(SRCDIR "rfc3252.txt");
    if (!source.open(QIODevice::ReadOnly))
        return;

    QByteArray contents = source.readAll();
    QByteArray md5sum = QCryptographicHash::hash(contents, QCryptographicHash::Md5).toHex() + '\n';
    QByteArray emptyMd5sum = "d41d8cd98f00b204e9800998ecf8427e\n";

    QTest::newRow("head") << QString() << false << false << QtNetworkSettings::serverName() << 80
                          << "HEAD" << "/qtest/rfc3252.txt"
                          << QByteArray();
    QTest::newRow("get") << QString() << false << false << QtNetworkSettings::serverName() << 80
                         << "GET" << "/qtest/rfc3252.txt"
                         << contents;
    QTest::newRow("post-empty-data") << QString() << false << false
                                     << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                                     << emptyMd5sum;
    QTest::newRow("post-empty-device") << QString() << true << false
                                       << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                                       << emptyMd5sum;
    QTest::newRow("post-data") << "rfc3252.txt" << false << false
                               << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                               << md5sum;
    QTest::newRow("post-device") << "rfc3252.txt" << true << false
                               << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                               << md5sum;

    QTest::newRow("proxy-head") << QString() << false << true << QtNetworkSettings::serverName() << 80
                                << "HEAD" << "/qtest/rfc3252.txt"
                                << QByteArray();
    QTest::newRow("proxy-get") << QString() << false << true << QtNetworkSettings::serverName() << 80
                               << "GET" << "/qtest/rfc3252.txt"
                               << contents;
    QTest::newRow("proxy-post-empty-data")  << QString() << false << true
                                            << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                                            << emptyMd5sum;
    QTest::newRow("proxy-post-data") << "rfc3252.txt" << false << true
                                     << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                                     << md5sum;
    // the following test won't work. See task 185996
/*
    QTest::newRow("proxy-post-device") << "rfc3252.txt" << true << true
                                       << QtNetworkSettings::serverName() << 80 << "POST" << "/qtest/cgi-bin/md5sum.cgi"
                                       << md5sum;
*/
}

void tst_QHttp::request()
{
    QFETCH(QString, source);
    QFETCH(bool, useIODevice);
    QFETCH(bool, useProxy);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, method);
    QFETCH(QString, path);

    http = newHttp(useProxy);
    QCOMPARE(http->currentId(), 0);
    QCOMPARE((int)http->state(), (int)QHttp::Unconnected);
    if (useProxy)
        addRequest(QHttpRequestHeader(), http->setProxy(QtNetworkSettings::serverName(), 3129));
    addRequest(QHttpRequestHeader(), http->setHost(host, port));

    QFile file(SRCDIR + source);
    QBuffer emptyBuffer;
    QIODevice *dev;
    if (!source.isEmpty()) {
        QVERIFY(file.open(QIODevice::ReadOnly));
        dev = &file;
    } else {
        emptyBuffer.open(QIODevice::ReadOnly);
        dev = &emptyBuffer;
    }

    // prepare the request
    QHttpRequestHeader request;
    request.setRequest(method, path, 1,1);
    request.addValue("Host", host);
    int *theId;

    if (method == "POST")
        theId = &postId;
    else if (method == "GET")
        theId = &getId;
    else if (method == "HEAD")
        theId = &headId;
    else
        QFAIL("You're lazy! Please implement your test!");

    // now send the request
    if (useIODevice)
        *theId = http->request(request, dev);
    else
        *theId = http->request(request, dev->readAll());
    addRequest(QHttpRequestHeader(), *theId);

    // run request
    connect(http, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    QTestEventLoop::instance().enterLoop( 30 );

    if ( QTestEventLoop::instance().timeout() )
    QFAIL( "Network operation timed out" );

    ResMapIt res = resultMap.find(*theId);
    QVERIFY(res != resultMap.end());
    QVERIFY(res.value().success);
    QCOMPARE(res.value().resp.statusCode(), 200);
    QTEST(readyRead_ba, "result");
}

void tst_QHttp::authorization_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("pass");
    QTest::addColumn<int>("result");

    QTest::newRow("correct password") << QtNetworkSettings::serverName()
                                << QString::fromLatin1("/qtest/rfcs-auth/index.html")
                                << QString::fromLatin1("httptest")
                                << QString::fromLatin1("httptest")
                                << 200;

    QTest::newRow("no password") << QtNetworkSettings::serverName()
                                 << QString::fromLatin1("/qtest/rfcs-auth/index.html")
                                 << QString::fromLatin1("")
                                 << QString::fromLatin1("")
                                 << 401;

    QTest::newRow("wrong password") << QtNetworkSettings::serverName()
                                 << QString::fromLatin1("/qtest/rfcs-auth/index.html")
                                 << QString::fromLatin1("maliciu0s")
                                 << QString::fromLatin1("h4X0r")
                                 << 401;
}

void tst_QHttp::authorization()
{
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(QString, user);
    QFETCH(QString, pass);
    QFETCH(int, result);

    QEventLoop loop;

    QHttp http;
    connect(&http, SIGNAL(done(bool)), &loop, SLOT(quit()));

    if (!user.isEmpty())
        http.setUser(user, pass);
    http.setHost(host);
    int id = http.get(path);

    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(http.lastResponse().statusCode(), result);
}

void tst_QHttp::proxy_data()
{
    QTest::addColumn<QString>("proxyhost");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("proxyuser");
    QTest::addColumn<QString>("proxypass");

    QTest::newRow("qt-test-server") << QtNetworkSettings::serverName() << 3128
                                 << QString::fromLatin1("qt.nokia.com") << QString::fromLatin1("/")
                                 << QString::fromLatin1("") << QString::fromLatin1("");
    QTest::newRow("qt-test-server pct") << QtNetworkSettings::serverName() << 3128
                                 << QString::fromLatin1("qt.nokia.com") << QString::fromLatin1("/%64eveloper")
                                 << QString::fromLatin1("") << QString::fromLatin1("");
    QTest::newRow("qt-test-server-basic") << QtNetworkSettings::serverName() << 3129
                                 << QString::fromLatin1("qt.nokia.com") << QString::fromLatin1("/")
                                 << QString::fromLatin1("qsockstest") << QString::fromLatin1("password");

#if 0
    // NTLM requires sending the same request three times for it to work
    // the tst_QHttp class is too strict to handle the byte counts sent by dataSendProgress
    // So don't run this test:
    QTest::newRow("qt-test-server-ntlm") << QtNetworkSettings::serverName() << 3130
                                 << QString::fromLatin1("qt.nokia.com") << QString::fromLatin1("/")
                                 << QString::fromLatin1("qsockstest") << QString::fromLatin1("password");
#endif
}

void tst_QHttp::proxy()
{
    QFETCH(QString, proxyhost);
    QFETCH(int, port);
    QFETCH(QString, host);
    QFETCH(QString, path);
    QFETCH(QString, proxyuser);
    QFETCH(QString, proxypass);

    http = newHttp(!proxyuser.isEmpty());

    QCOMPARE(http->currentId(), 0);
    QCOMPARE((int)http->state(), (int)QHttp::Unconnected);

    addRequest(QHttpRequestHeader(), http->setProxy(proxyhost, port, proxyuser, proxypass));
    addRequest(QHttpRequestHeader(), http->setHost(host));
    getId = http->get(path);
    addRequest(QHttpRequestHeader(), getId);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
    QFAIL("Network operation timed out");

    ResMapIt res = resultMap.find(getId);
    QVERIFY(res != resultMap.end());
    QVERIFY(res.value().success);
    QCOMPARE(res.value().resp.statusCode(), 200);
}

void tst_QHttp::proxy2()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    readyRead_ba.clear();

    QHttp http;
    http.setProxy(QtNetworkSettings::serverName(), 3128);
    http.setHost(QtNetworkSettings::serverName());
    http.get("/index.html");
    http.get("/index.html");

    connect(&http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(proxy2_slot()));
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(readyRead_ba.count("Welcome to qt-test-server"), 2);

    readyRead_ba.clear();
}

void tst_QHttp::proxy2_slot()
{
    QHttp *http = static_cast<QHttp *>(sender());
    readyRead_ba.append(http->readAll());
    if (!http->hasPendingRequests())
        QTestEventLoop::instance().exitLoop();
}

void tst_QHttp::proxy3()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    readyRead_ba.clear();

    QTcpSocket socket;
    socket.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::serverName(), 3128));

    QHttp http;
    http.setSocket(&socket);
    http.setHost(QtNetworkSettings::serverName());
    http.get("/index.html");
    http.get("/index.html");

    connect(&http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(proxy2_slot()));
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(readyRead_ba.count("Welcome to qt-test-server"), 2);

    readyRead_ba.clear();
}

// test QHttp::currentId() and QHttp::currentRequest()
#define CURRENTREQUEST_TEST \
    { \
    ResMapIt res = resultMap.find( http->currentId() ); \
    QVERIFY( res != resultMap.end() ); \
    if ( http->currentId() == getId ) { \
        QCOMPARE( http->currentRequest().method(), QString("GET") ); \
    } else if ( http->currentId() == headId ) { \
        QCOMPARE( http->currentRequest().method(), QString("HEAD") ); \
        } else if ( http->currentId() == postId ) { \
            QCOMPARE( http->currentRequest().method(), QString("POST") ); \
    } else { \
        QVERIFY( headerAreEqual( http->currentRequest(), res.value().req ) ); \
    } \
    }

void tst_QHttp::requestStarted( int id )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:requestStarted( %d )", http->currentId(), id );
#endif
    // make sure that the requestStarted and requestFinished are nested correctly
    QVERIFY( current_id == 0 );
    current_id = id;

    QVERIFY( !ids.isEmpty() );
    QVERIFY( ids.first() == id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }

    QVERIFY( http->currentId() == id );
    QVERIFY( cur_state == http->state() );




    CURRENTREQUEST_TEST;

    QVERIFY( http->error() == QHttp::NoError );
}

void tst_QHttp::requestFinished( int id, bool error )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:requestFinished( %d, %d ) -- errorString: '%s'",
        http->currentId(), id, (int)error, http->errorString().toAscii().data() );
#endif
    // make sure that the requestStarted and requestFinished are nested correctly
    QVERIFY( current_id == id );
    current_id = 0;

    QVERIFY( !ids.isEmpty() );
    QVERIFY( ids.first() == id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }

    if ( error ) {
    QVERIFY( http->error() != QHttp::NoError );
    ids.clear();
    } else {
    QVERIFY( http->error() == QHttp::NoError );
    ids.pop_front();
    }

    QVERIFY( http->currentId() == id );
    QVERIFY( cur_state == http->state() );
    CURRENTREQUEST_TEST;

    ResMapIt res = resultMap.find( http->currentId() );
    QVERIFY( res != resultMap.end() );
    QVERIFY( res.value().success == -1 );
    if ( error )
    res.value().success = 0;
    else
    res.value().success = 1;
}

void tst_QHttp::done( bool error )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:done( %d )", http->currentId(), (int)error );
#endif
    QVERIFY( http->currentId() == 0 );
    QVERIFY( current_id == 0 );
    QVERIFY( ids.isEmpty() );
    QVERIFY( cur_state == http->state() );
    QVERIFY( !http->hasPendingRequests() );

    QVERIFY( done_success == -1 );
    if ( error ) {
    QVERIFY( http->error() != QHttp::NoError );
    done_success = 0;
    } else {
    QVERIFY( http->error() == QHttp::NoError );
    done_success = 1;
    }
    QTestEventLoop::instance().exitLoop();
}

void tst_QHttp::stateChanged( int state )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  stateChanged( %d )", http->currentId(), state );
#endif
    QCOMPARE( http->currentId(), current_id );
    if ( ids.count() > 0 )
    CURRENTREQUEST_TEST;

    QVERIFY( state != cur_state );
    QVERIFY( state == http->state() );
    if ( state != QHttp::Unconnected && !connectionWithAuth ) {
    // make sure that the states are always emitted in the right order (for
    // this, we assume an ordering on the enum values, which they have at
    // the moment)
        // connections with authentication will possibly reconnect, so ignore them
        QVERIFY( cur_state < state );
    }
    cur_state = state;

    if (state == QHttp::Connecting) {
        bytesTotalSend = bytesTotal_init;
        bytesDoneSend = bytesDone_init;
        bytesTotalRead = bytesTotal_init;
        bytesDoneRead = bytesDone_init;
    }
}

void tst_QHttp::responseHeaderReceived( const QHttpResponseHeader &header )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  responseHeaderReceived(\n---{\n%s}---)", http->currentId(), header.toString().toAscii().data() );
#endif
    QCOMPARE( http->currentId(), current_id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }
    CURRENTREQUEST_TEST;

    resultMap[ http->currentId() ].resp = header;
}

void tst_QHttp::readyRead( const QHttpResponseHeader & )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  readyRead()", http->currentId() );
#endif
    QCOMPARE( http->currentId(), current_id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }
    QVERIFY( cur_state == http->state() );
    CURRENTREQUEST_TEST;

    if ( QTest::currentTestFunction() != QLatin1String("bytesAvailable") ) {
    int oldSize = readyRead_ba.size();
    quint64 bytesAvail = http->bytesAvailable();
    QByteArray ba = http->readAll();
    QVERIFY( (quint64) ba.size() == bytesAvail );
    readyRead_ba.resize( oldSize + ba.size() );
    memcpy( readyRead_ba.data()+oldSize, ba.data(), ba.size() );

    if ( bytesTotalRead > 0 ) {
        QVERIFY( (int)readyRead_ba.size() <= bytesTotalRead );
    }
    QVERIFY( (int)readyRead_ba.size() == bytesDoneRead );
    }
}

void tst_QHttp::dataSendProgress( int done, int total )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  dataSendProgress( %d, %d )", http->currentId(), done, total );
#endif
    QCOMPARE( http->currentId(), current_id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }
    QVERIFY( cur_state == http->state() );
    CURRENTREQUEST_TEST;

    if ( bytesTotalSend == bytesTotal_init ) {
    bytesTotalSend = total;
    } else {
    QCOMPARE( bytesTotalSend, total );
    }

    QVERIFY( bytesTotalSend != bytesTotal_init );
    QVERIFY( bytesDoneSend <= done );
    bytesDoneSend = done;
    if ( bytesTotalSend > 0 ) {
    QVERIFY( bytesDoneSend <= bytesTotalSend );
    }

    if ( QTest::currentTestFunction() == QLatin1String("abort") ) {
    // ### it would be nice if we could specify in our testdata when to do
    // the abort
    if ( done >= total/100000 ) {
        if ( ids.count() != 1 ) {
        // do abort only once
        int tmpId = ids.first();
        ids.clear();
        ids << tmpId;
        http->abort();
        }
    }
    }
}

void tst_QHttp::dataReadProgress( int done, int total )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  dataReadProgress( %d, %d )", http->currentId(), done, total );
#endif
    QCOMPARE( http->currentId(), current_id );
    if ( ids.count() > 1 ) {
    QVERIFY( http->hasPendingRequests() );
    } else {
    QVERIFY( !http->hasPendingRequests() );
    }
    QVERIFY( cur_state == http->state() );
    CURRENTREQUEST_TEST;

    if ( bytesTotalRead == bytesTotal_init )
    bytesTotalRead = total;
    else {
    QVERIFY( bytesTotalRead == total );
    }

    QVERIFY( bytesTotalRead != bytesTotal_init );
    QVERIFY( bytesDoneRead <= done );
    bytesDoneRead = done;
    if ( bytesTotalRead > 0 ) {
    QVERIFY( bytesDoneRead <= bytesTotalRead );
    }

    if ( QTest::currentTestFunction() == QLatin1String("abort") ) {
    // ### it would be nice if we could specify in our testdata when to do
    // the abort
    if ( done >= total/100000 ) {
        if ( ids.count() != 1 ) {
        // do abort only once
        int tmpId = ids.first();
        ids.clear();
        ids << tmpId;
        http->abort();
        }
    }
    }
}


QHttp *tst_QHttp::newHttp(bool withAuth)
{
    QHttp *nHttp = new QHttp( 0 );
    connect( nHttp, SIGNAL(requestStarted(int)),
        SLOT(requestStarted(int)) );
    connect( nHttp, SIGNAL(requestFinished(int,bool)),
        SLOT(requestFinished(int,bool)) );
    connect( nHttp, SIGNAL(done(bool)),
        SLOT(done(bool)) );
    connect( nHttp, SIGNAL(stateChanged(int)),
        SLOT(stateChanged(int)) );
    connect( nHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
        SLOT(responseHeaderReceived(const QHttpResponseHeader&)) );
    connect( nHttp, SIGNAL(readyRead(const QHttpResponseHeader&)),
        SLOT(readyRead(const QHttpResponseHeader&)) );
    connect( nHttp, SIGNAL(dataSendProgress(int,int)),
        SLOT(dataSendProgress(int,int)) );
    connect( nHttp, SIGNAL(dataReadProgress(int,int)),
        SLOT(dataReadProgress(int,int)) );

    connectionWithAuth = withAuth;
    return nHttp;
}

void tst_QHttp::addRequest( QHttpRequestHeader header, int id )
{
    ids << id;
    RequestResult res;
    res.req = header;
    res.success = -1;
    resultMap[ id ] = res;
}

bool tst_QHttp::headerAreEqual( const QHttpHeader &h1, const QHttpHeader &h2 )
{
    if ( !h1.isValid() )
    return !h2.isValid();
    if ( !h2.isValid() )
    return !h1.isValid();

    return h1.toString() == h2.toString();
}


void tst_QHttp::reconnect()
{
    reconnect_state_connect_count = 0;

    QHttp http;

    QObject::connect(&http, SIGNAL(stateChanged(int)), this, SLOT(reconnect_state(int)));
    http.setHost("trolltech.com", 80);
    http.get("/company/index.html");
    http.setHost("trolltech.com", 8080);
    http.get("/company/index.html");

    QTestEventLoop::instance().enterLoop(60);
    if (QTestEventLoop::instance().timeout())
    QFAIL("Network operation timed out");

    QCOMPARE(reconnect_state_connect_count, 1);

    QTestEventLoop::instance().enterLoop(60);
    if (QTestEventLoop::instance().timeout())
    QFAIL("Network operation timed out");

    QCOMPARE(reconnect_state_connect_count, 2);
}

void tst_QHttp::reconnect_state(int state)
{
    if (state == QHttp::Connecting) {
        ++reconnect_state_connect_count;
        QTestEventLoop::instance().exitLoop();
    }
}

void tst_QHttp::setSocket()
{
    QHttp *http = new QHttp;
    QPointer<QTcpSocket> replacementSocket = new QTcpSocket;
    http->setSocket(replacementSocket);
    QCoreApplication::processEvents();
    delete http;
    QVERIFY(replacementSocket);
    delete replacementSocket;
}

class Server : public QTcpServer
{
    Q_OBJECT
public:
    Server()
    {
        connect(this, SIGNAL(newConnection()),
                this, SLOT(serveConnection()));
    }

private slots:
    void serveConnection()
    {
        QTcpSocket *socket = nextPendingConnection();
        socket->write("HTTP/1.1 404 Not found\r\n"
                      "content-length: 4\r\n\r\nabcd");
        socket->disconnectFromHost();
    };
};

void tst_QHttp::unexpectedRemoteClose()
{
	QFETCH_GLOBAL(int, proxyType);
    if (proxyType == QNetworkProxy::Socks5Proxy) {
        // This test doesn't make sense for SOCKS5
        return;
    }

    Server server;
    server.listen();
    QCoreApplication::instance()->processEvents();

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, SLOT(quit()));

    QHttp http;
    QObject::connect(&http, SIGNAL(done(bool)), &loop, SLOT(quit()));
    QSignalSpy finishedSpy(&http, SIGNAL(requestFinished(int, bool)));
    QSignalSpy doneSpy(&http, SIGNAL(done(bool)));

    http.setHost("localhost", server.serverPort());
    http.get("/");
    http.get("/");
    http.get("/");

    loop.exec();

    QCOMPARE(finishedSpy.count(), 4);
    QVERIFY(!finishedSpy.at(1).at(1).toBool());
    QVERIFY(!finishedSpy.at(2).at(1).toBool());
    QVERIFY(!finishedSpy.at(3).at(1).toBool());
    QCOMPARE(doneSpy.count(), 1);
    QVERIFY(!doneSpy.at(0).at(0).toBool());
}

void tst_QHttp::pctEncodedPath()
{
    QHttpRequestHeader header;
    header.setRequest("GET", "/index.asp/a=%20&b=%20&c=%20");
    QCOMPARE(header.toString(), QString("GET /index.asp/a=%20&b=%20&c=%20 HTTP/1.1\r\n\r\n"));
}

void tst_QHttp::caseInsensitiveKeys()
{
    QHttpResponseHeader header("HTTP/1.1 200 OK\r\nContent-Length: 213\r\nX-Been-There: True\r\nLocation: http://www.TrollTech.com/\r\n\r\n");
    QVERIFY(header.hasKey("Content-Length"));
    QVERIFY(header.hasKey("X-Been-There"));
    QVERIFY(header.hasKey("Location"));
    QVERIFY(header.hasKey("content-length"));
    QVERIFY(header.hasKey("x-been-there"));
    QVERIFY(header.hasKey("location"));
    QCOMPARE(header.value("Content-Length"), QString("213"));
    QCOMPARE(header.value("X-Been-There"), QString("True"));
    QCOMPARE(header.value("Location"), QString("http://www.TrollTech.com/"));
    QCOMPARE(header.value("content-length"), QString("213"));
    QCOMPARE(header.value("x-been-there"), QString("True"));
    QCOMPARE(header.value("location"), QString("http://www.TrollTech.com/"));
    QCOMPARE(header.allValues("location"), QStringList("http://www.TrollTech.com/"));

    header.addValue("Content-Length", "213");
    header.addValue("Content-Length", "214");
    header.addValue("Content-Length", "215");
    qDebug() << header.toString();
}

void tst_QHttp::proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    proxyAuthCalled = true;
    auth->setUser("qsockstest");
    auth->setPassword("password");
}

void tst_QHttp::postAuthNtlm()
{
	QSKIP("NTLM not working", SkipAll);

    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    QByteArray postData("Hello World");
    QHttp http;

    http.setHost(QtNetworkSettings::serverName());
    http.setProxy(QtNetworkSettings::serverName(), 3130);
    http.post("/", postData);

    proxyAuthCalled = false;
    connect(&http, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));

    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(3);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY(proxyAuthCalled);
    QVERIFY(!QTestEventLoop::instance().timeout());
};

void tst_QHttp::proxyAndSsl()
{
#ifdef QT_NO_OPENSSL
    QSKIP("No OpenSSL support in this platform", SkipAll);
#else
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QHttp http;

    http.setHost(QtNetworkSettings::serverName(), QHttp::ConnectionModeHttps);
    http.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::serverName(), 3129));
    http.get("/");

    proxyAuthCalled = false;
    connect(&http, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    connect(&http, SIGNAL(sslErrors(QList<QSslError>)),
            &http, SLOT(ignoreSslErrors()));

    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(3);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(proxyAuthCalled);

    QHttpResponseHeader header = http.lastResponse();
    QVERIFY(header.isValid());
    QVERIFY(header.statusCode() < 400); // Should be 200, but as long as it's not an error, we're happy
#endif
}

void tst_QHttp::cachingProxyAndSsl()
{
#ifdef QT_NO_OPENSSL
    QSKIP("No OpenSSL support in this platform", SkipAll);
#else
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QHttp http;

    http.setHost(QtNetworkSettings::serverName(), QHttp::ConnectionModeHttps);
    http.setProxy(QNetworkProxy(QNetworkProxy::HttpCachingProxy, QtNetworkSettings::serverName(), 3129));
    http.get("/");

    proxyAuthCalled = false;
    connect(&http, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
            SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));

    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(3);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(!proxyAuthCalled);  // NOT called! QHttp should get a socket error
    QVERIFY(http.state() != QHttp::Connected);

    QHttpResponseHeader header = http.lastResponse();
    QVERIFY(!header.isValid());
#endif
}

void tst_QHttp::emptyBodyInReply()
{
    // Note: if this test starts failing, please verify the date on the file
    // returned by Apache on http://netiks.troll.no/
    // It is right now hard-coded to the date below
    QHttp http;
    http.setHost(QtNetworkSettings::serverName());

    QHttpRequestHeader headers("GET", "/");
    headers.addValue("If-Modified-Since", "Sun, 16 Nov 2008 12:29:51 GMT");
    headers.addValue("Host", QtNetworkSettings::serverName());
    http.request(headers);

    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY(!QTestEventLoop::instance().timeout());

    // check the reply
    if (http.lastResponse().statusCode() != 304) {
        qWarning() << http.lastResponse().statusCode() << qPrintable(http.lastResponse().reasonPhrase());
        qWarning() << "Last-Modified:" << qPrintable(http.lastResponse().value("last-modified"));
        QFAIL("Server replied with the wrong status code; see warning output");
    }
}

void tst_QHttp::abortSender()
{
    QHttp *http = qobject_cast<QHttp *>(sender());
    if (http)
        http->abort();
}

void tst_QHttp::abortInReadyRead()
{
    QHttp http;
    http.setHost(QtNetworkSettings::serverName());
    http.get("/qtest/bigfile");

    qRegisterMetaType<QHttpResponseHeader>();
    QSignalSpy spy(&http, SIGNAL(readyRead(QHttpResponseHeader)));

    QObject::connect(&http, SIGNAL(readyRead(QHttpResponseHeader)), this, SLOT(abortSender()));
    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY2(!QTestEventLoop::instance().timeout(), "Network timeout");
    QVERIFY(http.state() != QHttp::Connected);

    QCOMPARE(spy.count(), 1);
}

void tst_QHttp::abortInResponseHeaderReceived()
{
    QHttp http;
    http.setHost(QtNetworkSettings::serverName());
    http.get("/qtest/bigfile");

    qRegisterMetaType<QHttpResponseHeader>();
    QSignalSpy spy(&http, SIGNAL(readyRead(QHttpResponseHeader)));

    QObject::connect(&http, SIGNAL(responseHeaderReceived(QHttpResponseHeader)), this, SLOT(abortSender()));
    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY2(!QTestEventLoop::instance().timeout(), "Network timeout");
    QVERIFY(http.state() != QHttp::Connected);

    QCOMPARE(spy.count(), 0);
}

void tst_QHttp::connectionClose()
{
    // This was added in response to bug 176822
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    QHttp http;
    ClosingServer server;
    http.setHost("localhost", QHttp::ConnectionModeHttps, server.serverPort());
    http.get("/login/gateway/processLogin");

    // another possibility:
    //http.setHost("nexus.passport.com", QHttp::ConnectionModeHttps, 443);
    //http.get("/rdr/pprdr.asp");

    QObject::connect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(900);
    QObject::disconnect(&http, SIGNAL(done(bool)), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QHttp::nestedEventLoop_slot(int id)
{
    if (!ids.contains(id))
        return;
    QEventLoop subloop;

    // 16 seconds: fluke times out in 15 seconds, which triggers a QTcpSocket error
    QTimer::singleShot(16000, &subloop, SLOT(quit()));
    subloop.exec();

    QTestEventLoop::instance().exitLoop();
}

void tst_QHttp::nestedEventLoop()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        return;

    http = new QHttp;
    http->setHost(QtNetworkSettings::serverName());
    int getId = http->get("/");

    ids.clear();
    ids << getId;

    QSignalSpy spy(http, SIGNAL(requestStarted(int)));
    QSignalSpy spy2(http, SIGNAL(done(bool)));

    connect(http, SIGNAL(requestFinished(int,bool)), SLOT(nestedEventLoop_slot(int)));
    QTestEventLoop::instance().enterLoop(20);

    QVERIFY2(!QTestEventLoop::instance().timeout(), "Network timeout");

    // Find out how many signals with the first argument equalling our id were found
    int spyCount = 0;
    for (int i = 0; i < spy.count(); ++i)
        if (spy.at(i).at(0).toInt() == getId)
            ++spyCount;

    // each signal spied should have been emitted only once
    QCOMPARE(spyCount, 1);
    QCOMPARE(spy2.count(), 1);
}

QTEST_MAIN(tst_QHttp)
#include "tst_qhttp.moc"
