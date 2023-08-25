// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "xbelwriter.h"
#include "xbelreader.h"

#include <QTreeWidget>

using namespace Qt::StringLiterals;

//! [0]
XbelWriter::XbelWriter(const QTreeWidget *treeWidget) : treeWidget(treeWidget)
{
    xml.setAutoFormatting(true);
}
//! [0]

//! [1]
bool XbelWriter::writeFile(QIODevice *device)
{
    xml.setDevice(device);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE xbel>"_L1);
    xml.writeStartElement("xbel"_L1);
    xml.writeAttribute("version"_L1, "1.0"_L1);
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i)
        writeItem(treeWidget->topLevelItem(i));

    xml.writeEndDocument();
    return true;
}
//! [1]

//! [2]
void XbelWriter::writeItem(const QTreeWidgetItem *item)
{
    QString tagName = item->data(0, Qt::UserRole).toString();
    if (tagName == "folder"_L1) {
        bool folded = !item->isExpanded();
        xml.writeStartElement(tagName);
        xml.writeAttribute("folded"_L1, folded ? "yes"_L1 : "no"_L1);
        xml.writeTextElement("title"_L1, item->text(0));
        for (int i = 0; i < item->childCount(); ++i)
            writeItem(item->child(i));
        xml.writeEndElement();
    } else if (tagName == "bookmark"_L1) {
        xml.writeStartElement(tagName);
        if (!item->text(1).isEmpty())
            xml.writeAttribute("href"_L1, item->text(1));
        xml.writeTextElement("title"_L1, item->text(0));
        xml.writeEndElement();
    } else if (tagName == "separator"_L1) {
        xml.writeEmptyElement(tagName);
    }
}
//! [2]
