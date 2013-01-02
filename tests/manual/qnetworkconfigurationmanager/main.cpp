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
