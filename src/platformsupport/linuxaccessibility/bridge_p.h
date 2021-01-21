/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#ifndef QSPIACCESSIBLEBRIDGE_H
#define QSPIACCESSIBLEBRIDGE_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtDBus/qdbusconnection.h>
#include <qpa/qplatformaccessibility.h>

class DeviceEventControllerAdaptor;

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

class DBusConnection;
class QSpiDBusCache;
class AtSpiAdaptor;

class QSpiAccessibleBridge: public QObject, public QPlatformAccessibility
{
    Q_OBJECT
public:
    QSpiAccessibleBridge();

    virtual ~QSpiAccessibleBridge();

    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    QDBusConnection dBusConnection() const;

public Q_SLOTS:
    void enabledChanged(bool enabled);

private:
    void initializeConstantMappings();
    void updateStatus();

    QSpiDBusCache *cache;
    DeviceEventControllerAdaptor *dec;
    AtSpiAdaptor *dbusAdaptor;
    DBusConnection* dbusConnection;
};

QT_END_NAMESPACE

#endif
