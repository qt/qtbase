// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XBELWRITER_H
#define XBELWRITER_H

#include <QXmlStreamWriter>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

//! [0]
class XbelWriter
{
public:
    explicit XbelWriter(const QTreeWidget *treeWidget);
    bool writeFile(QIODevice *device);

private:
    void writeItem(const QTreeWidgetItem *item);
    QXmlStreamWriter xml;
    const QTreeWidget *treeWidget;
};
//! [0]

#endif
