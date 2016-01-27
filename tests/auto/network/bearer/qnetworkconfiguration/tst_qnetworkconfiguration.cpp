/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../qbearertestcommon.h"

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworkconfigmanager.h>
#endif

/*
  Although this unit test doesn't use QNetworkAccessManager
  this include is used to ensure that bearer continues to compile against
  Qt 4.7+ which has a QNetworkConfiguration enabled QNetworkAccessManager
*/
#include <QNetworkAccessManager>

QT_USE_NAMESPACE

class tst_QNetworkConfiguration : public QObject
{
    Q_OBJECT

private slots:
#ifndef QT_NO_BEARERMANAGEMENT
    void invalidPoint();
    void comparison();
    void children();
    void isRoamingAvailable();
#endif
};

#ifndef QT_NO_BEARERMANAGEMENT
void tst_QNetworkConfiguration::invalidPoint()
{
    QNetworkConfiguration pt;

    QVERIFY(pt.name().isEmpty());
    QVERIFY(!pt.isValid());
    QCOMPARE(pt.type(), QNetworkConfiguration::Invalid);
    QVERIFY(!(pt.state() & QNetworkConfiguration::Defined));
    QVERIFY(!(pt.state() & QNetworkConfiguration::Discovered));
    QVERIFY(!(pt.state() & QNetworkConfiguration::Active));
    QVERIFY(!pt.isRoamingAvailable());

    QNetworkConfiguration pt2(pt);
    QVERIFY(pt2.name().isEmpty());
    QVERIFY(!pt2.isValid());
    QCOMPARE(pt2.type(), QNetworkConfiguration::Invalid);
    QVERIFY(!(pt2.state() & QNetworkConfiguration::Defined));
    QVERIFY(!(pt2.state() & QNetworkConfiguration::Discovered));
    QVERIFY(!(pt2.state() & QNetworkConfiguration::Active));
    QVERIFY(!pt2.isRoamingAvailable());

}

void tst_QNetworkConfiguration::comparison()
{
    //test copy constructor and assignment operator
    //compare invalid connection points
    QNetworkConfiguration pt1;
    QVERIFY(!pt1.isValid());
    QCOMPARE(pt1.type(), QNetworkConfiguration::Invalid);

    QNetworkConfiguration pt2(pt1);
    QVERIFY(pt1==pt2);
    QVERIFY(!(pt1!=pt2));
    QCOMPARE(pt1.name(), pt2.name());
    QCOMPARE(pt1.isValid(), pt2.isValid());
    QCOMPARE(pt1.type(), pt2.type());
    QCOMPARE(pt1.state(), pt2.state());
    QCOMPARE(pt1.purpose(), pt2.purpose());


    QNetworkConfiguration pt3;
    pt3 = pt1;
    QVERIFY(pt1==pt3);
    QVERIFY(!(pt1!=pt3));
    QCOMPARE(pt1.name(), pt3.name());
    QCOMPARE(pt1.isValid(), pt3.isValid());
    QCOMPARE(pt1.type(), pt3.type());
    QCOMPARE(pt1.state(), pt3.state());
    QCOMPARE(pt1.purpose(), pt3.purpose());

    //test case must run on machine that has valid connection points
    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> preScanConfigs = manager.allConfigurations();

    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() == 1, TestTimeOut); //wait for scan to complete

    QList<QNetworkConfiguration> configs = manager.allConfigurations(QNetworkConfiguration::Discovered);
    QVERIFY(configs.count());
    QNetworkConfiguration defaultConfig = manager.defaultConfiguration();
    QVERIFY(defaultConfig.isValid());
    QVERIFY(defaultConfig.type() != QNetworkConfiguration::Invalid);
    QVERIFY(!defaultConfig.name().isEmpty());

    pt3 = defaultConfig;
    QVERIFY(defaultConfig==pt3);
    QVERIFY(!(defaultConfig!=pt3));
    QCOMPARE(defaultConfig.name(), pt3.name());
    QCOMPARE(defaultConfig.isValid(), pt3.isValid());
    QCOMPARE(defaultConfig.type(), pt3.type());
    QCOMPARE(defaultConfig.state(), pt3.state());
    QCOMPARE(defaultConfig.purpose(), pt3.purpose());
}

void tst_QNetworkConfiguration::children()
{
    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> configs = manager.allConfigurations();

    foreach(QNetworkConfiguration c, configs)
    {
        if ( c.type() == QNetworkConfiguration::ServiceNetwork ) {
            qDebug() << "found service network" << c.name() << c.children().count();
            QVERIFY(c.isValid());
            QList<QNetworkConfiguration> members = c.children();
            foreach(QNetworkConfiguration child, members) {
                QVERIFY(child.isValid());
                QVERIFY(configs.contains(child));
                qDebug() << "\t" << child.name();
            }
        }
    }
}

void tst_QNetworkConfiguration::isRoamingAvailable()
{
    QNetworkConfigurationManager manager;
    QList<QNetworkConfiguration> configs = manager.allConfigurations();

    //force update to get maximum list
    QSignalSpy spy(&manager, SIGNAL(updateCompleted()));
    manager.updateConfigurations(); //initiate scans
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() == 1, TestTimeOut); //wait for scan to complete

    foreach(QNetworkConfiguration c, configs)
    {
        QVERIFY(QNetworkConfiguration::UserChoice != c.type());
        QVERIFY(QNetworkConfiguration::Invalid != c.type());
        if ( c.type() == QNetworkConfiguration::ServiceNetwork ) {
            //cannot test flag as some SNAPs may not support roaming anyway
            //QVERIFY(c.roamingavailable())
            if ( c.children().count() <= 1 )
                QVERIFY(!c.isRoamingAvailable());
            foreach(QNetworkConfiguration child, c.children()) {
                QCOMPARE(QNetworkConfiguration::InternetAccessPoint, child.type());
                QCOMPARE(child.children().count(), 0);
            }
        } else {
            QVERIFY(!c.isRoamingAvailable());
        }
    }
}
#endif

QTEST_MAIN(tst_QNetworkConfiguration)
#include "tst_qnetworkconfiguration.moc"
