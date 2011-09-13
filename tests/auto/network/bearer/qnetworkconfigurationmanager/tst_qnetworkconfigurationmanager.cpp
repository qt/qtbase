/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../qbearertestcommon.h"

#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworkconfigmanager.h>

#if defined(Q_OS_UNIX) && !defined(QT_NO_ICD) && !defined (Q_OS_SYMBIAN)
#include <stdio.h>
#include <iapconf.h>
#endif

QT_USE_NAMESPACE
class tst_QNetworkConfigurationManager : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void usedInThread(); // this test must be first, or it will falsely pass
    void allConfigurations();
    void defaultConfiguration();
    void configurationFromIdentifier();

private:
#if defined(Q_OS_UNIX) && !defined(QT_NO_ICD) && !defined (Q_OS_SYMBIAN)
    Maemo::IAPConf *iapconf;
    Maemo::IAPConf *iapconf2;
    Maemo::IAPConf *gprsiap;
#define MAX_IAPS 50
    Maemo::IAPConf *iaps[MAX_IAPS];
    QProcess *icd_stub;
#endif
};

void tst_QNetworkConfigurationManager::initTestCase()
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_ICD) && !defined (Q_OS_SYMBIAN)
    iapconf = new Maemo::IAPConf("007");
    iapconf->setValue("ipv4_type", "AUTO");
    iapconf->setValue("wlan_wepkey1", "connt");
    iapconf->setValue("wlan_wepdefkey", 1);
    iapconf->setValue("wlan_ssid", QByteArray("JamesBond"));
    iapconf->setValue("name", "James Bond");
    iapconf->setValue("type", "WLAN_INFRA");

    gprsiap = new Maemo::IAPConf("This-is-GPRS-IAP");
    gprsiap->setValue("ask_password", false);
    gprsiap->setValue("gprs_accesspointname", "internet");
    gprsiap->setValue("gprs_password", "");
    gprsiap->setValue("gprs_username", "");
    gprsiap->setValue("ipv4_autodns", true);
    gprsiap->setValue("ipv4_type", "AUTO");
    gprsiap->setValue("sim_imsi", "244070123456789");
    gprsiap->setValue("name", "MI6");
    gprsiap->setValue("type", "GPRS");

    iapconf2 = new Maemo::IAPConf("osso.net");
    iapconf2->setValue("ipv4_type", "AUTO");
    iapconf2->setValue("wlan_wepkey1", "osso.net");
    iapconf2->setValue("wlan_wepdefkey", 1);
    iapconf2->setValue("wlan_ssid", QByteArray("osso.net"));
    iapconf2->setValue("name", "osso.net");
    iapconf2->setValue("type", "WLAN_INFRA");
    iapconf2->setValue("wlan_security", "WEP");

    /* Create large number of IAPs in the gconf and see what happens */
    fflush(stdout);
    printf("Creating %d IAPS: ", MAX_IAPS);
    for (int i=0; i<MAX_IAPS; i++) {
	QString num = QString().sprintf("%d", i);
	QString iap = "iap-" + num;
	iaps[i] = new Maemo::IAPConf(iap);
	iaps[i]->setValue("name", QString("test-iap-")+num);
	iaps[i]->setValue("type", "WLAN_INFRA");
	iaps[i]->setValue("wlan_ssid", QString(QString("test-ssid-")+num).toAscii());
	iaps[i]->setValue("wlan_security", "WPA_PSK");
	iaps[i]->setValue("EAP_wpa_preshared_passphrase", QString("test-passphrase-")+num);
	printf(".");
	fflush(stdout);
    }
    printf("\n");
    fflush(stdout);

    icd_stub = new QProcess(this);
    icd_stub->start("/usr/bin/icd2_stub.py");
    QTest::qWait(1000);

    // Add a known network to scan list that icd2 stub returns
    QProcess dbus_send;
    // 007 network
    dbus_send.start("dbus-send --type=method_call --system "
		    "--dest=com.nokia.icd2 /com/nokia/icd2 "
		    "com.nokia.icd2.testing.add_available_network "
		    "string:'' uint32:0 string:'' "
		    "string:WLAN_INFRA uint32:5000011 array:byte:48,48,55");
    dbus_send.waitForFinished();

    // osso.net network
    dbus_send.start("dbus-send --type=method_call --system "
		    "--dest=com.nokia.icd2 /com/nokia/icd2 "
		    "com.nokia.icd2.testing.add_available_network "
		    "string:'' uint32:0 string:'' "
		    "string:WLAN_INFRA uint32:83886097 array:byte:111,115,115,111,46,110,101,116");
    dbus_send.waitForFinished();
