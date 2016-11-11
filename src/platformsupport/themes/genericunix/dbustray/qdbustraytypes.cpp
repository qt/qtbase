/****************************************************************************
**
** Copyright (C) 2009 Marco Martin <notmart@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
#include <QPainter>
#include <QGuiApplication>
#include <qpa/qplatformmenu.h>
#include "qdbusplatformmenu_p.h"

QT_BEGIN_NAMESPACE

static const int IconSizeLimit = 64;
static const int IconNormalSmallSize = 22;
static const int IconNormalMediumSize = 64;

QXdgDBusImageVector iconToQXdgDBusImageVector(const QIcon &icon)
{
    QXdgDBusImageVector ret;
    QList<QSize> sizes = icon.availableSizes();

    // Omit any size larger than 64 px, to save D-Bus bandwidth;
    // ensure that 22px or smaller exists, because it's a common size;
    // and ensure that something between 22px and 64px exists, for better scaling to other sizes.
    bool hasSmallIcon = false;
    bool hasMediumIcon = false;
    qreal dpr = qGuiApp->devicePixelRatio();
    QList<QSize> toRemove;
    Q_FOREACH (const QSize &size, sizes) {
        int maxSize = qMax(size.width(), size.height());
        if (maxSize <= IconNormalSmallSize * dpr)
            hasSmallIcon = true;
        else if (maxSize <= IconNormalMediumSize * dpr)
            hasMediumIcon = true;
        else if (maxSize > IconSizeLimit * dpr)
            toRemove << size;
    }
    Q_FOREACH (const QSize &size, toRemove)
        sizes.removeOne(size);
    if (!hasSmallIcon)
        sizes.append(QSize(IconNormalSmallSize * dpr, IconNormalSmallSize * dpr));
    if (!hasMediumIcon)
        sizes.append(QSize(IconNormalMediumSize * dpr, IconNormalMediumSize * dpr));

    ret.reserve(sizes.size());
    foreach (QSize size, sizes) {
        // Protocol specifies ARGB32 format in network byte order
        QImage im = icon.pixmap(size).toImage().convertToFormat(QImage::Format_ARGB32);
        // letterbox if necessary to make it square
        if (im.height() != im.width()) {
            int maxSize = qMax(im.width(), im.height());
            QImage padded(maxSize, maxSize, QImage::Format_ARGB32);
            padded.fill(Qt::transparent);
            QPainter painter(&padded);
            painter.drawImage((maxSize - im.width()) / 2, (maxSize - im.height()) / 2, im);
            im = padded;
        }
        // copy and endian-convert
        QXdgDBusImageStruct kim(im.width(), im.height());
        const uchar *end = im.constBits() + im.sizeInBytes();
        uchar *dest = reinterpret_cast<uchar *>(kim.data.data());
        for (const uchar *src = im.constBits(); src < end; src += 4, dest += 4)
            qToUnaligned(qToBigEndian<quint32>(qFromUnaligned<quint32>(src)), dest);

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
