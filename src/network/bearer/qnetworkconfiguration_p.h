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

#ifndef QNETWORKCONFIGURATIONPRIVATE_H
#define QNETWORKCONFIGURATIONPRIVATE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qnetworkconfiguration.h"

#include <QtCore/qshareddata.h>
#include <QtCore/qmutex.h>
#include <QtCore/qmap.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

typedef QExplicitlySharedDataPointer<QNetworkConfigurationPrivate> QNetworkConfigurationPrivatePointer;
class QNetworkConfigurationPrivate : public QSharedData
{
public:
    QNetworkConfigurationPrivate() :
        type(QNetworkConfiguration::Invalid),
        purpose(QNetworkConfiguration::UnknownPurpose),
        bearerType(QNetworkConfiguration::BearerUnknown),
        isValid(false), roamingSupported(false),
        timeout(DefaultTimeout)
    {}

    mutable QRecursiveMutex mutex;

    QString name;
    QString id;

    QNetworkConfiguration::StateFlags state;
    QNetworkConfiguration::Type type;
    QNetworkConfiguration::Purpose purpose;
    QNetworkConfiguration::BearerType bearerType;

    bool isValid;
    bool roamingSupported;
    int timeout;

    static Q_CONSTEXPR int DefaultTimeout = 30000;

private:
    Q_DISABLE_COPY_MOVE(QNetworkConfigurationPrivate)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QNetworkConfigurationPrivatePointer)
#endif
#endif // QNETWORKCONFIGURATIONPRIVATE_H
