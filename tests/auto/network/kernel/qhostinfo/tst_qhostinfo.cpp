/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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


// When using WinSock2 on Windows, it's the first thing that can be included
// (except qglobal.h), or else you'll get tons of compile errors
#include <qglobal.h>

// To prevent windows system header files from re-defining min/max
#define NOMINMAX 1

#if defined(Q_OS_WIN)
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include <QtTest/QtTest>
#include <qcoreapplication.h>
#include <QDebug>
#include <QTcpSocket>
#include <private/qthread_p.h>
#include <QTcpServer>

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfigmanager.h>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworksession.h>
#endif

#include <time.h>
#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

#include <qhostinfo.h>
#include "private/qhostinfo_p.h"

#include <sys/types.h>
#if defined(Q_OS_UNIX)
#  include <sys/socket.h>
#  include <netdb.h>
#endif

#include "../../../network-settings.h"

#define TEST_DOMAIN ".test.qt-project.org"


class tst_QHostInfo : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void initTestCase();
    void swapFunction();
    void moveOperator();
    void getSetCheck();
    void staticInformation();
    void lookupIPv4_data();
    void lookupIPv4();
    void lookupIPv6_data();
    void lookupIPv6();
    void lookupConnectToFunctionPointer_data();
    void lookupConnectToFunctionPointer();
    void lookupConnectToFunctionPointerDeleted();
    void lookupConnectToLambda_data();
    void lookupConnectToLambda();
    void reverseLookup_data();
    void reverseLookup();

    void blockingLookup_data();
    void blockingLookup();

    void raceCondition();
    void threadSafety();
    void threadSafetyAsynchronousAPI();

    void multipleSameLookups();
    void multipleDifferentLookups_data();
    void multipleDifferentLookups();

    void cache();

    void abortHostLookup();
protected slots:
    void resultsReady(const QHostInfo &);

private:
    bool ipv6LookupsAvailable;
    bool ipv6Available;
    bool lookupDone;
    int lookupsDoneCounter;
    QHostInfo lookupResults;
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager *netConfMan;
    QNetworkConfiguration networkConfiguration;
    QScopedPointer<QNetworkSession> networkSession;
#endif
};

void tst_QHostInfo::swapFunction()
{
    QHostInfo obj1, obj2;
    obj1.setError(QHostInfo::HostInfoError(0));
    obj2.setError(QHostInfo::HostInfoError(1));
    obj1.swap(obj2);
    QCOMPARE(QHostInfo::HostInfoError(0), obj2.error());
    QCOMPARE(QHostInfo::HostInfoError(1), obj1.error());
}

void tst_QHostInfo::moveOperator()
{
    QHostInfo obj1, obj2, obj3(1);
    obj1.setError(QHostInfo::HostInfoError(0));
    obj2.setError(QHostInfo::HostInfoError(1));
    obj1 = std::move(obj2);
    obj2 = obj3;
    QCOMPARE(QHostInfo::HostInfoError(1), obj1.error());
    QCOMPARE(obj3.lookupId(), obj2.lookupId());
}



// Testing get/set functions
void tst_QHostInfo::getSetCheck()
{
    QHostInfo obj1;
    // HostInfoError QHostInfo::error()
    // void QHostInfo::setError(HostInfoError)
    obj1.setError(QHostInfo::HostInfoError(0));
    QCOMPARE(QHostInfo::HostInfoError(0), obj1.error());
    obj1.setError(QHostInfo::HostInfoError(1));
    QCOMPARE(QHostInfo::HostInfoError(1), obj1.error());

    // int QHostInfo::lookupId()
    // void QHostInfo::setLookupId(int)
    obj1.setLookupId(0);
    QCOMPARE(0, obj1.lookupId());
    obj1.setLookupId(INT_MIN);
    QCOMPARE(INT_MIN, obj1.lookupId());
    obj1.setLookupId(INT_MAX);
    QCOMPARE(INT_MAX, obj1.lookupId());
}

void tst_QHostInfo::staticInformation()
{
    qDebug() << "Hostname:" << QHostInfo::localHostName();
    qDebug() << "Domain name:" << QHostInfo::localDomainName();
}

