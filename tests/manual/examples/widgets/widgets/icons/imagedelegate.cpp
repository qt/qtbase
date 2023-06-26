// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagedelegate.h"
#include "iconpreviewarea.h"

#include <QComboBox>

//! [0]
ImageDelegate::ImageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}
//! [0]

//! [1]
QWidget *ImageDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem & /* option */,
                                     const QModelIndex &index) const
{
    QComboBox *comboBox = new QComboBox(parent);
    if (index.column() == 1)
        comboBox->addItems(IconPreviewArea::iconModeNames());
    else if (index.column() == 2)
        comboBox->addItems(IconPreviewArea::iconStateNames());

    connect(comboBox, &QComboBox::activated,
            this, &ImageDelegate::emitCommitData);

    return comboBox;
}
//! [1]

//! [2]
void ImageDelegate::setEditorData(QWidget *editor,
                                  const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
    if (!comboBox)
        return;

    int pos = comboBox->findText(index.model()->data(index).toString(),
                                 Qt::MatchExactly);
    comboBox->setCurrentIndex(pos);
}
//! [2]

//! [3]
void ImageDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
    if (!comboBox)
        return;

    model->setData(index, comboBox->currentText());
}
//! [3]

//! [4]
void ImageDelegate::emitCommitData()
{
    emit commitData(qobject_cast<QWidget *>(sender()));
}
//! [4]
