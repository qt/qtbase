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

#include "qnetworkfile_p.h"

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
        emit error(QNetworkReply::ContentOperationNotPermittedError, msg);
    } else {
        emit headerRead(QNetworkRequest::LastModifiedHeader, QVariant::fromValue(fi.lastModified()));
        emit headerRead(QNetworkRequest::ContentLengthHeader, QVariant::fromValue(fi.size()));
        opened = QFile::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!opened) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend",
                "Error opening %1: %2").arg(fileName(), errorString());
            if (exists())
                emit error(QNetworkReply::ContentAccessDenied, msg);
            else
                emit error(QNetworkReply::ContentNotFoundError, msg);
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
