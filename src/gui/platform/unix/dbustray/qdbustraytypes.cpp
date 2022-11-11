// Copyright (C) 2009 Marco Martin <notmart@gmail.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QT_NO_SYSTEMTRAYICON

#include "qdbustraytypes_p.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QImage>
#include <QIcon>
#include <QIconEngine>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QtEndian>
#include <QPainter>
#include <QGuiApplication>
#include <qpa/qplatformmenu.h>
#include <private/qdbusplatformmenu_p.h>
#include <private/qicon_p.h>

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN(QXdgDBusImageStruct)
QT_IMPL_METATYPE_EXTERN(QXdgDBusImageVector)
QT_IMPL_METATYPE_EXTERN(QXdgDBusToolTipStruct)

static const int IconSizeLimit = 64;
static const int IconNormalSmallSize = 22;
static const int IconNormalMediumSize = 64;

QXdgDBusImageVector iconToQXdgDBusImageVector(const QIcon &icon)
{
    QXdgDBusImageVector ret;
    if (icon.isNull())
        return ret;
    QIconEngine *engine = const_cast<QIcon &>(icon).data_ptr()->engine;
    QList<QSize> sizes = engine->availableSizes(QIcon::Normal, QIcon::Off);

    // Omit any size larger than 64 px, to save D-Bus bandwidth;
    // ensure that 22px or smaller exists, because it's a common size;
    // and ensure that something between 22px and 64px exists, for better scaling to other sizes.
    bool hasSmallIcon = false;
    bool hasMediumIcon = false;
    QList<QSize> toRemove;
    for (const QSize &size : std::as_const(sizes)) {
        int maxSize = qMax(size.width(), size.height());
        if (maxSize <= IconNormalSmallSize)
            hasSmallIcon = true;
        else if (maxSize <= IconNormalMediumSize)
            hasMediumIcon = true;
        else if (maxSize > IconSizeLimit)
            toRemove << size;
    }
    for (const QSize &size : std::as_const(toRemove))
        sizes.removeOne(size);
    if (!hasSmallIcon)
        sizes.append(QSize(IconNormalSmallSize, IconNormalSmallSize));
    if (!hasMediumIcon)
        sizes.append(QSize(IconNormalMediumSize, IconNormalMediumSize));

    ret.reserve(sizes.size());
    for (const QSize &size : std::as_const(sizes)) {
        // Protocol specifies ARGB32 format in network byte order
        QImage im = engine->pixmap(size, QIcon::Normal, QIcon::Off).toImage().convertToFormat(QImage::Format_ARGB32);
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
        qToBigEndian<quint32>(im.constBits(), im.width() * im.height(), kim.data.data());

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
