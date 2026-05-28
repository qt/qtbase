// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtEndian>
#include <QSet>

#include <qcoreapplication.h>
#include <qnetworkinterface.h>
#if QT_CONFIG(udpsocket)
#include <qudpsocket.h>
#endif
#include "../../../network-settings.h"

#include <private/qtnetwork-config_p.h>

Q_DECLARE_METATYPE(QHostAddress)

class tst_QNetworkInterface : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkInterface();
    virtual ~tst_QNetworkInterface();

    bool isIPv6Working();

private slots:
    void initTestCase();
    void dump();
    void consistencyCheck();
    void loopbackIPv4();
    void loopbackIPv6();
    void localAddress_data();
    void localAddress();
    void interfaceFromXXX_data();
    void interfaceFromXXX();
    void copyInvalidInterface();
private:
    bool hasNetworkServer = false;
};

tst_QNetworkInterface::tst_QNetworkInterface()
{
}

tst_QNetworkInterface::~tst_QNetworkInterface()
{
}

bool tst_QNetworkInterface::isIPv6Working()
{
    // QNetworkInterface may be unable to detect IPv6 addresses even if they
    // are there, due to limitations of the implementation.
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows ||
            QT_CONFIG(linux_netlink) || (QT_CONFIG(getifaddrs) && QT_CONFIG(ipv6ifname))) {
        return QtNetworkSettings::hasIPv6();
    }
    return false;
}

void tst_QNetworkInterface::initTestCase()
{
#ifdef QT_TEST_SERVER
    hasNetworkServer = QtNetworkSettings::verifyConnection(QtNetworkSettings::httpServerName(), 80);
#else
    hasNetworkServer = QtNetworkSettings::verifyTestNetworkSettings();
#endif
}

void tst_QNetworkInterface::dump()
{
    // This is for manual testing:
    const QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &i : allInterfaces) {
        QString flags;
        if (i.flags() & QNetworkInterface::IsUp) flags += "Up,";
        if (i.flags() & QNetworkInterface::IsRunning) flags += "Running,";
        if (i.flags() & QNetworkInterface::CanBroadcast) flags += "Broadcast,";
        if (i.flags() & QNetworkInterface::IsLoopBack) flags += "Loopback,";
        if (i.flags() & QNetworkInterface::IsPointToPoint) flags += "PointToPoint,";
        if (i.flags() & QNetworkInterface::CanMulticast) flags += "Multicast,";
        flags.chop(1);          // drop last comma

        QString friendlyName = i.humanReadableName();
        if (friendlyName != i.name()) {
            friendlyName.prepend('(');
            friendlyName.append(')');
        } else {
            friendlyName.clear();
        }
        qDebug() << "Interface:     " << i.name() << qPrintable(friendlyName);
        QVERIFY(i.isValid());

        qDebug() << "    index:     " << i.index();
        qDebug() << "    flags:     " << qPrintable(flags);
        qDebug() << "    type:      " << i.type();
        qDebug() << "    hw address:" << qPrintable(i.hardwareAddress());
        qDebug() << "    MTU:       " << i.maximumTransmissionUnit();

        int count = 0;
        const auto entries = i.addressEntries();
        for (const QNetworkAddressEntry &e : entries) {
            QDebug s = qDebug();
            s.nospace() <<    "    address "
                        << qSetFieldWidth(2) << count++ << qSetFieldWidth(0);
            s.nospace() << ": " << qPrintable(e.ip().toString());
            if (!e.netmask().isNull())
                s.nospace() << '/' << e.prefixLength()
                            << " (" << qPrintable(e.netmask().toString()) << ')';
            if (!e.broadcast().isNull())
                s.nospace() << " broadcast " << qPrintable(e.broadcast().toString());
            if (e.dnsEligibility() == QNetworkAddressEntry::DnsEligible)
                s.nospace() << " dns-eligible";
            else if (e.dnsEligibility() == QNetworkAddressEntry::DnsIneligible)
                s.nospace() << " dns-ineligible";
            if (e.isLifetimeKnown()) {
#define printable(l) qPrintable(l.isForever() ? "forever" : QString::fromLatin1("%1ms").arg(l.remainingTime()))
                s.nospace() << " preferred:" << printable(e.preferredLifetime())
                            << " valid:" << printable(e.validityLifetime());
#undef printable
            }
        }
    }
}

