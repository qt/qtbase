// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QNetworkAccessManager *manager = new QNetworkAccessManager(this);
QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
QString directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QLatin1StringView("/cacheDir/");
diskCache->setCacheDirectory(directory);
manager->setCache(diskCache);
//! [0]

//! [1]
using namespace Qt::StringLiterals;
// do a normal request (preferred from network, as this is the default)
QNetworkRequest request(QUrl(u"http://qt-project.org"_s));
manager->get(request);

// do a request preferred from cache
QNetworkRequest request2(QUrl(u"http://qt-project.org"_s));
request2.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
manager->get(request2);
//! [1]

//! [2]
void replyFinished(QNetworkReply *reply) {
    QVariant fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute);
    qDebug() << "page from cache?" << fromCache.toBool();
}
//! [2]
