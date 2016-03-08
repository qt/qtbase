/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
        error(QNetworkReply::ContentOperationNotPermittedError, msg);
    } else {
        headerRead(QNetworkRequest::LastModifiedHeader, QVariant::fromValue(fi.lastModified()));
        headerRead(QNetworkRequest::ContentLengthHeader, QVariant::fromValue(fi.size()));
        opened = QFile::open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        if (!opened) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend",
                "Error opening %1: %2").arg(fileName(), errorString());
            if (exists())
                error(QNetworkReply::ContentAccessDenied, msg);
            else
                error(QNetworkReply::ContentNotFoundError, msg);
        }
    }
    finished(opened);
}

void QNetworkFile::close()
{
    // This override is needed because 'using' keyword cannot be used for slots. And the base
    // function is not an invokable/slot function.
    QFile::close();
}

QT_END_NAMESPACE
