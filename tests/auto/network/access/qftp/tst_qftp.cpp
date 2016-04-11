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

#include <qcoreapplication.h>
#include <qfile.h>
#include <qbuffer.h>
#include "private/qftp_p.h"
#include <qmap.h>
#include <time.h>
#include <stdlib.h>
#include <QNetworkProxy>
#include <QNetworkConfiguration>
#include <qnetworkconfigmanager.h>
#include <QNetworkSession>
#include <QtNetwork/private/qnetworksession_p.h>

#include "../../../network-settings.h"

template <class T1, class T2>
static QByteArray msgComparison(T1 lhs, const char *op, T2 rhs)
{
    QString result;
    QTextStream(&result) << lhs << ' ' << op << ' ' << rhs;
    return result.toLatin1();
}

class tst_QFtp : public QObject
{
    Q_OBJECT

public:
    tst_QFtp();

private slots:
    void initTestCase_data();
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    void connectToHost_data();
    void connectToHost();
    void connectToUnresponsiveHost();
    void login_data();
    void login();
    void close_data();
    void close();

    void list_data();
    void list();
    void cd_data();
    void cd();
    void get_data();
    void get();
    void put_data();
    void put();
    void mkdir_data();
    void mkdir();
    void mkdir2();
    void rename_data();
    void rename();

    void commandSequence_data();
    void commandSequence();

    void abort_data();
    void abort();

    void bytesAvailable_data();
    void bytesAvailable();

    void activeMode();

    void proxy_data();
    void proxy();

    void binaryAscii();

    void doneSignal();
    void queueMoreCommandsInDoneSlot();

    void qtbug7359Crash();

protected slots:
    void stateChanged( int );
    void listInfo( const QUrlInfo & );
    void readyRead();
    void dataTransferProgress(qint64, qint64);

    void commandStarted( int );
    void commandFinished( int, bool );
    void done( bool );
    void activeModeDone( bool );
    void mkdir2Slot(int id, bool error);
    void cdUpSlot(bool);

private:
    QFtp *newFtp();
    void addCommand( QFtp::Command, int );
    bool fileExists( const QString &host, quint16 port, const QString &user, const QString &password, const QString &file, const QString &cdDir = QString::null );
    bool dirExists( const QString &host, quint16 port, const QString &user, const QString &password, const QString &cdDir, const QString &dirToCreate );

    void renameInit( const QString &host, const QString &user, const QString &password, const QString &createFile );
    void renameCleanup( const QString &host, const QString &user, const QString &password, const QString &fileToDelete );

    QFtp *ftp;
#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSessionExplicit;
    QSharedPointer<QNetworkSession> networkSessionImplicit;
#endif

    QList<int> ids; // helper to make sure that all expected signals are emitted
    int current_id;

    int connectToHost_state;
    int close_state;
    int login_state;
    int cur_state;
    struct CommandResult
    {
        int id;
        int success;
    };
    QMap< QFtp::Command, CommandResult > resultMap;
    typedef QMap<QFtp::Command,CommandResult>::Iterator ResMapIt;

    int done_success;
    int commandSequence_success;

    qlonglong bytesAvailable_finishedGet;
    qlonglong bytesAvailable_finished;
    qlonglong bytesAvailable_done;

    QList<QUrlInfo> listInfo_i;
    QByteArray newData_ba;
    qlonglong bytesTotal;
    qlonglong bytesDone;

    bool inFileDirExistsFunction;

    QString uniqueExtension;
    QString rfc3252File;
};

//#define DUMP_SIGNALS

const int bytesTotal_init = -10;
const int bytesDone_init = -10;

tst_QFtp::tst_QFtp() :
    ftp(0)
{
}

void tst_QFtp::initTestCase_data()
{
    QTest::addColumn<bool>("setProxy");
    QTest::addColumn<int>("proxyType");
    QTest::addColumn<bool>("setSession");

    QTest::newRow("WithoutProxy") << false << 0 << false;
#ifndef QT_NO_SOCKS5
    QTest::newRow("WithSocks5Proxy") << true << int(QNetworkProxy::Socks5Proxy) << false;
#endif
    //### doesn't work well yet.
    //QTest::newRow("WithHttpProxy") << true << int(QNetworkProxy::HttpProxy);

#ifndef QT_NO_BEARERMANAGEMENT
    QTest::newRow("WithoutProxyWithSession") << false << 0 << true;
#ifndef QT_NO_SOCKS5
    QTest::newRow("WithSocks5ProxyAndSession") << true << int(QNetworkProxy::Socks5Proxy) << true;
#endif
#endif
}

void tst_QFtp::initTestCase()
{
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager manager;
    networkSessionImplicit = QSharedPointer<QNetworkSession>(new QNetworkSession(manager.defaultConfiguration()));
    networkSessionImplicit->open();
    QVERIFY(networkSessionImplicit->waitForOpened(60000)); //there may be user prompt on 1st connect
#endif
    rfc3252File = QFINDTESTDATA("rfc3252.txt");
    QVERIFY(!rfc3252File.isEmpty());
}

void tst_QFtp::cleanupTestCase()
{
#ifndef QT_NO_BEARERMANAGEMENT
    networkSessionExplicit.clear();
    networkSessionImplicit.clear();
#endif
}

void tst_QFtp::init()
{
    QFETCH_GLOBAL(bool, setProxy);
    QFETCH_GLOBAL(int, proxyType);
    QFETCH_GLOBAL(bool, setSession);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        if (proxyType == QNetworkProxy::Socks5Proxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));
        } else if (proxyType == QNetworkProxy::HttpProxy) {
            QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QtNetworkSettings::serverName(), 3128));
        }
#else // !QT_NO_NETWORKPROXY
        Q_UNUSED(proxyType);
        QSKIP("No proxy support");
#endif // QT_NO_NETWORKPROXY
    }
#ifndef QT_NO_BEARERMANAGEMENT
    if (setSession) {
        networkSessionExplicit = networkSessionImplicit;
        if (!networkSessionExplicit->isOpen()) {
            networkSessionExplicit->open();
            QVERIFY(networkSessionExplicit->waitForOpened(30000));
        }
    } else {
        networkSessionExplicit.clear();
    }
#endif

    delete ftp;
    ftp = 0;

    ids.clear();
    current_id = 0;

    resultMap.clear();
    connectToHost_state = -1;
    close_state = -1;
    login_state = -1;
    cur_state = QFtp::Unconnected;

    listInfo_i.clear();
    newData_ba = QByteArray();
    bytesTotal = bytesTotal_init;
    bytesDone = bytesDone_init;

    done_success = -1;
    commandSequence_success = -1;

    bytesAvailable_finishedGet = 1234567890;
    bytesAvailable_finished = 1234567890;
    bytesAvailable_done = 1234567890;

    inFileDirExistsFunction = false;

    srand(time(0));
    uniqueExtension = QString::number((quintptr)this) + QString::number(rand())
        + QString::number((qulonglong)time(0));
}

void tst_QFtp::cleanup()
{
    if (ftp) {
        delete ftp;
        ftp = 0;
    }
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
#ifndef QT_NO_NETWORKPROXY
        QNetworkProxy::setApplicationProxy(QNetworkProxy::DefaultProxy);
#else
        QSKIP("No proxy support");
#endif
    }

    delete ftp;
    ftp = 0;
#ifndef QT_NO_BEARERMANAGEMENT
    networkSessionExplicit.clear();
#endif
}

void tst_QFtp::connectToHost_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<int>("state");

    QTest::newRow( "ok01" ) << QtNetworkSettings::serverName() << (uint)21 << (int)QFtp::Connected;
    QTest::newRow( "error01" ) << QtNetworkSettings::serverName() << (uint)2222 << (int)QFtp::Unconnected;
    QTest::newRow( "error02" ) << QString("foo.bar") << (uint)21 << (int)QFtp::Unconnected;
}

static QByteArray msgTimedOut(const QString &host, quint16 port = 0)
{
    QByteArray result = "Network operation timed out on " + host.toLatin1();
    if (port) {
        result += ':';
        result += QByteArray::number(port);
    }
    return result;
}

