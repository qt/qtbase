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

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkconfigmanager.h>

class tst_qnetworkconfigurationmanager : public QObject
{
    Q_OBJECT

private slots:
    void isOnline();
};

class SignalHandler : public QObject
{
    Q_OBJECT

public slots:
    void onOnlineStateChanged(bool isOnline)
    {
        qDebug() << "Online state changed to:" << isOnline;
    }
};

void tst_qnetworkconfigurationmanager::isOnline()
{
    QNetworkConfigurationManager manager;
    qDebug() << "Testing QNetworkConfigurationManager online status reporting functionality.";
    qDebug() << "This should tell the current online state:" << manager.isOnline();
    qDebug() << "Now please plug / unplug the network cable, and check the state update signal.";
    qDebug() << "Note that there might be some delays before you see the change, depending on the backend.";

    SignalHandler signalHandler;
    connect(&manager, SIGNAL(onlineStateChanged(bool)), &signalHandler, SLOT(onOnlineStateChanged(bool)));

    // event loop
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(QTestEventLoop::instance().timeout());
}

QTEST_MAIN(tst_qnetworkconfigurationmanager)

#include "main.moc"
