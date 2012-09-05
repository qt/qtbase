/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#include "cache_p.h"
#include "cache_adaptor.h"

#include "bridge_p.h"

#define QSPI_OBJECT_PATH_CACHE "/org/a11y/atspi/cache"

QT_BEGIN_NAMESPACE

/*!
    \class QSpiDBusCache
    \internal
    \brief This class is responsible for the AT-SPI cache interface.

    The idea behind the cache is that starting an application would
    result in many dbus calls. The way GTK/Gail/ATK work is that
    they create accessibles for all objects on startup.
    In order to avoid querying all the objects individually via DBus
    they get sent by using the GetItems call of the cache.

    Additionally the AddAccessible and RemoveAccessible signals
    are responsible for adding/removing objects from the cache.

    Currently the Qt bridge chooses to ignore these.
*/

QSpiDBusCache::QSpiDBusCache(QDBusConnection c, QObject* parent)
    : QObject(parent)
{
    new CacheAdaptor(this);
    c.registerObject(QLatin1String(QSPI_OBJECT_PATH_CACHE), this, QDBusConnection::ExportAdaptors);
}

void QSpiDBusCache::emitAddAccessible(const QSpiAccessibleCacheItem& item)
{
    emit AddAccessible(item);
}

void QSpiDBusCache::emitRemoveAccessible(const QSpiObjectReference& item)
{
    emit RemoveAccessible(item);
}

QSpiAccessibleCacheArray QSpiDBusCache::GetItems()
{
    QList <QSpiAccessibleCacheItem> cacheArray;
    return cacheArray;
}

QT_END_NAMESPACE