void tst_QHostInfo::initTestCase()
{
#ifndef QT_NO_BEARERMANAGEMENT
    //start the default network
    netConfMan = new QNetworkConfigurationManager(this);
    networkConfiguration = netConfMan->defaultConfiguration();
    networkSession.reset(new QNetworkSession(networkConfiguration));
    if (!networkSession->isOpen()) {
        networkSession->open();
        networkSession->waitForOpened(30000);
    }
#endif

    ipv6Available = false;
    ipv6LookupsAvailable = false;

    QTcpServer server;
    if (server.listen(QHostAddress("::1"))) {
        // We have IPv6 support
        ipv6Available = true;
    }

    // check if the system getaddrinfo can do IPv6 lookups
    struct addrinfo hint, *result = 0;
    memset(&hint, 0, sizeof hint);
    hint.ai_family = AF_UNSPEC;
#ifdef AI_ADDRCONFIG
    hint.ai_flags = AI_ADDRCONFIG;
#endif

    int res = getaddrinfo("::1", "80", &hint, &result);
    if (res == 0) {
        // this test worked
        freeaddrinfo(result);
        res = getaddrinfo("aaaa-single" TEST_DOMAIN, "80", &hint, &result);
        if (res == 0 && result != 0 && result->ai_family != AF_INET) {
            freeaddrinfo(result);
            ipv6LookupsAvailable = true;
        }
    }

    // run each testcase with and without test enabled
    QTest::addColumn<bool>("cache");
    QTest::newRow("WithCache") << true;
    QTest::newRow("WithoutCache") << false;
}

void tst_QHostInfo::init()
{
    // delete the cache so inidividual testcase results are independent from each other
    qt_qhostinfo_clear_cache();

    QFETCH_GLOBAL(bool, cache);
    qt_qhostinfo_enable_cache(cache);
}

void tst_QHostInfo::lookupIPv4_data()
{
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<QString>("addresses");
    QTest::addColumn<int>("err");

    QTest::newRow("empty") << "" << "" << int(QHostInfo::HostNotFound);

    QTest::newRow("single_ip4") << "a-single" TEST_DOMAIN << "192.0.2.1" << int(QHostInfo::NoError);
    QTest::newRow("multiple_ip4") << "a-multi" TEST_DOMAIN << "192.0.2.1 192.0.2.2 192.0.2.3" << int(QHostInfo::NoError);
    QTest::newRow("literal_ip4") << "192.0.2.1" << "192.0.2.1" << int(QHostInfo::NoError);

    QTest::newRow("notfound") << "invalid" TEST_DOMAIN << "" << int(QHostInfo::HostNotFound);

    QTest::newRow("idn-ace") << "a-single.xn--alqualond-34a" TEST_DOMAIN << "192.0.2.1" << int(QHostInfo::NoError);
    QTest::newRow("idn-unicode") << QString::fromLatin1("a-single.alqualond\353" TEST_DOMAIN) << "192.0.2.1" << int(QHostInfo::NoError);
}

void tst_QHostInfo::lookupIPv4()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    lookupDone = false;
    QHostInfo::lookupHost(hostname, this, SLOT(resultsReady(QHostInfo)));

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(lookupDone);

    if ((int)lookupResults.error() != (int)err) {
        qWarning() << hostname << "=>" << lookupResults.errorString();
    }
    QCOMPARE((int)lookupResults.error(), (int)err);

    QStringList tmp;
    for (int i = 0; i < lookupResults.addresses().count(); ++i)
        tmp.append(lookupResults.addresses().at(i).toString());
    tmp.sort();

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' '), expected.join(' '));
}

void tst_QHostInfo::lookupIPv6_data()
{
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<QString>("addresses");
    QTest::addColumn<int>("err");

    QTest::newRow("aaaa-single") << "aaaa-single" TEST_DOMAIN << "2001:db8::1" << int(QHostInfo::NoError);
    QTest::newRow("aaaa-multi") << "aaaa-multi" TEST_DOMAIN << "2001:db8::1 2001:db8::2 2001:db8::3" << int(QHostInfo::NoError);
    QTest::newRow("a-plus-aaaa") << "a-plus-aaaa" TEST_DOMAIN << "198.51.100.1 2001:db8::1:1" << int(QHostInfo::NoError);

    // avoid using real IPv6 addresses here because this will do a DNS query
    // real addresses are between 2000:: and 3fff:ffff:ffff:ffff:ffff:ffff:ffff
    QTest::newRow("literal_ip6") << "f001:6b0:1:ea:202:a5ff:fecd:13a6" << "f001:6b0:1:ea:202:a5ff:fecd:13a6" << int(QHostInfo::NoError);
    QTest::newRow("literal_shortip6") << "f001:618:1401::4" << "f001:618:1401::4" << int(QHostInfo::NoError);
}

