/****************************************************************************
**
** Copyright (C) 2013 Research in Motion.
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

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkconfiguration.h>
#include <QtNetwork/qnetworkconfigmanager.h>

class tst_qnetworkconfiguration : public QObject
{
    Q_OBJECT

private slots:
    void bearerType();
    void bearerTypeFamily();
};

void tst_qnetworkconfiguration::bearerType()
{
    QNetworkConfigurationManager m;
    QList<QNetworkConfiguration> allConfs = m.allConfigurations();
    QElapsedTimer timer;
    for (int a = 0; a < allConfs.count(); a++) {
        timer.start();
        QNetworkConfiguration::BearerType type = allConfs.at(a).bearerType();
        qint64 elapsed = timer.elapsed();
        QString typeString;
        switch (type) {
        case QNetworkConfiguration::BearerUnknown:
            typeString = QLatin1String("Unknown");
            break;
        case QNetworkConfiguration::BearerEthernet:
            typeString = QLatin1String("Ethernet");
            break;
        case QNetworkConfiguration::BearerWLAN:
            typeString = QLatin1String("WLAN");
            break;
        case QNetworkConfiguration::Bearer2G:
            typeString = QLatin1String("2G");
            break;
        case QNetworkConfiguration::BearerCDMA2000:
            typeString = QLatin1String("CDMA2000");
            break;
        case QNetworkConfiguration::BearerWCDMA:
            typeString = QLatin1String("WCDMA");
            break;
        case QNetworkConfiguration::BearerHSPA:
            typeString = QLatin1String("HSPA");
            break;
        case QNetworkConfiguration::BearerBluetooth:
            typeString = QLatin1String("Bluetooth");
            break;
        case QNetworkConfiguration::BearerWiMAX:
            typeString = QLatin1String("WiMAX");
            break;
        case QNetworkConfiguration::BearerEVDO:
            typeString = QLatin1String("EVDO");
            break;
        case QNetworkConfiguration::BearerLTE:
            typeString = QLatin1String("LTE");
            break;
        default:
            typeString = "unknown bearer (?)";
        }

        const char *isDefault = (allConfs.at(a) == m.defaultConfiguration())
                ? "*DEFAULT*" : "";
        qDebug() << isDefault << "identifier:" << allConfs.at(a).identifier()
                 << "bearer type name:" << allConfs.at(a).bearerTypeName()
                 << "bearer type:" << type << "(" << typeString << ")"
                 << "elapsed:" << elapsed;
        QCOMPARE(allConfs.at(a).bearerTypeName(), typeString);
    }
}

void tst_qnetworkconfiguration::bearerTypeFamily()
{
    QNetworkConfigurationManager m;
    foreach (const QNetworkConfiguration &config,
             m.allConfigurations(QNetworkConfiguration::Active)) {
        QString family;
        switch (config.bearerTypeFamily()) {
        case QNetworkConfiguration::Bearer3G:
            family = QLatin1String("Bearer3G");
            break;
        case QNetworkConfiguration::Bearer4G:
            family = QLatin1String("Bearer4G");
            break;
        default:
            family = config.bearerTypeName();
        }
        qDebug() << config.name() << "has bearer type"
                    << config.bearerTypeName() << "of bearer type family"
                    << family;
    }
}

QTEST_MAIN(tst_qnetworkconfiguration)

#include "main.moc"
