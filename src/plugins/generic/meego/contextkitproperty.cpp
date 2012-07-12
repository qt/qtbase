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

#include "contextkitproperty.h"

#include <QDBusReply>
#include <QDebug>

static QString objectPathForProperty(const QString& property)
{
    QString path = property;
    if (!path.startsWith(QLatin1Char('/'))) {
        path.replace(QLatin1Char('.'), QLatin1Char('/'));
        path.prepend(QLatin1String("/org/maemo/contextkit/"));
    }
    return path;
}

QContextKitProperty::QContextKitProperty(const QString& serviceName, const QString& propertyName)
    : propertyInterface(serviceName, objectPathForProperty(propertyName),
                        QLatin1String("org.maemo.contextkit.Property"), QDBusConnection::systemBus())
{
    propertyInterface.call("Subscribe");
    connect(&propertyInterface, SIGNAL(ValueChanged(QVariantList,qulonglong)),
            this, SLOT(cacheValue(QVariantList,qulonglong)));

    QDBusMessage reply = propertyInterface.call("Get");
    if (reply.type() == QDBusMessage::ReplyMessage)
        cachedValue = qdbus_cast<QList<QVariant> >(reply.arguments().value(0)).value(0);
}

QContextKitProperty::~QContextKitProperty()
{
    propertyInterface.call("Unsubscribe");
}

QVariant QContextKitProperty::value() const
{
    return cachedValue;
}

void QContextKitProperty::cacheValue(const QVariantList& values, qulonglong)
{
    cachedValue = values.value(0);
    emit valueChanged(cachedValue);
}