void tst_QNetworkInterface::consistencyCheck()
{
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    QSet<QString> interfaceNames;
    QList<int> interfaceIndexes;

    for (const QNetworkInterface &iface : ifaces) {
        QVERIFY(iface.isValid());
        QVERIFY2(!interfaceNames.contains(iface.name()),
                 "duplicate name = " + iface.name().toLocal8Bit());
        interfaceNames << iface.name();

        QVERIFY2(!interfaceIndexes.contains(iface.index()),
                 "duplicate index = " + QByteArray::number(iface.index()));
        if (iface.index())
            interfaceIndexes << iface.index();

        QVERIFY(iface.maximumTransmissionUnit() >= 0);

        const QList<QNetworkAddressEntry> addresses = iface.addressEntries();
        for (auto entry : addresses) {
            QVERIFY(entry.ip().protocol() != QAbstractSocket::UnknownNetworkLayerProtocol);
            if (!entry.preferredLifetime().isForever() || !entry.validityLifetime().isForever())
                QVERIFY(entry.isLifetimeKnown());
            if (!entry.validityLifetime().isForever())
                QVERIFY(entry.isTemporary());
        }
    }
}

void tst_QNetworkInterface::loopbackIPv4()
{
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(QHostAddress(QHostAddress::LocalHost)));
}

void tst_QNetworkInterface::loopbackIPv6()
{
    if (!isIPv6Working())
        QSKIP("IPv6 not active on this machine");
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(QHostAddress(QHostAddress::LocalHostIPv6)));
}
void tst_QNetworkInterface::localAddress_data()
{
    bool ipv6 = isIPv6Working();
    QTest::addColumn<QHostAddress>("target");

    QTest::newRow("localhost-ipv4") << QHostAddress(QHostAddress::LocalHost);
    if (ipv6)
        QTest::newRow("localhost-ipv6") << QHostAddress(QHostAddress::LocalHostIPv6);

    if (hasNetworkServer)
        QTest::newRow("test-server") << QtNetworkSettings::httpServerIp();

    QSet<QHostAddress> added;
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        if ((iface.flags() & QNetworkInterface::IsUp) == 0)
            continue;
        const QList<QNetworkAddressEntry> addrs = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : addrs) {
            QHostAddress addr = entry.ip();
            if (addr.isLoopback())
                continue;       // added above

            int prefixLength = entry.prefixLength();
            if (prefixLength > 0 && addr.protocol() == QAbstractSocket::IPv4Protocol) {
                // add an IPv4 address with bits in the host portion of the address flipped
                // (for point-to-point, we flip the least significant bit)
                quint32 ip4 = entry.ip().toIPv4Address();
                quint32 hostmask = 0x1;
                if (prefixLength != 32)
                    hostmask = ~0U >> prefixLength;
                addr.setAddress(ip4 ^ hostmask);
            } else if (!ipv6 || prefixLength != 64) {
                continue;
            } else {
#ifdef Q_OS_ANDROID
                // Android seem to not allow IPv6 connection from interfaces other than wlan,
                // if it's connected, and wlan is connected by default on Android emulators,
                // so prefer selecting wlan in this test.
                const QString scopeId = addr.scopeId();
                if (!scopeId.isEmpty() && !scopeId.startsWith("wlan"))
                    continue;
#endif
                // add a random node in this IPv6 network
                quint64 randomid = qFromBigEndian(Q_UINT64_C(0x8f41f072e5733caa));
                QIPv6Address ip6 = addr.toIPv6Address();
                memcpy(&ip6[8], &randomid, sizeof(randomid));
                addr.setAddress(ip6);
            }

            if (added.contains(addr))
                continue;
            added.insert(addr);

            QTest::addRow("%s-%s", qPrintable(iface.name()), qPrintable(addr.toString())) << addr;
        }
    }
}