void tst_QHostInfo::lookupIPv6()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    if (!ipv6LookupsAvailable)
        QSKIP("This platform does not support IPv6 lookups");

    lookupDone = false;
    QHostInfo::lookupHost(hostname, this, SLOT(resultsReady(QHostInfo)));

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(lookupDone);

    QCOMPARE((int)lookupResults.error(), (int)err);

    QStringList tmp;
    for (int i = 0; i < lookupResults.addresses().count(); ++i)
        tmp.append(lookupResults.addresses().at(i).toString());
    tmp.sort();

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' ').toLower(), expected.join(' ').toLower());
}

void tst_QHostInfo::lookupConnectToFunctionPointer_data()
{
    lookupIPv4_data();
}

void tst_QHostInfo::lookupConnectToFunctionPointer()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    lookupDone = false;
    QHostInfo::lookupHost(hostname, this, &tst_QHostInfo::resultsReady);

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(lookupDone);

    if (int(lookupResults.error()) != int(err))
        qWarning() << hostname << "=>" << lookupResults.errorString();
    QCOMPARE(int(lookupResults.error()), int(err));

    QStringList tmp;
    for (const auto &result : lookupResults.addresses())
        tmp.append(result.toString());
    tmp.sort();

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' '), expected.join(' '));
}

void tst_QHostInfo::lookupConnectToFunctionPointerDeleted()
{
    {
        QObject contextObject;
        QHostInfo::lookupHost("localhost", &contextObject, [](const QHostInfo){
            QFAIL("This should never be called!");
        });
    }
    QTestEventLoop::instance().enterLoop(3);
}

void tst_QHostInfo::lookupConnectToLambda_data()
{
    lookupIPv4_data();
}

void tst_QHostInfo::lookupConnectToLambda()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    lookupDone = false;
    QHostInfo::lookupHost(hostname, [=](const QHostInfo &hostInfo) {
        resultsReady(hostInfo);
    });

    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(lookupDone);

    if (int(lookupResults.error()) != int(err))
        qWarning() << hostname << "=>" << lookupResults.errorString();
    QCOMPARE(int(lookupResults.error()), int(err));

    QStringList tmp;
    for (int i = 0; i < lookupResults.addresses().count(); ++i)
        tmp.append(lookupResults.addresses().at(i).toString());
    tmp.sort();

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' '), expected.join(' '));
}

static QStringList reverseLookupHelper(const QString &ip)
{
    QStringList results;

    const QString pythonCode =
        "import socket;"
        "import sys;"
        "print (socket.getnameinfo((sys.argv[1], 0), 0)[0]);";

    QList<QByteArray> lines;
    QProcess python;
    python.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    python.start("python", QStringList() << QString("-c") << pythonCode << ip);
    if (python.waitForFinished()) {
        if (python.exitStatus() == QProcess::NormalExit && python.exitCode() == 0)
            lines = python.readAllStandardOutput().split('\n');
        for (QByteArray line : lines) {
            if (!line.isEmpty())
                results << line.trimmed();
        }
        if (!results.isEmpty())
            return results;
    }

    qDebug() << "Python failed, falling back to nslookup";
    QProcess lookup;
    lookup.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    lookup.start("nslookup", QStringList(ip));
    if (!lookup.waitForFinished()) {
        results << "nslookup failure";
        qDebug() << "nslookup failure";
        return results;
    }
    lines = lookup.readAllStandardOutput().split('\n');

    QByteArray name;

    const QByteArray nameMarkerNix("name =");
    const QByteArray nameMarkerWin("Name:");
    const QByteArray addressMarkerWin("Address:");

    for (QByteArray line : lines) {
        int index = -1;
        if ((index = line.indexOf(nameMarkerNix)) != -1) { // Linux and macOS
            name = line.mid(index + nameMarkerNix.length()).chopped(1).trimmed();
            results << name;
        } else if (line.startsWith(nameMarkerWin)) { // Windows formatting
            name = line.mid(line.lastIndexOf(" ")).trimmed();
        } else if (line.startsWith(addressMarkerWin)) {
            QByteArray address = line.mid(addressMarkerWin.length()).trimmed();
            if (address == ip.toUtf8()) {
                results << name;
            }
        }
    }

    if (results.isEmpty()) {
        qDebug() << "Failure to parse nslookup output: " << lines;
    }
    return results;
}

