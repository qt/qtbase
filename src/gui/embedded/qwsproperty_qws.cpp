/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwsproperty_qws.h"

#ifndef QT_NO_QWS_PROPERTIES
#include "qwscommand_qws_p.h"
#include "qwindowsystem_qws.h"
#include "qhash.h"
#include "qalgorithms.h"
#include "qbytearray.h"

#include <stdio.h>

QT_BEGIN_NAMESPACE

class QWSPropertyManager::Data {
public:
    QByteArray find(int winId, int property)
    {
        return properties.value(winId).value(property);
    }

    typedef QHash<int, QHash<int, QByteArray> > PropertyHash;
    PropertyHash properties;
};

/*********************************************************************
 *
 * Class: QWSPropertyManager
 *
 *********************************************************************/

QWSPropertyManager::QWSPropertyManager()
{
    d = new Data;
}

QWSPropertyManager::~QWSPropertyManager()
{
    delete d;
}

bool QWSPropertyManager::setProperty(int winId, int property, int mode, const char *data, int len)
{
    QHash<int, QByteArray> props = d->properties.value(winId);
    QHash<int, QByteArray>::iterator it = props.find(property);
    if (it == props.end())
        return false;

    switch (mode) {
    case PropReplace:
        d->properties[winId][property] = QByteArray(data, len);
        break;
    case PropAppend:
        d->properties[winId][property].append(data);
        break;
    case PropPrepend:
        d->properties[winId][property].prepend(data);
        break;
    }
    return true;
}

bool QWSPropertyManager::hasProperty(int winId, int property)
{
    return d->properties.value(winId).contains(property);
}

bool QWSPropertyManager::removeProperty(int winId, int property)
{
    QWSPropertyManager::Data::PropertyHash::iterator it = d->properties.find(winId);
    if (it == d->properties.end())
        return false;
    return d->properties[winId].remove( property );
}

bool QWSPropertyManager::addProperty(int winId, int property)
{
    if( !d->properties[winId].contains(property) )
	d->properties[winId][property] = QByteArray(); // only add if it doesn't exist
    return true;
}

bool QWSPropertyManager::getProperty(int winId, int property, const char *&data, int &len)
{
    QHash<int, QByteArray> props = d->properties.value(winId);
    QHash<int, QByteArray>::iterator it = props.find(property);
    if (it == props.end()) {
        data = 0;
        len = -1;
        return false;
    }
    data = it.value().constData();
    len = it.value().length();

    return true;
}

bool QWSPropertyManager::removeProperties(int winId)
{
    return d->properties.remove(winId);
}

QT_END_NAMESPACE

#endif //QT_NO_QWS_PROPERTIES
