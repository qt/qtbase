// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "colorlisteditor.h"

#include <QtWidgets>

ColorListEditor::ColorListEditor(QWidget *widget) : QComboBox(widget)
{
    populateList();
}

//! [0]
QColor ColorListEditor::color() const
{
    return qvariant_cast<QColor>(itemData(currentIndex(), Qt::DecorationRole));
}
//! [0]

//! [1]
void ColorListEditor::setColor(const QColor &color)
{
    setCurrentIndex(findData(color, Qt::DecorationRole));
}
//! [1]

//! [2]
void ColorListEditor::populateList()
{
    const QStringList colorNames = QColor::colorNames();

    for (int i = 0; i < colorNames.size(); ++i) {
        QColor color(colorNames[i]);

        insertItem(i, colorNames[i]);
        setItemData(i, color, Qt::DecorationRole);
    }
}
//! [2]
