/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

//#define QNETWORKACCESSCACHEBACKEND_DEBUG

#include "qnetworkaccesscachebackend_p.h"
#include "qabstractnetworkcache.h"
#include "qfileinfo.h"
#if QT_CONFIG(ftp)
#include "qurlinfo_p.h"
#endif
#include "qdir.h"
#include "qcoreapplication.h"

QT_BEGIN_NAMESPACE

QNetworkAccessCacheBackend::QNetworkAccessCacheBackend()
    : QNetworkAccessBackend()
{
}

QNetworkAccessCacheBackend::~QNetworkAccessCacheBackend()
{
}

void QNetworkAccessCacheBackend::open()
{
    if (operation() != QNetworkAccessManager::GetOperation || !sendCacheContents()) {
        QString msg = QCoreApplication::translate("QNetworkAccessCacheBackend", "Error opening %1")
                                                .arg(this->url().toString());
        error(QNetworkReply::ContentNotFoundError, msg);
    } else {
        setAttribute(QNetworkRequest::SourceIsFromCacheAttribute, true);
    }
    finished();
}

bool QNetworkAccessCacheBackend::sendCacheContents()
{
    setCachingEnabled(false);
    QAbstractNetworkCache *nc = networkCache();
    if (!nc)
        return false;

    QNetworkCacheMetaData item = nc->metaData(url());
    if (!item.isValid())
        return false;

    QNetworkCacheMetaData::AttributesMap attributes = item.attributes();
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, attributes.value(QNetworkRequest::HttpStatusCodeAttribute));
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, attributes.value(QNetworkRequest::HttpReasonPhraseAttribute));

    // set the raw headers
    const QNetworkCacheMetaData::RawHeaderList rawHeaders = item.rawHeaders();
    for (const auto &header : rawHeaders) {
        if (header.first.toLower() == "cache-control") {
            const QByteArray cacheControlValue = header.second.toLower();
            if (cacheControlValue.contains("must-revalidate")
                || cacheControlValue.contains("no-cache")) {
                return false;
            }
        }
        setRawHeader(header.first, header.second);
    }

    // handle a possible redirect
    QVariant redirectionTarget = attributes.value(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectionTarget.isValid()) {
        setAttribute(QNetworkRequest::RedirectionTargetAttribute, redirectionTarget);
        redirectionRequested(redirectionTarget.toUrl());
    }

    // signal we're open
    metaDataChanged();

    if (operation() == QNetworkAccessManager::GetOperation) {
        QIODevice *contents = nc->data(url());
        if (!contents)
            return false;
        contents->setParent(this);
        writeDownstreamData(contents);
    }

#if defined(QNETWORKACCESSCACHEBACKEND_DEBUG)
    qDebug() << "Successfully sent cache:" << url();
#endif
    return true;
}

bool QNetworkAccessCacheBackend::start()
{
    open();
    return true;
}

void QNetworkAccessCacheBackend::closeDownstreamChannel()
{
}

void QNetworkAccessCacheBackend::closeUpstreamChannel()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

void QNetworkAccessCacheBackend::upstreamReadyRead()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

void QNetworkAccessCacheBackend::downstreamReadyWrite()
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "This function show not have been called!");
}

QT_END_NAMESPACE

