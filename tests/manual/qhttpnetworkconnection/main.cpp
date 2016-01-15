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
// This file contains benchmarks for QNetworkReply functions.

#include <QDebug>
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>

class tst_qhttpnetworkconnection : public QObject
{
    Q_OBJECT
private slots:
    void bigRemoteFile();

};

const char urlC[] = "http://download.qt-project.org/official_releases/online_installers/qt-linux-opensource-1.4.0-x86-online.run";

void tst_qhttpnetworkconnection::bigRemoteFile()
{
    QNetworkAccessManager manager;
    qint64 size;
    QTime t;
    QNetworkRequest request(QUrl(QString::fromLatin1(urlC)));
    QNetworkReply* reply = manager.get(request);
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
    qDebug() << "Starting download" << urlC;
    t.start();
    QTestEventLoop::instance().enterLoop(50);
    QVERIFY(!QTestEventLoop::instance().timeout());
    size = reply->size();
    delete reply;
    qDebug() << "Finished!" << endl;
    qDebug() << "Time:" << t.elapsed() << "msec";
    qDebug() << "Bytes:" << size;
    qDebug() << "Speed:" <<  (size / qreal(1024)) / (t.elapsed() / qreal(1000)) << "KB/sec";
}

QTEST_MAIN(tst_qhttpnetworkconnection)

#include "main.moc"