void tst_QFtp::connectToHost()
{
    QFETCH( QString, host );
    QFETCH( uint, port );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );

    QTestEventLoop::instance().enterLoop( 61 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    QTEST( connectToHost_state, "state" );

    ResMapIt it = resultMap.find( QFtp::ConnectToHost );
    QVERIFY( it != resultMap.end() );
    QFETCH( int, state );
    if ( state == QFtp::Connected ) {
        QCOMPARE( it.value().success, 1 );
    } else {
        QCOMPARE( it.value().success , 0 );
    }
}

void tst_QFtp::connectToUnresponsiveHost()
{
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy)
        QSKIP( "This test takes too long if we test with proxies too");

    QString host = "192.0.2.42"; // IP out of TEST-NET, should be unreachable
    uint port = 21;

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );

    qDebug( "About to connect to host that won't reply (this test takes 60 seconds)" );
    QTestEventLoop::instance().enterLoop( 61 );
#ifdef Q_OS_WIN
    /* On Windows, we do not get a timeout, because Winsock is behaving in a strange way:
    We issue two "WSAConnect()" calls, after the first, as a result we get WSAEWOULDBLOCK,
    after the second, we get WSAEISCONN, which means that the socket is connected, which cannot be.
    However, after some seconds we get a socket error saying that the remote host closed the connection,
    which can neither be. For this test, that would actually enable us to finish before timout, but handling that case
    (in void QFtpPI::error(QAbstractSocket::SocketError e)) breaks
    a lot of other stuff in QFtp, so we just expect this test to fail on Windows.
    */
    QEXPECT_FAIL("", "timeout not working due to strange Windows socket behaviour (see source file of this test for explanation)", Abort);

#endif
    QVERIFY2(! QTestEventLoop::instance().timeout(), "Network timeout longer than expected (should have been 60 seconds)");

    QCOMPARE( ftp->state(), QFtp::Unconnected);
    ResMapIt it = resultMap.find( QFtp::ConnectToHost );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 0 );

    delete ftp;
    ftp = 0;
}

