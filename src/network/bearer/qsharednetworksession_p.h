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

#ifndef QSHAREDNETWORKSESSIONPRIVATE_H
#define QSHAREDNETWORKSESSIONPRIVATE_H

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
#include "qnetworksession.h"
#include "qnetworkconfiguration.h"
#include <QSharedPointer>
#include <QWeakPointer>
#include <QMutex>

#include <unordered_map>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

namespace QtPrivate {
struct NetworkConfigurationHash {
    using result_type = size_t;
    using argument_type = QNetworkConfiguration;
    size_t operator()(const QNetworkConfiguration &config) const noexcept
    {
        return std::hash<size_t>{}(size_t(config.type()) | (size_t(config.bearerType()) << 8) | (size_t(config.purpose()) << 16));
    }
};
}

class QSharedNetworkSessionManager
{
public:
    static QSharedPointer<QNetworkSession> getSession(const QNetworkConfiguration &config);
    static void setSession(const QNetworkConfiguration &config, QSharedPointer<QNetworkSession> session);
private:
    std::unordered_map<QNetworkConfiguration, QWeakPointer<QNetworkSession>, QtPrivate::NetworkConfigurationHash> sessions;
};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif //QSHAREDNETWORKSESSIONPRIVATE_H

