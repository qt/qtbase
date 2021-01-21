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

#ifndef QNETWORKFILE_H
#define QNETWORKFILE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QFile>
#include <qnetworkreply.h>

QT_BEGIN_NAMESPACE

class QNetworkFile : public QFile
{
    Q_OBJECT
public:
    QNetworkFile();
    QNetworkFile(const QString &name);
    using QFile::open;

public Q_SLOTS:
    void open();
    void close() override;

Q_SIGNALS:
    void finished(bool ok);
    void headerRead(QNetworkRequest::KnownHeaders header, const QVariant &value);
    void error(QNetworkReply::NetworkError error, const QString &message);
};

QT_END_NAMESPACE

#endif // QNETWORKFILE_H