#endif
}


void tst_QNetworkConfigurationManager::cleanupTestCase()
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_ICD) && !defined (Q_OS_SYMBIAN)
    iapconf->clear();
    delete iapconf;
    iapconf2->clear();
    delete iapconf2;
    gprsiap->clear();
    delete gprsiap;

    printf("Deleting %d IAPS : ", MAX_IAPS);
    for (int i=0; i<MAX_IAPS; i++) {
	iaps[i]->clear();
	delete iaps[i];
	printf(".");
	fflush(stdout);
    }
    printf("\n");
    qDebug() << "Deleted" << MAX_IAPS << "IAPs";

    icd_stub->terminate();
    icd_stub->waitForFinished();
#endif
}

void tst_QNetworkConfigurationManager::init()
{
}

void tst_QNetworkConfigurationManager::cleanup()
{
}

void printConfigurationDetails(const QNetworkConfiguration& p)
{
    qDebug() << p.name() <<":  isvalid->" <<p.isValid() << " type->"<< p.type() << 
                " roaming->" << p.isRoamingAvailable() << "identifier->" << p.identifier() <<
                " purpose->" << p.purpose() << " state->" << p.state();
}

void tst_QNetworkConfigurationManager::allConfigurations()
{
    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> preScanConfigs = manager.allConfigurations();

    foreach(QNetworkConfiguration c, preScanConfigs)
    {
        QVERIFY2(c.type()!=QNetworkConfiguration::UserChoice, "allConfiguration must not return UserChoice configs");
    }

    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY(spy.count() == 1); //wait for scan to complete

    QList<QNetworkConfiguration> configs = manager.allConfigurations();

    int all = configs.count();
    qDebug() << "All configurations:" << all;
    QVERIFY(all);
    foreach(QNetworkConfiguration p, configs) {
        QVERIFY(p.isValid());
        printConfigurationDetails(p);
        QVERIFY(p.type() != QNetworkConfiguration::Invalid);
        QVERIFY(p.type() != QNetworkConfiguration::UserChoice);
    }

    configs = manager.allConfigurations(QNetworkConfiguration::Undefined);
    int undefined = configs.count();
    QVERIFY(undefined <= all);
    qDebug() << "Undefined configurations:" << undefined;
    foreach( const QNetworkConfiguration p, configs) {
        printConfigurationDetails(p);
        QVERIFY(p.state() & QNetworkConfiguration::Undefined);
        QVERIFY(!(p.state() & QNetworkConfiguration::Defined));
    }

    //get defined configs only (same as all)
    configs = manager.allConfigurations(QNetworkConfiguration::Defined);
    int defined = configs.count();
    qDebug() << "Defined configurations:" << defined;
    QVERIFY(defined <= all);
    foreach( const QNetworkConfiguration p, configs) {
        printConfigurationDetails(p);
        QVERIFY(p.state() & QNetworkConfiguration::Defined);
        QVERIFY(!(p.state() & QNetworkConfiguration::Undefined));
    }

    //get discovered configurations only
    configs = manager.allConfigurations(QNetworkConfiguration::Discovered);
    int discovered = configs.count();
    //QVERIFY(discovered);
    qDebug() << "Discovered configurations:" << discovered;
    foreach(const QNetworkConfiguration p, configs) {
        printConfigurationDetails(p);
        QVERIFY(p.isValid());
        QVERIFY(!(p.state() & QNetworkConfiguration::Undefined));
        QVERIFY(p.state() & QNetworkConfiguration::Defined);
        QVERIFY(p.state() & QNetworkConfiguration::Discovered);
    }

    //getactive configurations only
    configs = manager.allConfigurations(QNetworkConfiguration::Active);
    int active = configs.count();
    if (active)
        QVERIFY(manager.isOnline());
    else
        QVERIFY(!manager.isOnline());

    //QVERIFY(active);
    qDebug() << "Active configurations:" << active;
    foreach(const QNetworkConfiguration p, configs) {
        printConfigurationDetails(p);
        QVERIFY(p.isValid());
        QVERIFY(!(p.state() & QNetworkConfiguration::Undefined));
        QVERIFY(p.state() & QNetworkConfiguration::Active);
        QVERIFY(p.state() & QNetworkConfiguration::Discovered);
        QVERIFY(p.state() & QNetworkConfiguration::Defined);
    }

    QVERIFY(all >= discovered);
    QVERIFY(discovered >= active);
}