void tst_QFtp::login_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("success");

    QTest::newRow( "ok01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << 1;
    QTest::newRow( "ok02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftp") << QString() << 1;
    QTest::newRow( "ok03" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftp") << QString("foo") << 1;
    QTest::newRow( "ok04" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest") << QString("password") << 1;

    QTest::newRow( "error01" ) << QtNetworkSettings::serverName() << (uint)21 << QString("foo") << QString() << 0;
    QTest::newRow( "error02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("foo") << QString("bar") << 0;
}

void tst_QFtp::login()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    ResMapIt it = resultMap.find( QFtp::Login );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );

    const QFtp::State loginState = static_cast<QFtp::State>(login_state);
    if ( it.value().success ) {
        QCOMPARE( loginState, QFtp::LoggedIn );
    } else {
        QVERIFY2( loginState != QFtp::LoggedIn, msgComparison(loginState, "!=", QFtp::LoggedIn));
    }
}

void tst_QFtp::close_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<bool>("login");

    QTest::newRow( "login01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << true;
    QTest::newRow( "login02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftp") << QString() << true;
    QTest::newRow( "login03" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftp") << QString("foo") << true;
    QTest::newRow( "login04" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest") << QString("password") << true;

    QTest::newRow( "no-login01" ) << QtNetworkSettings::serverName() << (uint)21 << QString("") << QString("") << false;
}

void tst_QFtp::close()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( bool, login );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    if ( login )
        addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    QCOMPARE( close_state, (int)QFtp::Unconnected );

    ResMapIt it = resultMap.find( QFtp::Close );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 1 );
}

void tst_QFtp::list_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("dir");
    QTest::addColumn<int>("success");
    QTest::addColumn<QStringList>("entryNames"); // ### we should rather use a QList<QUrlInfo> here

    QStringList flukeRoot;
    flukeRoot << "pub";
    flukeRoot << "qtest";
    QStringList flukeQtest;
    flukeQtest << "bigfile";
    flukeQtest << "nonASCII";
    flukeQtest << "rfc3252";
    flukeQtest << "rfc3252.txt";
    flukeQtest << "upload";

    QTest::newRow( "workDir01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString() << 1 << flukeRoot;
    QTest::newRow( "workDir02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString() << 1 << flukeRoot;

    QTest::newRow( "relPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("qtest") << 1 << flukeQtest;
    QTest::newRow( "relPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("qtest") << 1 << flukeQtest;

    QTest::newRow( "absPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/qtest") << 1 << flukeQtest;
    QTest::newRow( "absPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("/var/ftp/qtest") << 1 << flukeQtest;

    QTest::newRow( "nonExist01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("foo")  << 1 << QStringList();
    QTest::newRow( "nonExist02" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/foo") << 1 << QStringList();
    // ### The microsoft server does not seem to work properly at the moment --
    // I am also not able to open a data connection with other, non-Qt FTP
    // clients to it.
    // QTest::newRow( "nonExist03" ) << "ftp.microsoft.com" << (uint)21 << QString() << QString() << QString("/foo") << 0 << QStringList();

    QStringList susePub;
    susePub << "README.mirror-policy" << "axp" << "i386" << "ia64" << "install" << "noarch" << "pubring.gpg-build.suse.de" << "update" << "x86_64";
    QTest::newRow( "epsvNotSupported" ) << QString("ftp.funet.fi") << (uint)21 << QString::fromLatin1("ftp") << QString::fromLatin1("root@") << QString("/pub/Linux/suse/suse") << 1 << susePub;
}

void tst_QFtp::list()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, dir );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::List, ftp->list( dir ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    ResMapIt it = resultMap.find( QFtp::List );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );
    QFETCH( QStringList, entryNames );
    QCOMPARE( listInfo_i.count(), entryNames.count() );
    for ( uint i=0; i < (uint) entryNames.count(); i++ ) {
        QCOMPARE( listInfo_i[i].name(), entryNames[i] );
    }
}

void tst_QFtp::cd_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("dir");
    QTest::addColumn<int>("success");
    QTest::addColumn<QStringList>("entryNames"); // ### we should rather use a QList<QUrlInfo> here

    QStringList flukeRoot;
    flukeRoot << "qtest";
    QStringList flukeQtest;
    flukeQtest << "bigfile";
    flukeQtest << "nonASCII";
    flukeQtest << "rfc3252";
    flukeQtest << "rfc3252.txt";
    flukeQtest << "upload";

    QTest::newRow( "relPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("qtest") << 1 << flukeQtest;
    QTest::newRow( "relPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("qtest") << 1 << flukeQtest;

    QTest::newRow( "absPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/qtest") << 1 << flukeQtest;
    QTest::newRow( "absPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("/var/ftp/qtest") << 1 << flukeQtest;

    QTest::newRow( "nonExist01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("foo")  << 0 << QStringList();
    QTest::newRow( "nonExist03" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/foo") << 0 << QStringList();
}

void tst_QFtp::cd()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, dir );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Cd, ftp->cd( dir ) );
    addCommand( QFtp::List, ftp->list() );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );

    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() ) {
        QFAIL( msgTimedOut(host, port) );
    }

    ResMapIt it = resultMap.find( QFtp::Cd );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );
    QFETCH( QStringList, entryNames );
    QCOMPARE( listInfo_i.count(), entryNames.count() );
    for ( uint i=0; i < (uint) entryNames.count(); i++ ) {
        QCOMPARE( listInfo_i[i].name(), entryNames[i] );
    }
}

void tst_QFtp::get_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("success");
    QTest::addColumn<QByteArray>("res");
    QTest::addColumn<bool>("useIODevice");

    // ### move this into external testdata
    QFile file(rfc3252File);
    QVERIFY2( file.open( QIODevice::ReadOnly ), qPrintable(file.errorString()) );
    QByteArray rfc3252 = file.readAll();

    // test the two get() overloads in one routine
    for ( int i=0; i<2; i++ ) {
        const QByteArray iB = QByteArray::number(i);
        QTest::newRow(("relPath01_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
                << "qtest/rfc3252" << 1 << rfc3252 << (bool)(i==1);
        QTest::newRow(("relPath02_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
                << "qtest/rfc3252" << 1 << rfc3252 << (bool)(i==1);

        QTest::newRow(("absPath01_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
                << "/qtest/rfc3252" << 1 << rfc3252 << (bool)(i==1);
        QTest::newRow(("absPath02_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
                << "/var/ftp/qtest/rfc3252" << 1 << rfc3252 << (bool)(i==1);

        QTest::newRow(("nonExist01_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
                << QString("foo")  << 0 << QByteArray() << (bool)(i==1);
        QTest::newRow(("nonExist02_" + iB).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
                << QString("/foo") << 0 << QByteArray() << (bool)(i==1);
    }
}

void tst_QFtp::get()
{
    // for the overload that takes a QIODevice
    QByteArray buf_ba;
    QBuffer buf( &buf_ba );

    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, file );
    QFETCH( bool, useIODevice );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    if ( useIODevice ) {
        buf.open( QIODevice::WriteOnly );
        addCommand( QFtp::Get, ftp->get( file, &buf ) );
    } else {
        addCommand( QFtp::Get, ftp->get( file ) );
    }
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 50 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    ResMapIt it = resultMap.find( QFtp::Get );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );
    if ( useIODevice ) {
        QTEST( buf_ba, "res" );
    } else {
        QTEST( newData_ba, "res" );
    }
    QVERIFY2( bytesTotal != bytesTotal_init, msgComparison(bytesTotal, "!=", bytesTotal_init) );
    if ( bytesTotal != -1 ) {
        QCOMPARE( bytesDone, bytesTotal );
    }
    if ( useIODevice ) {
        if ( bytesDone != bytesDone_init ) {
            QCOMPARE( qlonglong(buf_ba.size()), bytesDone );
        }
    } else {
        if ( bytesDone != bytesDone_init ) {
            QCOMPARE( qlonglong(newData_ba.size()), bytesDone );
        }
    }
}

void tst_QFtp::put_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QByteArray>("fileData");
    QTest::addColumn<bool>("useIODevice");
    QTest::addColumn<int>("success");

    // ### move this into external testdata
    QFile file(rfc3252File);
    QVERIFY2( file.open( QIODevice::ReadOnly ), qPrintable(file.errorString()) );
    QByteArray rfc3252 = file.readAll();

    QByteArray bigData( 10*1024*1024, 0 );
    bigData.fill( 'A' );

    // test the two put() overloads in one routine with a file name containing
    // U+0x00FC (latin small letter u with diaeresis) for QTBUG-52303, testing UTF-8
    for ( int i=0; i<2; i++ ) {
        QTest::newRow(("relPath01_" + QByteArray::number(i)).constData()) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
                << (QLatin1String("qtest/upload/rel01_") + QChar(0xfc) + QLatin1String("%1")) << rfc3252
                << (bool)(i==1) << 1;
        /*
    QTest::newRow( QString("relPath02_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
        << QString("qtest/upload/rel02_%1") << rfc3252
        << (bool)(i==1) << 1;
    QTest::newRow( QString("relPath03_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
        << QString("qtest/upload/rel03_%1") << QByteArray()
        << (bool)(i==1) << 1;
    QTest::newRow( QString("relPath04_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
        << QString("qtest/upload/rel04_%1") << bigData
        << (bool)(i==1) << 1;

    QTest::newRow( QString("absPath01_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
        << QString("/qtest/upload/abs01_%1") << rfc3252
        << (bool)(i==1) << 1;
    QTest::newRow( QString("absPath02_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
        << QString("/srv/ftp/qtest/upload/abs02_%1") << rfc3252
        << (bool)(i==1) << 1;

    QTest::newRow( QString("nonExist01_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
        << QString("foo")  << QByteArray()
        << (bool)(i==1) << 0;
    QTest::newRow( QString("nonExist02_%1").arg(i).toLatin1().constData() ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
        << QString("/foo") << QByteArray()
        << (bool)(i==1) << 0;
*/
    }
}

void tst_QFtp::put()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, file );
    QFETCH( QByteArray, fileData );
    QFETCH( bool, useIODevice );

#if defined(Q_OS_WIN) && !defined(QT_NO_NETWORKPROXY)
    QFETCH_GLOBAL(bool, setProxy);
    if (setProxy) {
        QFETCH_GLOBAL(int, proxyType);
        if (proxyType == QNetworkProxy::Socks5Proxy)
            QSKIP("With socks5 the put() test takes too long time on Windows.");
    }
#endif // OS_WIN && !QT_NO_NETWORKPROXY

    const int timestep = 50;

    if(file.contains('%'))
        file = file.arg(uniqueExtension);

    // for the overload that takes a QIODevice
    QBuffer buf_fileData( &fileData );
    buf_fileData.open( QIODevice::ReadOnly );

    ResMapIt it;
    //////////////////////////////////////////////////////////////////
    // upload the file
    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    if ( useIODevice )
        addCommand( QFtp::Put, ftp->put( &buf_fileData, file ) );
    else
        addCommand( QFtp::Put, ftp->put( fileData, file ) );
    addCommand( QFtp::Close, ftp->close() );

    for(int time = 0; time <= fileData.length() / 20000; time += timestep) {
        QTestEventLoop::instance().enterLoop( timestep );
        if(ftp->currentCommand() == QFtp::None)
            break;
    }
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    it = resultMap.find( QFtp::Put );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );
    if ( !it.value().success ) {
        QVERIFY( !fileExists( host, port, user, password, file ) );
        return; // the following tests are only meaningful if the file could be put
    }
    QCOMPARE( bytesTotal, qlonglong(fileData.size()) );
    QCOMPARE( bytesDone, bytesTotal );

    QVERIFY( fileExists( host, port, user, password, file ) );

    //////////////////////////////////////////////////////////////////
    // fetch file to make sure that it is equal to the uploaded file
    init();
    ftp = newFtp();
    QBuffer buf;
    buf.open( QIODevice::WriteOnly );
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Get, ftp->get( file, &buf ) );
    addCommand( QFtp::Close, ftp->close() );

    for(int time = 0; time <= fileData.length() / 20000; time += timestep) {
        QTestEventLoop::instance().enterLoop( timestep );
        if(ftp->currentCommand() == QFtp::None)
            break;
    }
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    QCOMPARE( done_success, 1 );
    QTEST( buf.buffer(), "fileData" );

    //////////////////////////////////////////////////////////////////
    // cleanup (i.e. remove the file) -- this also tests the remove command
    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Remove, ftp->remove( file ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( timestep );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    it = resultMap.find( QFtp::Remove );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 1 );

    QVERIFY( !fileExists( host, port, user, password, file ) );
}

void tst_QFtp::mkdir_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("cdDir");
    QTest::addColumn<QString>("dirToCreate");
    QTest::addColumn<int>("success");

    QTest::newRow( "relPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
            << "qtest/upload" << QString("rel01_%1") << 1;
    QTest::newRow( "relPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
            << "qtest/upload" << QString("rel02_%1") << 1;
    QTest::newRow( "relPath03" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
            << "qtest/upload" << QString("rel03_%1") << 1;

    QTest::newRow( "absPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
            << "." << QString("/qtest/upload/abs01_%1") << 1;
    QTest::newRow( "absPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")
            << "." << QString("/var/ftp/qtest/upload/abs02_%1") << 1;

    //    QTest::newRow( "nonExist01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("foo")  << 0;
    QTest::newRow( "nonExist01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
            << "." << QString("foo")  << 0;
    QTest::newRow( "nonExist02" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString()
            << "." << QString("/foo") << 0;
}

void tst_QFtp::mkdir()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, cdDir );
    QFETCH( QString, dirToCreate );

    if(dirToCreate.contains('%'))
        dirToCreate = dirToCreate.arg(uniqueExtension);

    //////////////////////////////////////////////////////////////////
    // create the directory
    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Cd, ftp->cd( cdDir ) );
    addCommand( QFtp::Mkdir, ftp->mkdir( dirToCreate ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    ResMapIt it = resultMap.find( QFtp::Mkdir );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );
    if ( !it.value().success ) {
        QVERIFY( !dirExists( host, port, user, password, cdDir, dirToCreate ) );
        return; // the following tests are only meaningful if the dir could be created
    }
    QVERIFY( dirExists( host, port, user, password, cdDir, dirToCreate ) );

    //////////////////////////////////////////////////////////////////
    // create the directory again (should always fail!)
    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Cd, ftp->cd( cdDir ) );
    addCommand( QFtp::Mkdir, ftp->mkdir( dirToCreate ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    it = resultMap.find( QFtp::Mkdir );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 0 );

    //////////////////////////////////////////////////////////////////
    // remove the directory
    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Cd, ftp->cd( cdDir ) );
    addCommand( QFtp::Rmdir, ftp->rmdir( dirToCreate ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    it = resultMap.find( QFtp::Rmdir );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 1 );

    QVERIFY( !dirExists( host, port, user, password, cdDir, dirToCreate ) );
}

void tst_QFtp::mkdir2()
{
    ftp = new QFtp;
    ftp->connectToHost(QtNetworkSettings::serverName());
    ftp->login();
    current_id = ftp->cd("kake/test");

    QEventLoop loop;
    connect(ftp, SIGNAL(done(bool)), &loop, SLOT(quit()));
    connect(ftp, SIGNAL(commandFinished(int,bool)), this, SLOT(mkdir2Slot(int,bool)));
    QTimer::singleShot(5000, &loop, SLOT(quit()));

    QSignalSpy commandStartedSpy(ftp, SIGNAL(commandStarted(int)));
    QSignalSpy commandFinishedSpy(ftp, SIGNAL(commandFinished(int,bool)));

    loop.exec();

    QCOMPARE(commandStartedSpy.count(), 4); // connect, login, cd, mkdir
    QCOMPARE(commandFinishedSpy.count(), 4);

    for (int i = 0; i < 4; ++i)
        QCOMPARE(commandFinishedSpy.at(i).at(0), commandStartedSpy.at(i).at(0));

    QVERIFY(!commandFinishedSpy.at(0).at(1).toBool());
    QVERIFY(!commandFinishedSpy.at(1).at(1).toBool());
    QVERIFY(commandFinishedSpy.at(2).at(1).toBool());
    QVERIFY(commandFinishedSpy.at(3).at(1).toBool());

    delete ftp;
    ftp = 0;
}

void tst_QFtp::mkdir2Slot(int id, bool)
{
    if (id == current_id)
        ftp->mkdir("kake/test");
}

void tst_QFtp::rename_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("cdDir");
    QTest::addColumn<QString>("oldfile");
    QTest::addColumn<QString>("newfile");
    QTest::addColumn<QString>("createFile");
    QTest::addColumn<QString>("renamedFile");
    QTest::addColumn<int>("success");

    QTest::newRow("relPath01") << QtNetworkSettings::serverName() << QString() << QString()
            << "qtest/upload"
            << QString("rel_old01_%1") << QString("rel_new01_%1")
            << QString("qtest/upload/rel_old01_%1") << QString("qtest/upload/rel_new01_%1")
            << 1;
    QTest::newRow("relPath02") << QtNetworkSettings::serverName() << QString("ftptest")     << "password"
            << "qtest/upload"
            << QString("rel_old02_%1") << QString("rel_new02_%1")
            << QString("qtest/upload/rel_old02_%1") << QString("qtest/upload/rel_new02_%1")
            << 1;
    QTest::newRow("relPath03") << QtNetworkSettings::serverName() << QString("ftptest")     << "password"
            << "qtest/upload"
            << QString("rel_old03_%1")<< QString("rel_new03_%1")
            << QString("qtest/upload/rel_old03_%1") << QString("qtest/upload/rel_new03_%1")
            << 1;

    QTest::newRow("absPath01") << QtNetworkSettings::serverName() << QString() << QString()
            << QString()
            << QString("/qtest/upload/abs_old01_%1") << QString("/qtest/upload/abs_new01_%1")
            << QString("/qtest/upload/abs_old01_%1") << QString("/qtest/upload/abs_new01_%1")
            << 1;
    QTest::newRow("absPath02") << QtNetworkSettings::serverName() << QString("ftptest")     << "password"
            << QString()
            << QString("/var/ftp/qtest/upload/abs_old02_%1") << QString("/var/ftp/qtest/upload/abs_new02_%1")
            << QString("/var/ftp/qtest/upload/abs_old02_%1") << QString("/var/ftp/qtest/upload/abs_new02_%1")
            << 1;

    QTest::newRow("nonExist01") << QtNetworkSettings::serverName() << QString() << QString()
            << QString()
            << QString("foo") << "new_foo"
            << QString() << QString()
            << 0;
    QTest::newRow("nonExist02") << QtNetworkSettings::serverName() << QString() << QString()
            << QString()
            << QString("/foo") << QString("/new_foo")
            << QString() << QString()
            << 0;
}

void tst_QFtp::renameInit( const QString &host, const QString &user, const QString &password, const QString &createFile )
{
    if ( !createFile.isNull() ) {
        // upload the file
        init();
        ftp = newFtp();
        addCommand( QFtp::ConnectToHost, ftp->connectToHost( host ) );
        addCommand( QFtp::Login, ftp->login( user, password ) );
        addCommand( QFtp::Put, ftp->put( QByteArray(), createFile ) );
        addCommand( QFtp::Close, ftp->close() );

        QTestEventLoop::instance().enterLoop( 50 );
        delete ftp;
        ftp = 0;
        if ( QTestEventLoop::instance().timeout() )
            QFAIL( msgTimedOut(host) );

        ResMapIt it = resultMap.find( QFtp::Put );
        QVERIFY( it != resultMap.end() );
        QCOMPARE( it.value().success, 1 );

        QVERIFY( fileExists( host, 21, user, password, createFile ) );
    }
}

void tst_QFtp::renameCleanup( const QString &host, const QString &user, const QString &password, const QString &fileToDelete )
{
    if ( !fileToDelete.isNull() ) {
        // cleanup (i.e. remove the file)
        init();
        ftp = newFtp();
        addCommand( QFtp::ConnectToHost, ftp->connectToHost( host ) );
        addCommand( QFtp::Login, ftp->login( user, password ) );
        addCommand( QFtp::Remove, ftp->remove( fileToDelete ) );
        addCommand( QFtp::Close, ftp->close() );

        QTestEventLoop::instance().enterLoop( 30 );
        delete ftp;
        ftp = 0;
        if ( QTestEventLoop::instance().timeout() )
            QFAIL( msgTimedOut(host) );

        ResMapIt it = resultMap.find( QFtp::Remove );
        QVERIFY( it != resultMap.end() );
        QCOMPARE( it.value().success, 1 );

        QVERIFY( !fileExists( host, 21, user, password, fileToDelete ) );
    }
}

void tst_QFtp::rename()
{
    QFETCH( QString, host );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, cdDir );
    QFETCH( QString, oldfile );
    QFETCH( QString, newfile );
    QFETCH( QString, createFile );
    QFETCH( QString, renamedFile );

    if(oldfile.contains('%'))
        oldfile = oldfile.arg(uniqueExtension);
    if(newfile.contains('%'))
        newfile = newfile.arg(uniqueExtension);
    if(createFile.contains('%'))
        createFile = createFile.arg(uniqueExtension);
    if(renamedFile.contains('%'))
        renamedFile = renamedFile.arg(uniqueExtension);

    renameInit( host, user, password, createFile );

    init();
    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    if ( !cdDir.isNull() )
        addCommand( QFtp::Cd, ftp->cd( cdDir ) );
    addCommand( QFtp::Rename, ftp->rename( oldfile, newfile ) );
    addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host) );

    ResMapIt it = resultMap.find( QFtp::Rename );
    QVERIFY( it != resultMap.end() );
    QTEST( it.value().success, "success" );

    if ( it.value().success ) {
        QVERIFY( !fileExists( host, 21, user, password, oldfile, cdDir ) );
        QVERIFY( fileExists( host, 21, user, password, newfile, cdDir ) );
        QVERIFY( fileExists( host, 21, user, password, renamedFile ) );
    } else {
        QVERIFY( !fileExists( host, 21, user, password, newfile, cdDir ) );
        QVERIFY( !fileExists( host, 21, user, password, renamedFile ) );
    }

    renameCleanup( host, user, password, renamedFile );
}

/*
  The commandSequence() test does not test any particular function. It rather
  tests a sequence of arbitrary commands specified in the test data.
*/
class FtpCommand
{
public:
    FtpCommand() :
            cmd(QFtp::None)
    { }

    FtpCommand( QFtp::Command command ) :
            cmd(command)
    { }

    FtpCommand( QFtp::Command command, const QStringList &arguments ) :
            cmd(command), args(arguments)
    { }

    FtpCommand( const FtpCommand &c )
    { *this = c; }

    FtpCommand &operator=( const FtpCommand &c )
                         {
        this->cmd  = c.cmd;
        this->args = c.args;
        return *this;
    }

    QFtp::Command cmd;
    QStringList args;
};
QDataStream &operator<<( QDataStream &s, const FtpCommand &command )
{
    s << (int)command.cmd;
    s << command.args;
    return s;
}
QDataStream &operator>>( QDataStream &s, FtpCommand &command )
{
    int tmp;
    s >> tmp;
    command.cmd = (QFtp::Command)tmp;
    s >> command.args;
    return s;
}
Q_DECLARE_METATYPE(QList<FtpCommand>)

void tst_QFtp::commandSequence_data()
{
    // some "constants"
    QStringList argConnectToHost01;
    argConnectToHost01 << QtNetworkSettings::serverName() << "21";

    QStringList argLogin01, argLogin02, argLogin03, argLogin04;
    argLogin01 << QString() << QString();
    argLogin02 << "ftp"         << QString();
    argLogin03 << "ftp"         << "foo";
    argLogin04 << QString("ftptest")     << "password";

    FtpCommand connectToHost01( QFtp::ConnectToHost, argConnectToHost01 );
    FtpCommand login01( QFtp::Login, argLogin01 );
    FtpCommand login02( QFtp::Login, argLogin01 );
    FtpCommand login03( QFtp::Login, argLogin01 );
    FtpCommand login04( QFtp::Login, argLogin01 );
    FtpCommand close01( QFtp::Close );

    QTest::addColumn<QList<FtpCommand> >("cmds");
    QTest::addColumn<int>("success");

    // success data
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        QTest::newRow( "simple_ok01" ) << cmds << 1;
    }
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        cmds << login01;
        QTest::newRow( "simple_ok02" ) << cmds << 1;
    }
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        cmds << login01;
        cmds << close01;
        QTest::newRow( "simple_ok03" ) << cmds << 1;
    }
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        cmds << close01;
        QTest::newRow( "simple_ok04" ) << cmds << 1;
    }
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        cmds << login01;
        cmds << close01;
        cmds << connectToHost01;
        cmds << login02;
        cmds << close01;
        QTest::newRow( "connect_twice" ) << cmds << 1;
    }

    // error data
    {
        QList<FtpCommand> cmds;
        cmds << close01;
        QTest::newRow( "error01" ) << cmds << 0;
    }
    {
        QList<FtpCommand> cmds;
        cmds << login01;
        QTest::newRow( "error02" ) << cmds << 0;
    }
    {
        QList<FtpCommand> cmds;
        cmds << login01;
        cmds << close01;
        QTest::newRow( "error03" ) << cmds << 0;
    }
    {
        QList<FtpCommand> cmds;
        cmds << connectToHost01;
        cmds << login01;
        cmds << close01;
        cmds << login01;
        QTest::newRow( "error04" ) << cmds << 0;
    }
}

