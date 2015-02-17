/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "qpepperintegration.h"

#include <QtCore/QDebug>
#include <qpa/qplatformintegrationplugin.h>

QT_BEGIN_NAMESPACE

class QPepperIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPA.QPlatformIntegrationFactoryInterface.5.1" FILE
                          "pepper.json")
public:
    QStringList keys() const;
    QPlatformIntegration *create(const QString &, const QStringList &);
};

QStringList QPepperIntegrationPlugin::keys() const
{
    QStringList list;
    list << QStringLiteral("pepper");
    return list;
}

QPlatformIntegration *QPepperIntegrationPlugin::create(const QString &system,
                                                       const QStringList &paramList)
{
    Q_UNUSED(paramList);
    // qDebug() << "QPepperIntegrationPlugin::create" << system;
    if (system.toLower() == QStringLiteral("pepper"))
        return new QPepperIntegration;
    return 0;
}

QT_END_NAMESPACE

#include "qpepperpluginmain.moc"
