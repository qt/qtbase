// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
// This file contains benchmarks for QNetworkReply functions.

#include <QDebug>
#include <qtest.h>
#include <QTest>
#include <QtTest/qtesteventloop.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtCore/QElapsedTimer>

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
    QElapsedTimer t;
    QNetworkRequest request(QUrl(QString::fromLatin1(urlC)));
    QNetworkReply* reply = manager.get(request);
    connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()), Qt::QueuedConnection);
    qDebug() << "Starting download" << urlC;
    t.start();
    QTestEventLoop::instance().enterLoop(50);
    QVERIFY(!QTestEventLoop::instance().timeout());
    size = reply->size();
    delete reply;
    qDebug() << "Finished!" << Qt::endl;
    qDebug() << "Time:" << t.elapsed() << "msec";
    qDebug() << "Bytes:" << size;
    qDebug() << "Speed:" <<  (size / qreal(1024)) / (t.elapsed() / qreal(1000)) << "KB/sec";
}

QTEST_MAIN(tst_qhttpnetworkconnection)

#include "main.moc"