void tst_QFtp::commandSequence()
{
    QFETCH( QList<FtpCommand>, cmds );

    ftp = newFtp();
    QString host;
    quint16 port = 0;
    QList<FtpCommand>::iterator it;
    for ( it = cmds.begin(); it != cmds.end(); ++it ) {
        switch ( (*it).cmd ) {
        case QFtp::ConnectToHost:
            {
                QCOMPARE( (*it).args.count(), 2 );
                bool portOk;
                port = (*it).args[1].toUShort( &portOk );
                QVERIFY( portOk );
                host = (*it).args[0];
                ids << ftp->connectToHost( host, port );
            }
            break;
        case QFtp::Login:
            QCOMPARE( (*it).args.count(), 2 );
            ids << ftp->login( (*it).args[0], (*it).args[1] );
            break;
        case QFtp::Close:
            QCOMPARE( (*it).args.count(), 0 );
            ids << ftp->close();
            break;
        default:
            QFAIL( "Error in test: unexpected enum value" );
            break;
        }
    }

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host) );

    QTEST( commandSequence_success, "success" );
}

void tst_QFtp::abort_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QByteArray>("uploadData");

    QTest::newRow( "get_fluke01" ) << QtNetworkSettings::serverName() << (uint)21 << QString("qtest/bigfile") << QByteArray();
    QTest::newRow( "get_fluke02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("qtest/rfc3252") << QByteArray();

    // Qt/CE test environment has too little memory for this test
    QByteArray bigData( 10*1024*1024, 0 );
    bigData.fill( 'B' );
    QTest::newRow( "put_fluke01" ) << QtNetworkSettings::serverName() << (uint)21 << QString("qtest/upload/abort_put") << bigData;
}

