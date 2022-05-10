// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>

namespace qfontdatabase_snippets {
void wrapper()
{
//! [0]
QTreeWidget fontTree;
fontTree.setColumnCount(2);
fontTree.setHeaderLabels(QStringList() << "Font" << "Smooth Sizes");

const QStringList fontFamilies = QFontDatabase::families();
for (const QString &family : fontFamilies) {
    QTreeWidgetItem *familyItem = new QTreeWidgetItem(&fontTree);
    familyItem->setText(0, family);

    const QStringList fontStyles = QFontDatabase::styles(family);
    for (const QString &style : fontStyles) {
        QTreeWidgetItem *styleItem = new QTreeWidgetItem(familyItem);
        styleItem->setText(0, style);

        QString sizes;
        const QList<int> smoothSizes = QFontDatabase::smoothSizes(family, style);
        for (const auto &points : smoothSizes)
            sizes += QString::number(points) + ' ';

        styleItem->setText(1, sizes.trimmed());
    }
}
//! [0]
} // wrapper
} // qfontdatabase_snippets
