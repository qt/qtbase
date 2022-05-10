// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XBELREADER_H
#define XBELREADER_H

#include <QIcon>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

//! [0]
class XbelReader
{
public:
//! [1]
    XbelReader(QTreeWidget *treeWidget);
//! [1]

    bool read(QIODevice *device);

    QString errorString() const;

    static inline QString versionAttribute() { return QStringLiteral("version"); }
    static inline QString hrefAttribute() { return QStringLiteral("href"); }
    static inline QString foldedAttribute() { return QStringLiteral("folded"); }

private:
//! [2]
    void readXBEL();
    void readTitle(QTreeWidgetItem *item);
    void readSeparator(QTreeWidgetItem *item);
    void readFolder(QTreeWidgetItem *item);
    void readBookmark(QTreeWidgetItem *item);

    QTreeWidgetItem *createChildItem(QTreeWidgetItem *item);

    QXmlStreamReader xml;
    QTreeWidget *treeWidget;
//! [2]

    QIcon folderIcon;
    QIcon bookmarkIcon;
};
//! [0]

#endif