void tst_QFtp::abort()
{
    QSKIP("This test takes too long.");
    // In case you wonder where the abort() actually happens, look into
    // tst_QFtp::dataTransferProgress
    //
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, file );
    QFETCH( QByteArray, uploadData );

    QFtp::Command cmd;
    if ( uploadData.size() == 0 )
        cmd = QFtp::Get;
    else
        cmd = QFtp::Put;

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login() );
    if ( cmd == QFtp::Get )
        addCommand( cmd, ftp->get( file ) );
    else
        addCommand( cmd, ftp->put( uploadData, file ) );
    addCommand( QFtp::Close, ftp->close() );

    for(int time = 0; time <= uploadData.length() / 30000; time += 30) {
        QTestEventLoop::instance().enterLoop( 50 );
        if(ftp->currentCommand() == QFtp::None)
            break;
    }
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host, port) );

    ResMapIt it = resultMap.find( cmd );
    QVERIFY( it != resultMap.end() );
    // ### how to test the abort?
    if ( it.value().success ) {
        // The FTP server on fluke is sadly returning a success, even when
        // the operation was aborted. So we have to use some heuristics.
        if ( host == QtNetworkSettings::serverName() ) {
            if ( cmd == QFtp::Get ) {
                QVERIFY2(bytesDone <= bytesTotal, msgComparison(bytesDone, "<=", bytesTotal));
            } else {
                // put commands should always be aborted, since we use really
                // big data
                QVERIFY2( bytesDone != bytesTotal, msgComparison(bytesDone, "!=", bytesTotal) );
            }
        } else {
            // this could be tested by verifying that no more progress signals are emitted
            QVERIFY2(bytesDone <= bytesTotal, msgComparison(bytesDone, "<=", bytesTotal));
        }
    } else {
        QVERIFY2( bytesDone != bytesTotal, msgComparison(bytesDone, "!=", bytesTotal) );
    }

    if ( cmd == QFtp::Put ) {
        //////////////////////////////////////
        // cleanup (i.e. remove the file)
        init();
        ftp = newFtp();
        addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
        addCommand( QFtp::Login, ftp->login() );
        addCommand( QFtp::Remove, ftp->remove( file ) );
        addCommand( QFtp::Close, ftp->close() );

        QTestEventLoop::instance().enterLoop( 30 );
        delete ftp;
        ftp = 0;
        if ( QTestEventLoop::instance().timeout() )
            QFAIL( msgTimedOut(host, port) );

        it = resultMap.find( QFtp::Remove );
        QVERIFY( it != resultMap.end() );
        QCOMPARE( it.value().success, 1 );
    }
}

void tst_QFtp::bytesAvailable_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("type");
    QTest::addColumn<qlonglong>("bytesAvailFinishedGet");
    QTest::addColumn<qlonglong>("bytesAvailFinished");
    QTest::addColumn<qlonglong>("bytesAvailDone");

    QTest::newRow( "fluke01" ) << QtNetworkSettings::serverName() << QString("qtest/bigfile") << 0 << (qlonglong)519240 << (qlonglong)519240 << (qlonglong)519240;
    QTest::newRow( "fluke02" ) << QtNetworkSettings::serverName() << QString("qtest/rfc3252") << 0 << (qlonglong)25962 << (qlonglong)25962 << (qlonglong)25962;

    QTest::newRow( "fluke03" ) << QtNetworkSettings::serverName() << QString("qtest/bigfile") << 1 << (qlonglong)519240 << (qlonglong)0 << (qlonglong)0;
    QTest::newRow( "fluke04" ) << QtNetworkSettings::serverName() << QString("qtest/rfc3252") << 1 << (qlonglong)25962 << (qlonglong)0 << (qlonglong)0;
}

