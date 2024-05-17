// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <QtCore/QDebug>

class tst_QNetworkAccessManager : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkAccessManager();

private slots:
    void alwaysCacheRequest();
};

tst_QNetworkAccessManager::tst_QNetworkAccessManager()
{
}

void tst_QNetworkAccessManager::alwaysCacheRequest()
{
    QNetworkAccessManager manager;

    QNetworkRequest req;
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysCache);
    QNetworkReply *reply = manager.get(req);
    reply->close();
    delete reply;
}

QTEST_MAIN(tst_QNetworkAccessManager)
#include "tst_qnetworkaccessmanager.moc"
