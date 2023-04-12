// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XBELTREE_H
#define XBELTREE_H

#include <QDomDocument>
#include <QIcon>
#include <QTreeWidget>

//! [0]
class XbelTree : public QTreeWidget
{
    Q_OBJECT

public:
    XbelTree(QWidget *parent = nullptr);

    bool read(QIODevice *device);
    bool write(QIODevice *device) const;

protected:
#if !defined(QT_NO_CONTEXTMENU) && !defined(QT_NO_CLIPBOARD)
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif

private slots:
    void updateDomElement(const QTreeWidgetItem *item, int column);

private:
    void parseFolderElement(const QDomElement &element,
                            QTreeWidgetItem *parentItem = nullptr);
    QTreeWidgetItem *createItem(const QDomElement &element,
                                QTreeWidgetItem *parentItem = nullptr);

    QDomDocument domDocument;
    QIcon folderIcon;
    QIcon bookmarkIcon;
};
//! [0]

#endif