void tst_QFtp::bytesAvailable()
{
    QFETCH( QString, host );
    QFETCH( QString, file );
    QFETCH( int, type );

    ftp = newFtp();
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host ) );
    addCommand( QFtp::Login, ftp->login() );
    addCommand( QFtp::Get, ftp->get( file ) );
    if ( type != 0 )
        addCommand( QFtp::Close, ftp->close() );

    QTestEventLoop::instance().enterLoop( 40 );
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(host) );

    ResMapIt it = resultMap.find( QFtp::Get );
    QVERIFY( it != resultMap.end() );
    QVERIFY( it.value().success );

    QFETCH(qlonglong, bytesAvailFinishedGet);
    QCOMPARE(bytesAvailable_finishedGet, bytesAvailFinishedGet);

    QFETCH(qlonglong, bytesAvailFinished);
    QCOMPARE(bytesAvailable_finished, bytesAvailFinished);

    QFETCH(qlonglong, bytesAvailDone);
    QCOMPARE(bytesAvailable_done, bytesAvailDone);

    ftp->readAll();
    QCOMPARE( ftp->bytesAvailable(), 0 );
    delete ftp;
    ftp = 0;
}

void tst_QFtp::activeMode()
{
    QFile file("tst_QFtp_activeMode_inittab");
    file.open(QIODevice::ReadWrite);
    QFtp ftp;
    ftp.setTransferMode(QFtp::Active);
    ftp.connectToHost(QtNetworkSettings::serverName(), 21);
    ftp.login();
    ftp.list();
    ftp.get("/qtest/rfc3252.txt", &file);
    connect(&ftp, SIGNAL(done(bool)), SLOT(activeModeDone(bool)));
    QTestEventLoop::instance().enterLoop(900);
    QFile::remove("tst_QFtp_activeMode_inittab");
    QCOMPARE(done_success, 1);

}

void tst_QFtp::activeModeDone(bool error)
{
    done_success = error ? -1 : 1;
    QTestEventLoop::instance().exitLoop();
}

void tst_QFtp::proxy_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<uint>("port");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("dir");
    QTest::addColumn<int>("success");
    QTest::addColumn<QStringList>("entryNames"); // ### we should rather use a QList<QUrlInfo> here

    QStringList flukeRoot;
    flukeRoot << "qtest";
    QStringList flukeQtest;
    flukeQtest << "bigfile";
    flukeQtest << "nonASCII";
    flukeQtest << "rfc3252";
    flukeQtest << "rfc3252.txt";
    flukeQtest << "upload";

    QTest::newRow( "proxy_relPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("qtest") << 1 << flukeQtest;
    QTest::newRow( "proxy_relPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("qtest") << 1 << flukeQtest;

    QTest::newRow( "proxy_absPath01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/qtest") << 1 << flukeQtest;
    QTest::newRow( "proxy_absPath02" ) << QtNetworkSettings::serverName() << (uint)21 << QString("ftptest")     << QString("password")     << QString("/var/ftp/qtest") << 1 << flukeQtest;

    QTest::newRow( "proxy_nonExist01" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("foo")  << 0 << QStringList();
    QTest::newRow( "proxy_nonExist03" ) << QtNetworkSettings::serverName() << (uint)21 << QString() << QString() << QString("/foo") << 0 << QStringList();
}

void tst_QFtp::proxy()
{
    QFETCH( QString, host );
    QFETCH( uint, port );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, dir );

    ftp = newFtp();
    addCommand( QFtp::SetProxy, ftp->setProxy( QtNetworkSettings::serverName(), 2121 ) );
    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    addCommand( QFtp::Cd, ftp->cd( dir ) );
    addCommand( QFtp::List, ftp->list() );

    QTestEventLoop::instance().enterLoop( 50 );

    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() ) {
        QFAIL( msgTimedOut(host, port) );
    }

    ResMapIt it = resultMap.find( QFtp::Cd );
    QVERIFY( it != resultMap.end() );
    QFETCH( int, success );
    QCOMPARE( it.value().success, success );
    QFETCH( QStringList, entryNames );
    QCOMPARE( listInfo_i.count(), entryNames.count() );
    for ( uint i=0; i < (uint) entryNames.count(); i++ ) {
        QCOMPARE( listInfo_i[i].name(), entryNames[i] );
    }
}

void tst_QFtp::binaryAscii()
{
    QString file = "asciifile%1.txt";

    if(file.contains('%'))
        file = file.arg(uniqueExtension);

    QByteArray putData = "a line of text\r\n";

    init();
    ftp = newFtp();
    addCommand(QFtp::ConnectToHost, ftp->connectToHost(QtNetworkSettings::serverName(), 21));
    addCommand(QFtp::Login, ftp->login("ftptest", "password"));
    addCommand(QFtp::Cd, ftp->cd("qtest/upload"));
    addCommand(QFtp::Put, ftp->put(putData, file, QFtp::Ascii));
    addCommand(QFtp::Close, ftp->close());

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(QtNetworkSettings::serverName()) );

    ResMapIt it = resultMap.find(QFtp::Put);
    QVERIFY(it != resultMap.end());
    QVERIFY(it.value().success);

    QByteArray getData;
    QBuffer getBuf(&getData);
    getBuf.open(QBuffer::WriteOnly);

    init();
    ftp = newFtp();
    addCommand(QFtp::ConnectToHost, ftp->connectToHost(QtNetworkSettings::serverName(), 21));
    addCommand(QFtp::Login, ftp->login("ftptest", "password"));
    addCommand(QFtp::Cd, ftp->cd("qtest/upload"));
    addCommand(QFtp::Get, ftp->get(file, &getBuf, QFtp::Binary));
    addCommand(QFtp::Close, ftp->close());

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(QtNetworkSettings::serverName()) );

    ResMapIt it2 = resultMap.find(QFtp::Get);
    QVERIFY(it2 != resultMap.end());
    QVERIFY(it2.value().success);
    // most modern ftp servers leave the file as it is by default
    // (and do not remove the windows line ending), the -1 below could be
    // deleted in the future
    QCOMPARE(getData.size(), putData.size() - 1);
    //////////////////////////////////////////////////////////////////
    // cleanup (i.e. remove the file) -- this also tests the remove command
    init();
    ftp = newFtp();
    addCommand(QFtp::ConnectToHost, ftp->connectToHost(QtNetworkSettings::serverName(), 21));
    addCommand(QFtp::Login, ftp->login("ftptest", "password"));
    addCommand(QFtp::Cd, ftp->cd("qtest/upload"));
    addCommand(QFtp::Remove, ftp->remove(file));
    addCommand(QFtp::Close, ftp->close());

    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() )
        QFAIL( msgTimedOut(QtNetworkSettings::serverName()) );

    it = resultMap.find( QFtp::Remove );
    QVERIFY( it != resultMap.end() );
    QCOMPARE( it.value().success, 1 );

    QVERIFY(!fileExists(QtNetworkSettings::serverName(), 21, "ftptest", "password", file));
}


// test QFtp::currentId() and QFtp::currentCommand()
#define CURRENTCOMMAND_TEST \
{ \
  ResMapIt it; \
  for ( it = resultMap.begin(); it != resultMap.end(); ++it ) { \
                                                                if ( it.value().id == ftp->currentId() ) { \
                                                                                                           QVERIFY( it.key() == ftp->currentCommand() ); \
                                                                                                       } \
} \
}

void tst_QFtp::commandStarted( int id )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:commandStarted( %d )", ftp->currentId(), id );
#endif
    // make sure that the commandStarted and commandFinished are nested correctly
    QCOMPARE( current_id, 0 );
    current_id = id;

    QVERIFY( !ids.isEmpty() );
    QCOMPARE( ids.first(), id );
    if ( ids.count() > 1 ) {
        QVERIFY( ftp->hasPendingCommands() );
    } else {
        QVERIFY( !ftp->hasPendingCommands() );
    }

    QVERIFY( ftp->currentId() == id );
    QCOMPARE( cur_state, int(ftp->state()) );
    CURRENTCOMMAND_TEST;

    QCOMPARE( ftp->error(), QFtp::NoError );
}

