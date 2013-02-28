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

#include <QtNetwork/QNetworkAccessManager>
#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/QNetworkConfigurationManager>
#endif

#include <QtCore/QDebug>

#ifndef QT_NO_BEARERMANAGEMENT
Q_DECLARE_METATYPE(QNetworkAccessManager::NetworkAccessibility)
#endif

class tst_QNetworkAccessManager : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkAccessManager();

private slots:
    void networkAccessible();
};

tst_QNetworkAccessManager::tst_QNetworkAccessManager()
{
}

void tst_QNetworkAccessManager::networkAccessible()
{
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkAccessManager manager;

    qRegisterMetaType<QNetworkAccessManager::NetworkAccessibility>("QNetworkAccessManager::NetworkAccessibility");

    QSignalSpy spy(&manager,
                   SIGNAL(networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility)));

    // if there is no session, we cannot know in which state we are in
    QNetworkAccessManager::NetworkAccessibility initialAccessibility =
            manager.networkAccessible();
    QCOMPARE(manager.networkAccessible(), initialAccessibility);

    manager.setNetworkAccessible(QNetworkAccessManager::NotAccessible);

    int expectedCount = (initialAccessibility == QNetworkAccessManager::Accessible) ? 1 : 0;
    QCOMPARE(spy.count(), expectedCount);
    if (expectedCount > 0)
        QCOMPARE(spy.takeFirst().at(0).value<QNetworkAccessManager::NetworkAccessibility>(),
                 QNetworkAccessManager::NotAccessible);
    QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::NotAccessible);

    manager.setNetworkAccessible(QNetworkAccessManager::Accessible);

    QCOMPARE(spy.count(), expectedCount);
    if (expectedCount > 0)
        QCOMPARE(spy.takeFirst().at(0).value<QNetworkAccessManager::NetworkAccessibility>(),
                 initialAccessibility);
    QCOMPARE(manager.networkAccessible(), initialAccessibility);

    QNetworkConfigurationManager configManager;
    bool sessionRequired = (configManager.capabilities()
                            & QNetworkConfigurationManager::NetworkSessionRequired);
    QNetworkConfiguration defaultConfig = configManager.defaultConfiguration();
    if (defaultConfig.isValid()) {
        manager.setConfiguration(defaultConfig);

        // the accessibility has not changed if no session is required
        if (sessionRequired) {
            QCOMPARE(spy.count(), 1);
            QCOMPARE(spy.takeFirst().at(0).value<QNetworkAccessManager::NetworkAccessibility>(),
                     QNetworkAccessManager::Accessible);
        } else {
            QCOMPARE(spy.count(), 0);
        }
        QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::Accessible);

        manager.setNetworkAccessible(QNetworkAccessManager::NotAccessible);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(QNetworkAccessManager::NetworkAccessibility(spy.takeFirst().at(0).toInt()),
                 QNetworkAccessManager::NotAccessible);
        QCOMPARE(manager.networkAccessible(), QNetworkAccessManager::NotAccessible);
    }
#endif
}

QTEST_MAIN(tst_QNetworkAccessManager)
#include "tst_qnetworkaccessmanager.moc"