void tst_QNetworkInterface::localAddress()
{
#if !QT_CONFIG(udpsocket)
    QSKIP("UDP socket support not built in");
#else
    QFETCH(QHostAddress, target);
    QUdpSocket socket;
    socket.connectToHost(target, 80);
    QVERIFY(socket.waitForConnected(5000));

    QHostAddress local = socket.localAddress();

    // find the interface that contains the address QUdpSocket reported
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    const QNetworkInterface *outgoingIface = nullptr;
    for (const QNetworkInterface &iface : ifaces) {
        QList<QNetworkAddressEntry> addrs = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : addrs) {
            if (entry.ip() == local) {
                outgoingIface = &iface;
                break;
            }
        }
        if (outgoingIface)
            break;
    }
    QVERIFY(outgoingIface);

    // we get QVariant() if the QNativeSocketEngine doesn't know how to get the PMTU
    int pmtu = socket.socketOption(QAbstractSocket::PathMtuSocketOption).toInt();
    qDebug() << "Connected to" << target.toString() << "via interface" << outgoingIface->name()
             << "pmtu" << pmtu;

    // check that the Path MTU is less than or equal the interface's MTU
    QCOMPARE_LE(pmtu, outgoingIface->maximumTransmissionUnit());
#endif // QT_CONFIG(udpsocket)
}

void tst_QNetworkInterface::interfaceFromXXX_data()
{
    QTest::addColumn<QNetworkInterface>("iface");

    const QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    if (allInterfaces.size() == 0)
        QSKIP("No interfaces to test!");
    for (const QNetworkInterface &iface : allInterfaces)
        QTest::newRow(iface.name().toLocal8Bit()) << iface;
}

void tst_QNetworkInterface::interfaceFromXXX()
{
    QFETCH(QNetworkInterface, iface);

    QVERIFY(QNetworkInterface::interfaceFromName(iface.name()).isValid());
    if (int idx = iface.index()) {
        QVERIFY(QNetworkInterface::interfaceFromIndex(idx).isValid());
        QCOMPARE(QNetworkInterface::interfaceNameFromIndex(idx), iface.name());
        QCOMPARE(QNetworkInterface::interfaceIndexFromName(iface.name()), idx);
    }
    const auto entries = iface.addressEntries();
    for (const QNetworkAddressEntry &entry : entries) {
        QVERIFY(!entry.ip().isNull());

        if (!entry.netmask().isNull()) {
            QCOMPARE(entry.netmask().protocol(), entry.ip().protocol());

            // if the netmask is known, the broadcast is known
            // but only for IPv4 (there is no such thing as broadcast in IPv6)
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                QVERIFY(!entry.broadcast().isNull());
            }
        }

        if (!entry.broadcast().isNull())
            QCOMPARE(entry.broadcast().protocol(), entry.ip().protocol());
    }
}

void tst_QNetworkInterface::copyInvalidInterface()
{
    // Force a copy of an interfaces that isn't likely to exist
    QNetworkInterface i = QNetworkInterface::interfaceFromName("plopp");
    QVERIFY(!i.isValid());

    QCOMPARE(i.index(), 0);
    QVERIFY(i.name().isEmpty());
    QVERIFY(i.humanReadableName().isEmpty());
    QVERIFY(i.hardwareAddress().isEmpty());
    QCOMPARE(int(i.flags()), 0);
    QVERIFY(i.addressEntries().isEmpty());
}

QTEST_MAIN(tst_QNetworkInterface)
#include "tst_qnetworkinterface.moc"