void tst_QFtp::commandFinished( int id, bool error )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:commandFinished( %d, %d ) -- errorString: '%s'",
            ftp->currentId(), id, (int)error, ftp->errorString().toLatin1().constData() );
#endif
    if ( ftp->currentCommand() == QFtp::Get ) {
        bytesAvailable_finishedGet = ftp->bytesAvailable();
    }
    bytesAvailable_finished = ftp->bytesAvailable();

    // make sure that the commandStarted and commandFinished are nested correctly
    QCOMPARE( current_id, id );
    current_id = 0;

    QVERIFY( !ids.isEmpty() );
    QCOMPARE( ids.first(), id );
    if ( !error && ids.count() > 1) {
        QVERIFY( ftp->hasPendingCommands() );
    } else {
        QVERIFY( !ftp->hasPendingCommands() );
    }
    if ( error ) {
        QVERIFY2( ftp->error() != QFtp::NoError, msgComparison(ftp->error(), "!=", QFtp::NoError) );
        ids.clear();
    } else {
        QCOMPARE( ftp->error(), QFtp::NoError );
        ids.pop_front();
    }

    QCOMPARE( ftp->currentId(), id );
    QCOMPARE( cur_state, int(ftp->state()) );
    CURRENTCOMMAND_TEST;

    if ( QTest::currentTestFunction() != QLatin1String("commandSequence") ) {
        ResMapIt it = resultMap.find( ftp->currentCommand() );
        QVERIFY( it != resultMap.end() );
        QCOMPARE( it.value().success, -1 );
        if ( error )
            it.value().success = 0;
        else
            it.value().success = 1;
    }
}

void tst_QFtp::done( bool error )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:done( %d )", ftp->currentId(), (int)error );
#endif
    bytesAvailable_done = ftp->bytesAvailable();

    QCOMPARE( ftp->currentId(), 0 );
    QVERIFY( current_id == 0 );
    QVERIFY( ids.isEmpty() );
    QVERIFY( cur_state == ftp->state() );
    QVERIFY( !ftp->hasPendingCommands() );

    if ( QTest::currentTestFunction() == QLatin1String("commandSequence") ) {
        QCOMPARE( commandSequence_success, -1 );
        if ( error )
            commandSequence_success = 0;
        else
            commandSequence_success = 1;
    }
    QCOMPARE( done_success, -1 );
    if ( error ) {
        QVERIFY2( ftp->error() != QFtp::NoError, msgComparison(ftp->error(), "!=", QFtp::NoError) );
        done_success = 0;
    } else {
        QCOMPARE( ftp->error(), QFtp::NoError );
        done_success = 1;
    }
    QTestEventLoop::instance().exitLoop();
}

void tst_QFtp::stateChanged( int state )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  stateChanged( %d )", ftp->currentId(), state );
#endif
    QCOMPARE( ftp->currentId(), current_id );
    CURRENTCOMMAND_TEST;

    QVERIFY2( state != cur_state, msgComparison(state, "!=", cur_state) );
    QCOMPARE( state, (int)ftp->state() );
    if ( state != QFtp::Unconnected ) {
        // make sure that the states are always emitted in the right order (for
        // this, we assume an ordering on the enum values, which they have at
        // the moment)
        QVERIFY2( cur_state < state, msgComparison(cur_state, "<", state) );

        // make sure that state changes are only emitted in response to certain
        // commands
        switch ( state ) {
        case QFtp::HostLookup:
        case QFtp::Connecting:
        case QFtp::Connected:
            QCOMPARE( (int)ftp->currentCommand(), (int)QFtp::ConnectToHost );
            break;
        case QFtp::LoggedIn:
            QCOMPARE( (int)ftp->currentCommand(), (int)QFtp::Login );
            break;
        case QFtp::Closing:
            QCOMPARE( (int)ftp->currentCommand(), (int)QFtp::Close );
            break;
        default:
            QWARN( QString("Unexpected state '%1'").arg(state).toLatin1().constData() );
            break;
        }
    }
    cur_state = state;

    if ( QTest::currentTestFunction() == QLatin1String("connectToHost") ) {
        switch ( state ) {
        case QFtp::HostLookup:
        case QFtp::Connecting:
        case QFtp::LoggedIn:
        case QFtp::Closing:
            // ignore
            break;
        case QFtp::Connected:
        case QFtp::Unconnected:
            QVERIFY( connectToHost_state == -1 );
            connectToHost_state = state;
            break;
        default:
            QWARN( QString("Unknown state '%1'").arg(state).toLatin1().constData() );
            break;
        }
    } else if ( QTest::currentTestFunction() == QLatin1String("close") ) {
        ResMapIt it = resultMap.find( QFtp::Close );
        if ( it!=resultMap.end() && ftp->currentId()==it.value().id ) {
            if ( state == QFtp::Closing ) {
                QCOMPARE( close_state, -1 );
                close_state = state;
            } else if ( state == QFtp::Unconnected ) {
                QCOMPARE(close_state, int(QFtp::Closing) );
                close_state = state;
            }
        }
    } else if ( QTest::currentTestFunction() == QLatin1String("login") ) {
        ResMapIt it = resultMap.find( QFtp::Login );
        if ( it!=resultMap.end() && ftp->currentId()==it.value().id ) {
            if ( state == QFtp::LoggedIn ) {
                QCOMPARE( login_state, -1 );
                login_state = state;
            }
        }
    }
}

void tst_QFtp::listInfo( const QUrlInfo &i )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  listInfo( %s )", ftp->currentId(), i.name().toLatin1().constData() );
#endif
    QCOMPARE( ftp->currentId(), current_id );
    if ( ids.count() > 1 ) {
        QVERIFY( ftp->hasPendingCommands() );
    } else {
        QVERIFY( !ftp->hasPendingCommands() );
    }
    QCOMPARE( cur_state, int(ftp->state()) );
    CURRENTCOMMAND_TEST;

    if ( QTest::currentTestFunction()==QLatin1String("list") || QTest::currentTestFunction()==QLatin1String("cd") || QTest::currentTestFunction()==QLatin1String("proxy") || inFileDirExistsFunction ) {
        ResMapIt it = resultMap.find( QFtp::List );
        QVERIFY( it != resultMap.end() );
        QCOMPARE( ftp->currentId(), it.value().id );
        listInfo_i << i;
    }
}

void tst_QFtp::readyRead()
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  readyRead(), bytesAvailable == %lu", ftp->currentId(), ftp->bytesAvailable() );
#endif
    QCOMPARE( ftp->currentId(), current_id );
    if ( ids.count() > 1 ) {
        QVERIFY( ftp->hasPendingCommands() );
    } else {
        QVERIFY( !ftp->hasPendingCommands() );
    }
    QVERIFY( cur_state == ftp->state() );
    CURRENTCOMMAND_TEST;

    if ( QTest::currentTestFunction() != QLatin1String("bytesAvailable") ) {
        int oldSize = newData_ba.size();
        qlonglong bytesAvail = ftp->bytesAvailable();
        QByteArray ba = ftp->readAll();
        QCOMPARE( ba.size(), (int) bytesAvail );
        newData_ba.resize( oldSize + ba.size() );
        memcpy( newData_ba.data()+oldSize, ba.data(), ba.size() );

        if ( bytesTotal != -1 ) {
            QVERIFY2( (int)newData_ba.size() <= bytesTotal, msgComparison(newData_ba.size(), "<=", bytesTotal) );
        }
        QCOMPARE( qlonglong(newData_ba.size()), bytesDone );
    }
}

void tst_QFtp::dataTransferProgress( qint64 done, qint64 total )
{
#if defined( DUMP_SIGNALS )
    qDebug( "%d:  dataTransferProgress( %lli, %lli )", ftp->currentId(), done, total );
#endif
    QCOMPARE( ftp->currentId(), current_id );
    if ( ids.count() > 1 ) {
        QVERIFY( ftp->hasPendingCommands() );
    } else {
        QVERIFY( !ftp->hasPendingCommands() );
    }
    QCOMPARE( cur_state, int(ftp->state()) );
    CURRENTCOMMAND_TEST;

    if ( bytesTotal == bytesTotal_init ) {
        bytesTotal = total;
    } else {
        QCOMPARE( bytesTotal, total );
    }

    QVERIFY2( bytesTotal != bytesTotal_init, msgComparison(bytesTotal, "!=", bytesTotal_init) );
    QVERIFY2( bytesDone <= done, msgComparison(bytesDone, "<=", done) );
    bytesDone = done;
    if ( bytesTotal != -1 ) {
        QVERIFY2( bytesDone <= bytesTotal, msgComparison(bytesDone, "<=", bytesTotal) );
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
                ftp->abort();
            }
        }
    }
}


