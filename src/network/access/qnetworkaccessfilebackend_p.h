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

#ifndef QNETWORKACCESSFILEBACKEND_P_H
#define QNETWORKACCESSFILEBACKEND_P_H

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
#include "qnetworkaccessbackend_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "QtCore/qfile.h"

QT_BEGIN_NAMESPACE

class QNetworkAccessFileBackend: public QNetworkAccessBackend
{
    Q_OBJECT
public:
    QNetworkAccessFileBackend();
    virtual ~QNetworkAccessFileBackend();

    virtual void open() override;
    virtual void closeDownstreamChannel() override;

    virtual void downstreamReadyWrite() override;

public slots:
    void uploadReadyReadSlot();
private:
    QFile file;
    qint64 totalBytes;
    bool hasUploadFinished;

    bool loadFileInfo();
    bool readMoreFromFile();
};

class QNetworkAccessFileBackendFactory: public QNetworkAccessBackendFactory
{
public:
    virtual QStringList supportedSchemes() const override;
    virtual QNetworkAccessBackend *create(QNetworkAccessManager::Operation op,
                                          const QNetworkRequest &request) const override;
};

QT_END_NAMESPACE

#endif
