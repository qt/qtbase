// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


// When using WinSock2 on Windows, it's the first thing that can be included
// (except qglobal.h), or else you'll get tons of compile errors
#include <qglobal.h>

#if defined(Q_OS_WIN)
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include <qhostinfo.h>
#include "private/qhostinfo_p.h"
#include "private/qnativesocketengine_p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTestEventLoop>

#include <private/qthread_p.h>

#include <sys/types.h>

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
#  ifdef gai_strerror
#    undef gai_strerror
#    define gai_strerror gai_strerrorA
#  endif
#else
#  include <netdb.h>
#  include <sys/socket.h>
#  include <unistd.h>
#endif
#ifndef NI_MAXHOST
#  define NI_MAXHOST        1025
#endif

#include "../../../network-settings.h"

#define TEST_DOMAIN ".test.qt-project.org"

using namespace std::chrono_literals;

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

private:
    bool ipv6LookupsAvailable;
    bool ipv6Available;
};

class tst_QHostInfo_Helper : public QObject
{
    Q_OBJECT
protected slots:
    void resultsReady(const QHostInfo &);
public:
    tst_QHostInfo_Helper(const QString &hostname)
        : hostname(hostname)
    {}

    QString hostname;
    bool lookupDone = false;
    int lookupsDoneCounter = 0;
    QHostInfo lookupResults;

    void blockingLookup()
    {
        lookupResults = QHostInfo::fromName(hostname);
        lookupDone = true;
        ++lookupsDoneCounter;
    }
    void lookupHostOldStyle()
    {
        QHostInfo::lookupHost(hostname, this, SLOT(resultsReady(QHostInfo)));
    }
    void lookupHostNewStyle()
    {
        QHostInfo::lookupHost(hostname, this, &tst_QHostInfo_Helper::resultsReady);
    }
    void lookupHostLambda()
    {
        QHostInfo::lookupHost(hostname, this, [this](const QHostInfo &hostInfo) {
            resultsReady(hostInfo);
        });
    }

    bool waitForResults(std::chrono::milliseconds timeout = 15s)
    {
        QTestEventLoop::instance().enterLoop(timeout);
        return !QTestEventLoop::instance().timeout() && lookupDone;
    }

