/****************************************************************************
**
** Copyright (C) 2009 Marco Martin <notmart@gmail.com>
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_SYSTEMTRAYICON

#include "qdbustraytypes_p.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QImage>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QtEndian>
#include <qpa/qplatformmenu.h>
#include "qdbusplatformmenu_p.h"

QT_BEGIN_NAMESPACE

static const int IconSizeLimit = 32;
static const int IconNormalSmallSize = 22;

QXdgDBusImageVector iconToQXdgDBusImageVector(const QIcon &icon)
{
    QXdgDBusImageVector ret;
    QList<QSize> sizes = icon.availableSizes();

    // Omit any size larger than 32 px, to save D-Bus bandwidth;
    // and ensure that 22px or smaller exists, because it's a common size.
    bool hasSmallIcon = false;
    QList<QSize> toRemove;
    Q_FOREACH (const QSize &size, sizes) {
        if (size.width() <= IconNormalSmallSize)
            hasSmallIcon = true;
        else if (size.width() > IconSizeLimit)
            toRemove << size;
    }
    Q_FOREACH (const QSize &size, toRemove)
        sizes.removeOne(size);
    if (!hasSmallIcon)
        sizes.append(QSize(IconNormalSmallSize, IconNormalSmallSize));

    foreach (QSize size, sizes) {
        QXdgDBusImageStruct kim;
        kim.width = size.width();
        kim.height = size.height();
        // Protocol specifies ARGB32 format in network byte order
        QImage im = icon.pixmap(size).toImage().convertToFormat(QImage::Format_ARGB32);
        kim.data = QByteArray((char *)(im.bits()), im.byteCount());
        if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
            for (char *ptr = kim.data.begin(); ptr < kim.data.end(); ptr += 4)
                qToUnaligned(qToBigEndian<quint32>(*ptr), reinterpret_cast<uchar *>(ptr));
        }
        ret << kim;
    }
    return ret;
}

// Marshall the ImageStruct data into a D-Bus argument
const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusImageStruct &icon)
{
    argument.beginStructure();
    argument << icon.width;
    argument << icon.height;
    argument << icon.data;
    argument.endStructure();
    return argument;
}

// Retrieve the ImageStruct data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusImageStruct &icon)
{
    qint32 width;
    qint32 height;
    QByteArray data;

    argument.beginStructure();
    argument >> width;
    argument >> height;
    argument >> data;
    argument.endStructure();

    icon.width = width;
    icon.height = height;
    icon.data = data;

    return argument;
}

// Marshall the ImageVector data into a D-Bus argument
const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusImageVector &iconVector)
{
    argument.beginArray(qMetaTypeId<QXdgDBusImageStruct>());
    for (int i = 0; i < iconVector.size(); ++i) {
        argument << iconVector[i];
    }
    argument.endArray();
    return argument;
}

// Retrieve the ImageVector data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusImageVector &iconVector)
{
    argument.beginArray();
    iconVector.clear();

    while (!argument.atEnd()) {
        QXdgDBusImageStruct element;
        argument >> element;
        iconVector.append(element);
    }

    argument.endArray();

    return argument;
}

// Marshall the ToolTipStruct data into a D-Bus argument
const QDBusArgument &operator<<(QDBusArgument &argument, const QXdgDBusToolTipStruct &toolTip)
{
    argument.beginStructure();
    argument << toolTip.icon;
    argument << toolTip.image;
    argument << toolTip.title;
    argument << toolTip.subTitle;
    argument.endStructure();
    return argument;
}

// Retrieve the ToolTipStruct data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, QXdgDBusToolTipStruct &toolTip)
{
    QString icon;
    QXdgDBusImageVector image;
    QString title;
    QString subTitle;

    argument.beginStructure();
    argument >> icon;
    argument >> image;
    argument >> title;
    argument >> subTitle;
    argument.endStructure();

    toolTip.icon = icon;
    toolTip.image = image;
    toolTip.title = title;
    toolTip.subTitle = subTitle;

    return argument;
}

QT_END_NAMESPACE
#endif // QT_NO_SYSTEMTRAYICON
