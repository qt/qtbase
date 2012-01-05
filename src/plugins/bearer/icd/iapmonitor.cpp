/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QStringList>

#include <conn_settings.h>
#include "iapmonitor.h"

namespace Maemo {


void conn_settings_notify_func (ConnSettingsType type,
                                const char *id,
                                const char *key,
                                ConnSettingsValue *value,
                                void *user_data);

class IAPMonitorPrivate {
private:
    IAPMonitor *monitor;
    ConnSettings *settings;

public:

    IAPMonitorPrivate(IAPMonitor *monitor)
        : monitor(monitor)
    {
        settings = conn_settings_open(CONN_SETTINGS_CONNECTION, NULL);
        conn_settings_add_notify(
                        settings,
                        (ConnSettingsNotifyFunc *)conn_settings_notify_func,
                        this);
    }

    ~IAPMonitorPrivate()
    {
        conn_settings_del_notify(settings);
        conn_settings_close(settings);
    }

    void iapAdded(const QString &iap)
    {
        monitor->iapAdded(iap);
    }

    void iapRemoved(const QString &iap)
    {
        monitor->iapRemoved(iap);
    }
};

void conn_settings_notify_func (ConnSettingsType type,
                                const char *id,
                                const char *key,
                                ConnSettingsValue *value,
                                void *user_data)
{
    Q_UNUSED(id);

    if (type != CONN_SETTINGS_CONNECTION) return;
    IAPMonitorPrivate *priv = (IAPMonitorPrivate *)user_data;

    QString iapId(key);
    iapId = iapId.split("/")[0];
    if (value != 0) {
        priv->iapAdded(iapId);
    } else if (iapId == QString(key)) {
        // IAP is removed only when the directory gets removed
        priv->iapRemoved(iapId);
    }
}

IAPMonitor::IAPMonitor()
    : d_ptr(new IAPMonitorPrivate(this))
{
}

IAPMonitor::~IAPMonitor()
{
    delete d_ptr;
}

void IAPMonitor::iapAdded(const QString &id)
{
    Q_UNUSED(id);
    // By default do nothing
}

void IAPMonitor::iapRemoved(const QString &id)
{
    Q_UNUSED(id);
    // By default do nothing
}

} // namespace Maemo
