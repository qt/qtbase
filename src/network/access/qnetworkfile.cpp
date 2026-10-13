// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkfile_p.h"
#include "qnetworkrequest_p.h"

#include <QtCore/QDebug>
#include <QNetworkReply>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

QNetworkFile::QNetworkFile()
    : QFile()
{
}

QNetworkFile::QNetworkFile(const QString &name)
    : QFile(name)
{
}

void QNetworkFile::open()
{
    bool opened = false;
    QFileInfo fi(fileName());
    if (fi.isDir()) {
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend",
            "Cannot open %1: Path is a directory").arg(fileName());
        emit networkError(QNetworkReply::ContentOperationNotPermittedError, msg);
    } else {
        emit headerRead(QHttpHeaders::WellKnownHeader::LastModified,
                        QNetworkHeadersPrivate::toHttpDate(fi.lastModified()));
        emit headerRead(QHttpHeaders::WellKnownHeader::ContentLength,
                        QByteArray::number(fi.size()));
        opened = QFile::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!opened) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend",
                "Error opening %1: %2").arg(fileName(), errorString());
            if (exists())
                emit networkError(QNetworkReply::ContentAccessDenied, msg);
            else
                emit networkError(QNetworkReply::ContentNotFoundError, msg);
        }
    }
    emit finished(opened);
}

void QNetworkFile::close()
{
    // This override is needed because 'using' keyword cannot be used for slots. And the base
    // function is not an invokable/slot function.
    QFile::close();
}

QT_END_NAMESPACE

#include "moc_qnetworkfile_p.cpp"