void tst_QHostInfo::reverseLookup_data()
{
    QTest::addColumn<QString>("address");
    QTest::addColumn<QStringList>("hostNames");
    QTest::addColumn<int>("err");
    QTest::addColumn<bool>("ipv6");

    QTest::newRow("dns.google") << QString("8.8.8.8") << reverseLookupHelper("8.8.8.8") << 0 << false;
    QTest::newRow("one.one.one.one") << QString("1.1.1.1") << reverseLookupHelper("1.1.1.1") << 0 << false;
    QTest::newRow("dns.google IPv6") << QString("2001:4860:4860::8888") << reverseLookupHelper("2001:4860:4860::8888") << 0 << true;
    QTest::newRow("cloudflare IPv6") << QString("2606:4700:4700::1111") << reverseLookupHelper("2606:4700:4700::1111") << 0 << true;
    QTest::newRow("bogus-name IPv6") << QString("1::2::3::4") << QStringList() << 1 << true;
}

void tst_QHostInfo::reverseLookup()
{
    QFETCH(QString, address);
    QFETCH(QStringList, hostNames);
    QFETCH(int, err);
    QFETCH(bool, ipv6);

    if (ipv6 && !ipv6LookupsAvailable) {
        QSKIP("IPv6 reverse lookups are not supported on this platform");
    }

    QHostInfo info = QHostInfo::fromName(address);

    if (err == 0) {
        if (!hostNames.contains(info.hostName()))
            qDebug() << "Failure: expecting" << hostNames << ",got " << info.hostName();
        QVERIFY(hostNames.contains(info.hostName()));
        QCOMPARE(info.addresses().first(), QHostAddress(address));
    } else {
        QCOMPARE(info.hostName(), address);
        QCOMPARE(info.error(), QHostInfo::HostNotFound);
    }

}

void tst_QHostInfo::blockingLookup_data()
{
    lookupIPv4_data();
    if (ipv6LookupsAvailable)
        lookupIPv6_data();
}

void tst_QHostInfo::blockingLookup()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    QHostInfo hostInfo = QHostInfo::fromName(hostname);
    QStringList tmp;
    for (int i = 0; i < hostInfo.addresses().count(); ++i)
        tmp.append(hostInfo.addresses().at(i).toString());
    tmp.sort();

    if ((int)hostInfo.error() != (int)err) {
        qWarning() << hostname << "=>" << lookupResults.errorString();
    }
    QCOMPARE((int)hostInfo.error(), (int)err);

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' ').toUpper(), expected.join(' ').toUpper());
}

void tst_QHostInfo::raceCondition()
{
    for (int i = 0; i < 1000; ++i) {
        QTcpSocket socket;
        socket.connectToHost("invalid" TEST_DOMAIN, 80);
    }
}

class LookupThread : public QThread
{
protected:
    inline void run()
    {
         QHostInfo info = QHostInfo::fromName("a-single" TEST_DOMAIN);
         QCOMPARE(info.error(), QHostInfo::NoError);
         QVERIFY(info.addresses().count() > 0);
         QCOMPARE(info.addresses().at(0).toString(), QString("192.0.2.1"));
    }
};

void tst_QHostInfo::threadSafety()
{
    const int nattempts = 5;
    const int runs = 100;
    LookupThread thr[nattempts];
    for (int j = 0; j < runs; ++j) {
        for (int i = 0; i < nattempts; ++i)
            thr[i].start();
        for (int k = nattempts - 1; k >= 0; --k)
            thr[k].wait();
    }
}

class LookupReceiver : public QObject
{
    Q_OBJECT
public slots:
    void start();
    void resultsReady(const QHostInfo&);
public:
    QHostInfo result;
    int numrequests;
};

void LookupReceiver::start()
{
    for (int i=0;i<numrequests;i++)
        QHostInfo::lookupHost(QString("a-single" TEST_DOMAIN), this, SLOT(resultsReady(QHostInfo)));
}

void LookupReceiver::resultsReady(const QHostInfo &info)
{
    result = info;
    numrequests--;
    if (numrequests == 0 || info.error() != QHostInfo::NoError)
        QThread::currentThread()->quit();
}

void tst_QHostInfo::threadSafetyAsynchronousAPI()
{
    const int nattempts = 10;
    const int lookupsperthread = 10;
    QList<QThread*> threads;
    QList<LookupReceiver*> receivers;
    for (int i = 0; i < nattempts; ++i) {
        QThread* thread = new QThread;
        LookupReceiver* receiver = new LookupReceiver;
        receiver->numrequests = lookupsperthread;
        receivers.append(receiver);
        receiver->moveToThread(thread);
        connect(thread, SIGNAL(started()), receiver, SLOT(start()));
        thread->start();
        threads.append(thread);
    }
    for (int k = threads.count() - 1; k >= 0; --k)
        QVERIFY(threads.at(k)->wait(60000));
    foreach (LookupReceiver* receiver, receivers) {
        QCOMPARE(receiver->result.error(), QHostInfo::NoError);
        QCOMPARE(receiver->result.addresses().at(0).toString(), QString("192.0.2.1"));
        QCOMPARE(receiver->numrequests, 0);
    }
}

