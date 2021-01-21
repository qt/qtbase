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

#ifndef QBEARERENGINE_P_H
#define QBEARERENGINE_P_H

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
#include "qnetworkconfiguration_p.h"
#include "qnetworksession.h"
#include "qnetworkconfigmanager.h"

#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qmutex.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfiguration;

class Q_NETWORK_EXPORT QBearerEngine : public QObject
{
    Q_OBJECT

    friend class QNetworkConfigurationManagerPrivate;

public:
    explicit QBearerEngine(QObject *parent = nullptr);
    virtual ~QBearerEngine();

    virtual bool hasIdentifier(const QString &id) = 0;

    virtual QNetworkConfigurationManager::Capabilities capabilities() const = 0;

    virtual QNetworkSessionPrivate *createSessionBackend() = 0;

    virtual QNetworkConfigurationPrivatePointer defaultConfiguration() = 0;

    virtual bool requiresPolling() const;
    bool configurationsInUse() const;

Q_SIGNALS:
    void configurationAdded(QNetworkConfigurationPrivatePointer config);
    void configurationRemoved(QNetworkConfigurationPrivatePointer config);
    void configurationChanged(QNetworkConfigurationPrivatePointer config);
    void updateCompleted();

protected:
    //this table contains an up to date list of all configs at any time.
    //it must be updated if configurations change, are added/removed or
    //the members of ServiceNetworks change
    QHash<QString, QNetworkConfigurationPrivatePointer> accessPointConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer> snapConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer> userChoiceConfigurations;

    mutable QRecursiveMutex mutex;
};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif // QBEARERENGINE_P_H
