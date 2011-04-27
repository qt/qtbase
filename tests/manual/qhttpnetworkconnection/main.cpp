/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
// This file contains benchmarks for QNetworkReply functions.

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include "../../auto/network-settings.h"

class tst_qhttpnetworkconnection : public QObject
{
    Q_OBJECT
private slots:
    void bigRemoteFile();

};

void tst_qhttpnetworkconnection::bigRemoteFile()
{
    QNetworkAccessManager manager;
    qint64 size;
    QTime t;
    QNetworkRequest request(QUrl("http://nds1.nokia.com/files/support/global/phones/software/Nokia_Ovi_Suite_webinstaller.exe"));
    QNetworkReply* reply = manager.get(request);
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
    qDebug() << "Starting download";
    t.start();
    QTestEventLoop::instance().enterLoop(50);
    QVERIFY(!QTestEventLoop::instance().timeout());
    size = reply->size();
    delete reply;
    qDebug() << "Finished!" << endl;
    qDebug() << "Time:" << t.elapsed() << "msec";
    qDebug() << "Bytes:" << size;
    qDebug() << "Speed:" <<  (size / 1024) / (t.elapsed() / 1000) << "KB/sec";
}

QTEST_MAIN(tst_qhttpnetworkconnection)

#include "main.moc"