QFtp *tst_QFtp::newFtp()
{
    QFtp *nFtp = new QFtp( this );
#ifndef QT_NO_BEARERMANAGEMENT
    if (networkSessionExplicit) {
        nFtp->setProperty("_q_networksession", QVariant::fromValue(networkSessionExplicit));
    }
#endif
    connect( nFtp, SIGNAL(commandStarted(int)),
             SLOT(commandStarted(int)) );
    connect( nFtp, SIGNAL(commandFinished(int,bool)),
             SLOT(commandFinished(int,bool)) );
    connect( nFtp, SIGNAL(done(bool)),
             SLOT(done(bool)) );
    connect( nFtp, SIGNAL(stateChanged(int)),
             SLOT(stateChanged(int)) );
    connect( nFtp, SIGNAL(listInfo(QUrlInfo)),
             SLOT(listInfo(QUrlInfo)) );
    connect( nFtp, SIGNAL(readyRead()),
             SLOT(readyRead()) );
    connect( nFtp, SIGNAL(dataTransferProgress(qint64,qint64)),
             SLOT(dataTransferProgress(qint64,qint64)) );

    return nFtp;
}

void tst_QFtp::addCommand( QFtp::Command cmd, int id )
{
    ids << id;
    CommandResult res;
    res.id = id;
    res.success = -1;
    resultMap[ cmd ] = res;
}

bool tst_QFtp::fileExists( const QString &host, quint16 port, const QString &user, const QString &password, const QString &file, const QString &cdDir )
{
    init();
    ftp = newFtp();
    // ### make these tests work
    if (ftp->currentId() != 0) {
        qWarning("ftp->currentId() != 0");
        return false;
    }

    if (ftp->state() != QFtp::Unconnected) {
        qWarning("ftp->state() != QFtp::Unconnected");
        return false;
    }

    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    if ( !cdDir.isNull() )
        addCommand( QFtp::Cd, ftp->cd( cdDir ) );
    addCommand( QFtp::List, ftp->list( file ) );
    addCommand( QFtp::Close, ftp->close() );

    inFileDirExistsFunction = true;
    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() ) {
        // ### make this test work
        qWarning("tst_QFtp::fileExists: Network operation timed out");
        return false;
    }
    inFileDirExistsFunction = false;

    ResMapIt it = resultMap.find( QFtp::ConnectToHost );
    // ### make these tests work
    if (it == resultMap.end()) {
        qWarning("it != resultMap.end()");
        return false;
    }

    if (it.value().success == -1) {
        qWarning("it.value().success != -1");
        return false;
    }

    if ( it.value().success == 1 ) {
        for ( uint i=0; i < (uint) listInfo_i.count(); i++ ) {
            if ( QFileInfo(listInfo_i[i].name()).fileName() == QFileInfo(file).fileName() )
                return true;
        }
    }

    //this is not a good warning considering sometime this function is used to test that a file does not exist
    //qWarning("file doesn't exist");
    return false;
}

bool tst_QFtp::dirExists( const QString &host, quint16 port, const QString &user, const QString &password, const QString &cdDir, const QString &dirToCreate )
{
    init();
    ftp = newFtp();
    // ### make these tests work
    //    QCOMPARE( ftp->currentId(), 0 );
    //    QCOMPARE( (int)ftp->state(), (int)QFtp::Unconnected );

    addCommand( QFtp::ConnectToHost, ftp->connectToHost( host, port ) );
    addCommand( QFtp::Login, ftp->login( user, password ) );
    if ( dirToCreate.startsWith( QLatin1Char('/') ) )
        addCommand( QFtp::Cd, ftp->cd( dirToCreate ) );
    else
        addCommand( QFtp::Cd, ftp->cd( cdDir + QLatin1Char('/') + dirToCreate ) );
    addCommand( QFtp::Close, ftp->close() );

    inFileDirExistsFunction = true;
    QTestEventLoop::instance().enterLoop( 30 );
    delete ftp;
    ftp = 0;
    if ( QTestEventLoop::instance().timeout() ) {
        // ### make this test work
        // QFAIL( msgTimedOut(host, port) );
        qWarning("tst_QFtp::dirExists: Network operation timed out");
        return false;
    }
    inFileDirExistsFunction = false;

    ResMapIt it = resultMap.find( QFtp::Cd );
    // ### make these tests work
    //    QVERIFY( it != resultMap.end() );
    //    QVERIFY( it.value().success != -1 );
    return it.value().success == 1;
}

void tst_QFtp::doneSignal()
{
    QFtp ftp;
    QSignalSpy spy(&ftp, SIGNAL(done(bool)));

    ftp.connectToHost(QtNetworkSettings::serverName());
    ftp.login("anonymous");
    ftp.list();
    ftp.close();

    done_success = 0;
    connect(&ftp, SIGNAL(done(bool)), &(QTestEventLoop::instance()), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(61);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Network operation timed out");

    QTest::qWait(200);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toBool(), false);
}

void tst_QFtp::queueMoreCommandsInDoneSlot()
{
    QSKIP("Task 127050 && 113966");

    QFtp ftp;
    QSignalSpy doneSpy(&ftp, SIGNAL(done(bool)));
    QSignalSpy commandFinishedSpy(&ftp, SIGNAL(commandFinished(int,bool)));

    this->ftp = &ftp;
    connect(&ftp, SIGNAL(done(bool)), this, SLOT(cdUpSlot(bool)));

    ftp.connectToHost("ftp.qt-project.org");
    ftp.login();
    ftp.cd("qt");
    ftp.rmdir("qtest-removedir-noexist");

    while ( ftp.hasPendingCommands() || ftp.currentCommand() != QFtp::None ) {
        QCoreApplication::instance()->processEvents(QEventLoop::AllEvents
                                                    | QEventLoop::WaitForMoreEvents);
    }

    QCOMPARE(doneSpy.count(), 2);
    QCOMPARE(doneSpy.first().first().toBool(), true);
    QCOMPARE(doneSpy.last().first().toBool(), false);

    QCOMPARE(commandFinishedSpy.count(), 6);
    int firstId = commandFinishedSpy.at(0).at(0).toInt();
    QCOMPARE(commandFinishedSpy.at(0).at(1).toBool(), false);
    QCOMPARE(commandFinishedSpy.at(1).at(0).toInt(), firstId + 1);
    QCOMPARE(commandFinishedSpy.at(1).at(1).toBool(), false);
    QCOMPARE(commandFinishedSpy.at(2).at(0).toInt(), firstId + 2);
    QCOMPARE(commandFinishedSpy.at(2).at(1).toBool(), false);
    QCOMPARE(commandFinishedSpy.at(3).at(0).toInt(), firstId + 3);
    QCOMPARE(commandFinishedSpy.at(3).at(1).toBool(), true);
    QCOMPARE(commandFinishedSpy.at(4).at(0).toInt(), firstId + 4);
    QCOMPARE(commandFinishedSpy.at(4).at(1).toBool(), false);
    QCOMPARE(commandFinishedSpy.at(5).at(0).toInt(), firstId + 5);
    QCOMPARE(commandFinishedSpy.at(5).at(1).toBool(), false);

    this->ftp = 0;
}

void tst_QFtp::cdUpSlot(bool error)
{
    if (error) {
        ftp->cd("..");
        ftp->cd("qt");
    }
}

void tst_QFtp::qtbug7359Crash()
{
    QFtp ftp;
    ftp.connectToHost("127.0.0.1");

    QTime t;
    int elapsed;

    t.start();
    while ((elapsed = t.elapsed()) < 200)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200 - elapsed);

    ftp.close();
    t.restart();
    while ((elapsed = t.elapsed()) < 1000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1000 - elapsed);

    ftp.connectToHost("127.0.0.1");

    t.restart();
    while ((elapsed = t.elapsed()) < 2000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 2000 - elapsed);
}

QTEST_MAIN(tst_QFtp)

#include "tst_qftp.moc"
