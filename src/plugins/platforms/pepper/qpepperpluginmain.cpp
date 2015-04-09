/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
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
    QPlatformIntegration *create(const QString &, const QStringList &) Q_DECL_OVERRIDE;
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
