// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "informationwindow.h"

//! [0]
InformationWindow::InformationWindow(int id, QSqlRelationalTableModel *items,
                                     QWidget *parent)
    : QDialog(parent)
{
//! [0] //! [1]
    QLabel *itemLabel = new QLabel(tr("Item:"));
    QLabel *descriptionLabel = new QLabel(tr("Description:"));
    QLabel *imageFileLabel = new QLabel(tr("Image file:"));

    createButtons();

    itemText = new QLabel;
    descriptionEditor = new QTextEdit;
//! [1]

//! [2]
    imageFileEditor = new QComboBox;
    imageFileEditor->setModel(items->relationModel(1));
    imageFileEditor->setModelColumn(items->relationModel(1)->fieldIndex("file"));
//! [2]

//! [3]
    mapper = new QDataWidgetMapper(this);
    mapper->setModel(items);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setItemDelegate(new QSqlRelationalDelegate(mapper));
    mapper->addMapping(imageFileEditor, 1);
    mapper->addMapping(itemText, 2, "text");
    mapper->addMapping(descriptionEditor, 3);
    mapper->setCurrentIndex(id);
//! [3]

//! [4]
    connect(descriptionEditor, &QTextEdit::textChanged, this, [this]() { enableButtons(true); });
    connect(imageFileEditor, &QComboBox::currentIndexChanged, this, [this]() { enableButtons(true); });

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(itemLabel, itemText);
    formLayout->addRow(imageFileLabel, imageFileEditor);
    formLayout->addRow(descriptionLabel, descriptionEditor);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
    setLayout(layout);

    itemId = id;
    displayedImage = imageFileEditor->currentText();

    setWindowFlags(Qt::Window);
    enableButtons(false);
    setWindowTitle(itemText->text());
}
//! [4]

//! [5]
int InformationWindow::id() const
{
    return itemId;
}
//! [5]

//! [6]
void InformationWindow::revert()
{
    mapper->revert();
    enableButtons(false);
}
//! [6]

//! [7]
void InformationWindow::submit()
{
    QString newImage(imageFileEditor->currentText());

    if (displayedImage != newImage) {
        displayedImage = newImage;
        emit imageChanged(itemId, newImage);
    }

    mapper->submit();
    mapper->setCurrentIndex(itemId);

    enableButtons(false);
}
//! [7]

//! [8]
void InformationWindow::createButtons()
{
    closeButton = new QPushButton(tr("&Close"));
    revertButton = new QPushButton(tr("&Revert"));
    submitButton = new QPushButton(tr("&Submit"));

    closeButton->setDefault(true);

    connect(closeButton, &QPushButton::clicked, this, &InformationWindow::close);
    connect(revertButton, &QPushButton::clicked, this, &InformationWindow::revert);
    connect(submitButton, &QPushButton::clicked, this, &InformationWindow::submit);
//! [8]

//! [9]
    buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(submitButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(revertButton, QDialogButtonBox::ResetRole);
    buttonBox->addButton(closeButton, QDialogButtonBox::RejectRole);
}
//! [9]

//! [10]
void InformationWindow::enableButtons(bool enable)
{
    revertButton->setEnabled(enable);
    submitButton->setEnabled(enable);
}
//! [10]