void tst_QNetworkConfigurationManager::defaultConfiguration()
{
    QNetworkConfigurationManager manager;
    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY(spy.count() == 1); //wait for scan to complete

    QList<QNetworkConfiguration> configs = manager.allConfigurations();
    QNetworkConfiguration defaultConfig = manager.defaultConfiguration();

    bool confirm = configs.contains(defaultConfig);

    if (defaultConfig.type() != QNetworkConfiguration::UserChoice) {
        QVERIFY(confirm || !defaultConfig.isValid());
        QVERIFY(!(confirm && !defaultConfig.isValid()));
    } else {
        QVERIFY(!confirm);  // user choice config is not part of allConfigurations()
        QVERIFY(defaultConfig.isValid());
        QCOMPARE(defaultConfig.name(), QString("UserChoice"));
        QCOMPARE(defaultConfig.children().count(), 0);
        QVERIFY(!defaultConfig.isRoamingAvailable());
        QCOMPARE(defaultConfig.state(), QNetworkConfiguration::Discovered);
        QNetworkConfiguration copy = manager.configurationFromIdentifier(defaultConfig.identifier());
        QVERIFY(copy == defaultConfig);
    }
}

void tst_QNetworkConfigurationManager::configurationFromIdentifier()
{
    QNetworkConfigurationManager manager;
    QSet<QString> allIdentifier;

    //force an update to get maximum number of configs
    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY(spy.count() == 1); //wait for scan to complete
    
    QList<QNetworkConfiguration> configs = manager.allConfigurations();

    foreach(QNetworkConfiguration c, configs) {
        QVERIFY(!allIdentifier.contains(c.identifier()));
        allIdentifier.insert(c.identifier());

        QNetworkConfiguration direct = manager.configurationFromIdentifier(c.identifier());
        QVERIFY(direct.isValid());
        QVERIFY(direct == c);
    }

    //assume that there is no item with identifier 'FooBar'
    QVERIFY(!allIdentifier.contains("FooBar"));
    QNetworkConfiguration invalid = manager.configurationFromIdentifier("FooBar");
    QVERIFY(!invalid.isValid());
}

class QNCMTestThread : public QThread
{
protected:
    virtual void run()
    {
        QNetworkConfigurationManager manager;
        preScanConfigs = manager.allConfigurations();
        QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
        manager.updateConfigurations(); //initiate scans
        QTRY_VERIFY(spy.count() == 1); //wait for scan to complete
        configs = manager.allConfigurations();
    }
public:
    QList<QNetworkConfiguration> configs;
    QList<QNetworkConfiguration> preScanConfigs;
};

// regression test for QTBUG-18795
void tst_QNetworkConfigurationManager::usedInThread()
{
#if defined Q_OS_MAC && !defined (QT_NO_COREWLAN)
    QSKIP("QTBUG-19070 Mac CoreWlan plugin is broken", SkipAll);
#else
    QNCMTestThread thread;
    connect(&thread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    thread.start();
    QTestEventLoop::instance().enterLoop(100); //QTRY_VERIFY could take ~90 seconds to time out in the thread
    QVERIFY(!QTestEventLoop::instance().timeout());
    qDebug() << "prescan:" << thread.preScanConfigs.count();
    qDebug() << "postscan:" << thread.configs.count();

    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> preScanConfigs = manager.allConfigurations();
    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY(spy.count() == 1); //wait for scan to complete
    QList<QNetworkConfiguration> configs = manager.allConfigurations();
    QCOMPARE(thread.configs, configs);
    //Don't compare pre scan configs, because these may be cached and therefore give different results
    //which makes the test unstable.  The post scan results should have all configurations every time
    //QCOMPARE(thread.preScanConfigs, preScanConfigs);
#endif
}

QTEST_MAIN(tst_QNetworkConfigurationManager)
#include "tst_qnetworkconfigurationmanager.moc"
