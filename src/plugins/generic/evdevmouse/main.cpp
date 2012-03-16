/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qgenericplugin_qpa.h>
#include "qevdevmousemanager.h"

QT_BEGIN_NAMESPACE

class QEvdevMousePlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "evdevmouse.json")

public:
    QEvdevMousePlugin();

    QStringList keys() const;
    QObject* create(const QString &key, const QString &specification);
};

QEvdevMousePlugin::QEvdevMousePlugin()
    : QGenericPlugin()
{
}

QStringList QEvdevMousePlugin::keys() const
{
    return (QStringList()
            << QLatin1String("EvdevMouse"));
}

QObject* QEvdevMousePlugin::create(const QString &key,
                                   const QString &specification)
{
    if (!key.compare(QLatin1String("EvdevMouse"), Qt::CaseInsensitive))
        return new QEvdevMouseManager(key, specification);
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
