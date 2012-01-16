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
#include "qconnmanengine.h"

#include <QtNetwork/private/qbearerplugin_p.h>

#include <QtCore/qdebug.h>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QConnmanEnginePlugin : public QBearerEnginePlugin
{
public:
    QConnmanEnginePlugin();
    ~QConnmanEnginePlugin();

    QStringList keys() const;
    QBearerEngine *create(const QString &key) const;
};

QConnmanEnginePlugin::QConnmanEnginePlugin()
{
}

QConnmanEnginePlugin::~QConnmanEnginePlugin()
{
}

QStringList QConnmanEnginePlugin::keys() const
{
    return QStringList() << QLatin1String("connman");
}

QBearerEngine *QConnmanEnginePlugin::create(const QString &key) const
{
    if (key == QLatin1String("connman")) {
        QConnmanEngine *engine = new QConnmanEngine;
        if (engine->connmanAvailable())
            return engine;
        else
            delete engine;
    }
    return 0;
}

Q_EXPORT_STATIC_PLUGIN(QConnmanEnginePlugin)
Q_EXPORT_PLUGIN2(qconnmanbearer, QConnmanEnginePlugin)

QT_END_NAMESPACE

#endif
#endif // QT_NO_BEARERMANAGEMENT