    void checkResults(QHostInfo::HostInfoError err, const QString &addresses);
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

void tst_QHostInfo_Helper::checkResults(QHostInfo::HostInfoError err, const QString &addresses)
{
    if ((int)lookupResults.error() != (int)err) {
        qWarning() << hostname << "=>" << lookupResults.errorString();
    }
    QCOMPARE(lookupResults.error(), err);

    QStringList tmp;
    for (int i = 0; i < lookupResults.addresses().size(); ++i)
        tmp.append(lookupResults.addresses().at(i).toString());
    tmp.sort();

    QStringList expected = addresses.split(' ');
    expected.sort();

    QCOMPARE(tmp.join(' '), expected.join(' '));
}

void tst_QHostInfo::lookupIPv4()
{
    QFETCH(QString, hostname);
    QFETCH(int, err);
    QFETCH(QString, addresses);

    tst_QHostInfo_Helper helper(hostname);
    helper.lookupHostOldStyle();
    QVERIFY(helper.waitForResults());
    helper.checkResults(QHostInfo::HostInfoError(err), addresses);
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

    tst_QHostInfo_Helper helper(hostname);
    helper.lookupHostOldStyle();
    QVERIFY(helper.waitForResults());
    helper.checkResults(QHostInfo::HostInfoError(err), addresses);
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

    tst_QHostInfo_Helper helper(hostname);
    helper.lookupHostNewStyle();
    QVERIFY(helper.waitForResults());
    helper.checkResults(QHostInfo::HostInfoError(err), addresses);
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

    tst_QHostInfo_Helper helper(hostname);
    helper.lookupHostLambda();
    QVERIFY(helper.waitForResults());
    helper.checkResults(QHostInfo::HostInfoError(err), addresses);
}

static QStringList reverseLookupHelper(const QString &ip)
{
    QStringList results;
    union qt_sockaddr {
        sockaddr a;
        sockaddr_in a4;
        sockaddr_in6 a6;
    } sa = {};

    QHostAddress addr(ip);
    if (addr.isNull()) {
        qWarning("Could not parse IP address: %ls", qUtf16Printable(ip));
        return results;
    }

    // from qnativesocketengine_p.h:
    QT_SOCKLEN_T len = setSockaddr(&sa.a, addr, /*port = */ 0);

    QByteArray name(NI_MAXHOST, Qt::Uninitialized);
    int ni_flags = NI_NAMEREQD | NI_NUMERICSERV;
    if (int r = getnameinfo(&sa.a, len, name.data(), name.size(), nullptr, 0, ni_flags)) {
        qWarning("Failed to reverse look up '%ls': %s", qUtf16Printable(ip), gai_strerror(r));
    } else {
        results << QString::fromLatin1(name, qstrnlen(name, name.size()));
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
    if (QStringList hostNames = reverseLookupHelper("2001:4860:4860::8888"); !hostNames.isEmpty())
        QTest::newRow("dns.google IPv6") << QString("2001:4860:4860::8888") << std::move(hostNames) << 0 << true;
    if (QStringList hostNames = reverseLookupHelper("2606:4700:4700::1111"); !hostNames.isEmpty())
        QTest::newRow("cloudflare IPv6") << QString("2606:4700:4700::1111") << std::move(hostNames) << 0 << true;
    QTest::newRow("bogus-name IPv6") << QString("1::2::3::4") << QStringList() << 1 << true;
}

void tst_QHostInfo::reverseLookup()
{
    QFETCH(QString, address);
    QFETCH(QStringList, hostNames);
    QFETCH(int, err);

    QHostInfo info = QHostInfo::fromName(address);

    if (err == 0) {
        if (!hostNames.contains(info.hostName()))
            qDebug() << "Failure: expecting" << hostNames << ",got " << info.hostName();
        QVERIFY(hostNames.contains(info.hostName()));
        QCOMPARE(info.addresses().constFirst(), QHostAddress(address));
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

    tst_QHostInfo_Helper helper(hostname);
    helper.blockingLookup();
    helper.checkResults(QHostInfo::HostInfoError(err), addresses);
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
    inline void run() override
    {
         QHostInfo info = QHostInfo::fromName("a-single" TEST_DOMAIN);
         QCOMPARE(info.errorString(), "Unknown error"); // no error
         QCOMPARE(info.error(), QHostInfo::NoError);
         QVERIFY(info.addresses().size() > 0);
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
    QThread threads[nattempts];
    LookupReceiver receivers[nattempts];
    for (int i = 0; i < nattempts; ++i) {
        QThread *thread = &threads[i];
        LookupReceiver *receiver = &receivers[i];
        receiver->numrequests = lookupsperthread;
        receiver->moveToThread(thread);
        connect(thread, SIGNAL(started()), receiver, SLOT(start()));
        thread->start();
    }
    for (int k = nattempts - 1; k >= 0; --k)
        QVERIFY(threads[k].wait(60000));
    for (LookupReceiver &receiver : receivers) {
        QCOMPARE(receiver.result.error(), QHostInfo::NoError);
        QCOMPARE(receiver.result.addresses().at(0).toString(), QString("192.0.2.1"));
        QCOMPARE(receiver.numrequests, 0);
    }
}

// this test is for the multi-threaded QHostInfo rewrite. It is about getting results at all,
// not about getting correct IPs
void tst_QHostInfo::multipleSameLookups()
{
    const int COUNT = 10;
    tst_QHostInfo_Helper helper("localhost");
    for (int i = 0; i < COUNT; i++)
        helper.lookupHostOldStyle();

    QTRY_COMPARE_WITH_TIMEOUT(helper.lookupsDoneCounter, COUNT, 10s);
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
    tst_QHostInfo_Helper helper(QString{});

    for (int i = 0; i < hostnameList.size(); i++)
        for (int j = 0; j < repeats; ++j) {
            helper.hostname = hostnameList.at(i);
            helper.lookupHostOldStyle();
        }

    QTRY_COMPARE_WITH_TIMEOUT(helper.lookupsDoneCounter, repeats*COUNT, 60s);
}

void tst_QHostInfo::cache()
{
    QFETCH_GLOBAL(bool, cache);
    if (!cache)
        return; // test makes only sense when cache enabled

    tst_QHostInfo_Helper helper("localhost");

    // lookup once, wait in event loop, result should not come directly.
    bool valid = true;
    int id = -1;
    QHostInfo result = qt_qhostinfo_lookup(helper.hostname, &helper, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(!valid);
    QVERIFY(result.addresses().isEmpty());

    // loopkup second time, result should come directly
    valid = false;
    result = qt_qhostinfo_lookup(helper.hostname, &helper, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QVERIFY(valid);
    QVERIFY(!result.addresses().isEmpty());

    // clear the cache
    qt_qhostinfo_clear_cache();

    // lookup third time, result should not come directly.
    valid = true;
    result = qt_qhostinfo_lookup(helper.hostname, &helper, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(!valid);
    QVERIFY(result.addresses().isEmpty());

    // the slot should have been called 2 times.
    QCOMPARE(helper.lookupsDoneCounter, 2);
}

void tst_QHostInfo_Helper::resultsReady(const QHostInfo &hi)
{
    QVERIFY(QThread::currentThread() == thread());
    lookupDone = true;
    lookupResults = hi;
    lookupsDoneCounter++;
    QTestEventLoop::instance().exitLoop();
}

void tst_QHostInfo::abortHostLookup()
{
    tst_QHostInfo_Helper helper("a-single" TEST_DOMAIN);
    bool valid = false;
    int id = -1;
    QHostInfo result = qt_qhostinfo_lookup(helper.hostname, &helper, SLOT(resultsReady(QHostInfo)), &valid, &id);
    QVERIFY(!valid);
    //it is assumed that the DNS request/response in the backend is slower than it takes to call abort
    QHostInfo::abortHostLookup(id);
    QTestEventLoop::instance().enterLoop(5);
    QCOMPARE(helper.lookupsDoneCounter, 0);
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
