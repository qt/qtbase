// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xbelreader.h"

#include <QStyle>
#include <QTreeWidget>

//! [0]
XbelReader::XbelReader(QTreeWidget *treeWidget)
    : treeWidget(treeWidget)
{
    QStyle *style = treeWidget->style();

    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);
    bookmarkIcon.addPixmap(style->standardPixmap(QStyle::SP_FileIcon));
}
//! [0]

//! [1]
bool XbelReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("xbel")
            && xml.attributes().value(versionAttribute()) == QLatin1String("1.0")) {
            readXBEL();
        } else {
            xml.raiseError(QObject::tr("The file is not an XBEL version 1.0 file."));
        }
    }

    return !xml.error();
}
//! [1]

//! [2]
QString XbelReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}
//! [2]

//! [3]
void XbelReader::readXBEL()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("xbel"));

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("folder"))
            readFolder(nullptr);
        else if (xml.name() == QLatin1String("bookmark"))
            readBookmark(nullptr);
        else if (xml.name() == QLatin1String("separator"))
            readSeparator(nullptr);
        else
            xml.skipCurrentElement();
    }
}
//! [3]

//! [4]
void XbelReader::readTitle(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("title"));

    QString title = xml.readElementText();
    item->setText(0, title);
}
//! [4]

//! [5]
void XbelReader::readSeparator(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("separator"));

    QTreeWidgetItem *separator = createChildItem(item);
    separator->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    separator->setText(0, QString(30, u'\xB7'));
    xml.skipCurrentElement();
}
//! [5]

void XbelReader::readFolder(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("folder"));

    QTreeWidgetItem *folder = createChildItem(item);
    bool folded = (xml.attributes().value(foldedAttribute()) != QLatin1String("no"));
    folder->setExpanded(!folded);

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("title"))
            readTitle(folder);
        else if (xml.name() == QLatin1String("folder"))
            readFolder(folder);
        else if (xml.name() == QLatin1String("bookmark"))
            readBookmark(folder);
        else if (xml.name() == QLatin1String("separator"))
            readSeparator(folder);
        else
            xml.skipCurrentElement();
    }
}

void XbelReader::readBookmark(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("bookmark"));

    QTreeWidgetItem *bookmark = createChildItem(item);
    bookmark->setFlags(bookmark->flags() | Qt::ItemIsEditable);
    bookmark->setIcon(0, bookmarkIcon);
    bookmark->setText(0, QObject::tr("Unknown title"));
    bookmark->setText(1, xml.attributes().value(hrefAttribute()).toString());

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("title"))
            readTitle(bookmark);
        else
            xml.skipCurrentElement();
    }
}

QTreeWidgetItem *XbelReader::createChildItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *childItem;
    if (item) {
        childItem = new QTreeWidgetItem(item);
    } else {
        childItem = new QTreeWidgetItem(treeWidget);
    }
    childItem->setData(0, Qt::UserRole, xml.name().toString());
    return childItem;
}
