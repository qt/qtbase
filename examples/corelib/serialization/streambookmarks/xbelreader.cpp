// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xbelreader.h"

#include <QStyle>
#include <QTreeWidget>

using namespace Qt::StringLiterals;

//! [0]
XbelReader::XbelReader(QTreeWidget *treeWidget) : treeWidget(treeWidget)
{
    QStyle *style = treeWidget->style();

    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon), QIcon::Normal,
                         QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon), QIcon::Normal, QIcon::On);
    bookmarkIcon.addPixmap(style->standardPixmap(QStyle::SP_FileIcon));
}
//! [0]

//! [1]
bool XbelReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == "xbel"_L1 && xml.attributes().value("version"_L1) == "1.0"_L1)
            readXBEL();
        else
            xml.raiseError(QObject::tr("The file is not an XBEL version 1.0 file."));
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
    Q_ASSERT(xml.isStartElement() && xml.name() == "xbel"_L1);

    while (xml.readNextStartElement()) {
        if (xml.name() == "folder"_L1)
            readFolder(nullptr);
        else if (xml.name() == "bookmark"_L1)
            readBookmark(nullptr);
        else if (xml.name() == "separator"_L1)
            readSeparator(nullptr);
        else
            xml.skipCurrentElement();
    }
}
//! [3]

//! [4]
void XbelReader::readBookmark(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "bookmark"_L1);

    QTreeWidgetItem *bookmark = createChildItem(item);
    bookmark->setFlags(bookmark->flags() | Qt::ItemIsEditable);
    bookmark->setIcon(0, bookmarkIcon);
    bookmark->setText(0, QObject::tr("Unknown title"));
    bookmark->setText(1, xml.attributes().value("href"_L1).toString());

    while (xml.readNextStartElement()) {
        if (xml.name() == "title"_L1)
            readTitle(bookmark);
        else
            xml.skipCurrentElement();
    }
}
//! [4]

//! [5]
void XbelReader::readTitle(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "title"_L1);
    item->setText(0, xml.readElementText());
}
//! [5]

//! [6]
void XbelReader::readSeparator(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "separator"_L1);
    constexpr char16_t midDot = u'\xB7';
    static const QString dots(30, midDot);

    QTreeWidgetItem *separator = createChildItem(item);
    separator->setFlags(item ? item->flags() & ~Qt::ItemIsSelectable : Qt::ItemFlags{});
    separator->setText(0, dots);
    xml.skipCurrentElement();
}
//! [6]

//! [7]
void XbelReader::readFolder(QTreeWidgetItem *item)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "folder"_L1);

    QTreeWidgetItem *folder = createChildItem(item);
    bool folded = xml.attributes().value("folded"_L1) != "no"_L1;
    folder->setExpanded(!folded);

    while (xml.readNextStartElement()) {
        if (xml.name() == "title"_L1)
            readTitle(folder);
        else if (xml.name() == "folder"_L1)
            readFolder(folder);
        else if (xml.name() == "bookmark"_L1)
            readBookmark(folder);
        else if (xml.name() == "separator"_L1)
            readSeparator(folder);
        else
            xml.skipCurrentElement();
    }
}
//! [7]

//! [8]
QTreeWidgetItem *XbelReader::createChildItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *childItem = item ? new QTreeWidgetItem(item) : new QTreeWidgetItem(treeWidget);
    childItem->setData(0, Qt::UserRole, xml.name().toString());
    return childItem;
}
//! [8]