// this test is for the multi-threaded QHostInfo rewrite. It is about getting results at all,
// not about getting correct IPs
void tst_QHostInfo::multipleSameLookups()
{
    const int COUNT = 10;
    lookupsDoneCounter = 0;

    for (int i = 0; i < COUNT; i++)
        QHostInfo::lookupHost("localhost", this, SLOT(resultsReady(QHostInfo)));

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10000 && lookupsDoneCounter < COUNT) {
        QTestEventLoop::instance().enterLoop(2);
    }
    QCOMPARE(lookupsDoneCounter, COUNT);
}

// this test is for the multi-threaded QHostInfo rewrite. It is about getting results at all,
// not about getting correct IPs
void tst_QHostInfo::multipleDifferentLookups_data()
{
    QTest::addColumn<int>("repeats");
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
    QTest::newRow("5") << 5;
    QTest::newRow("10") << 10;
}

void tst_QHostInfo::multipleDifferentLookups()
{
    QStringList hostnameList;
    hostnameList << "a-single" TEST_DOMAIN
                 << "a-multi" TEST_DOMAIN
                 << "aaaa-single" TEST_DOMAIN
                 << "aaaa-multi" TEST_DOMAIN
                 << "a-plus-aaaa" TEST_DOMAIN
                 << "multi" TEST_DOMAIN
                 << "localhost" TEST_DOMAIN
                 << "cname" TEST_DOMAIN
                 << "127.0.0.1" << "----";

    QFETCH(int, repeats);
    const int COUNT = hostnameList.size();
    lookupsDoneCounter = 0;

    for (int i = 0; i < hostnameList.size(); i++)
        for (int j = 0; j < repeats; ++j)
            QHostInfo::lookupHost(hostnameList.at(i), this, SLOT(resultsReady(QHostInfo)));

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 60000 && lookupsDoneCounter < repeats*COUNT) {
        QTestEventLoop::instance().enterLoop(2);
        //qDebug() << "t:" << timer.elapsed();
    }
    QCOMPARE(lookupsDoneCounter, repeats*COUNT);
}

void tst_QHostInfo::cache()
{
    QFETCH_GLOBAL(bool, cache);
    if (!cache)
        return; // test makes only sense when cache enabled

    // reset slot counter
    lookupsDoneCounter = 0;

    // lookup once, wait in event loop, result should not come directly.
    bool valid = true;
    int id = -1;
    QHostInfo result = qt_qhostinfo_lookup("localhost", this, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(!valid);
    QVERIFY(result.addresses().isEmpty());

    // loopkup second time, result should come directly
    valid = false;
    result = qt_qhostinfo_lookup("localhost", this, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QVERIFY(valid);
    QVERIFY(!result.addresses().isEmpty());

    // clear the cache
    qt_qhostinfo_clear_cache();

    // lookup third time, result should not come directly.
    valid = true;
    result = qt_qhostinfo_lookup("localhost", this, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(!valid);
    QVERIFY(result.addresses().isEmpty());

    // the slot should have been called 2 times.
    QCOMPARE(lookupsDoneCounter, 2);
}

void tst_QHostInfo::resultsReady(const QHostInfo &hi)
{
    QVERIFY(QThread::currentThread() == thread());
    lookupDone = true;
    lookupResults = hi;
    lookupsDoneCounter++;
    QTestEventLoop::instance().exitLoop();
}

void tst_QHostInfo::abortHostLookup()
{
    //reset counter
    lookupsDoneCounter = 0;
    bool valid = false;
    int id = -1;
    QHostInfo result = qt_qhostinfo_lookup("a-single" TEST_DOMAIN, this, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QVERIFY(!valid);
    //it is assumed that the DNS request/response in the backend is slower than it takes to call abort
    QHostInfo::abortHostLookup(id);
    QTestEventLoop::instance().enterLoop(5);
    QCOMPARE(lookupsDoneCounter, 0);
}

class LookupAborter : public QObject
{
    Q_OBJECT
public slots:
    void abort()
    {
        QHostInfo::abortHostLookup(id);
        QThread::currentThread()->quit();
    }
public:
    int id;
};

QTEST_MAIN(tst_QHostInfo)
#include "tst_qhostinfo.moc"
