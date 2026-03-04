// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDataWidgetMapper>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include <QtCore/QRangeModel>
#include <QtCore/QStringListModel>

using namespace Qt::StringLiterals;

//! [Set up widgets]
Window::Window(QWidget *parent)
    : QWidget(parent),
      nameEdit(new QLineEdit),
      addressEdit(new QTextEdit),
      typeComboBox(new QComboBox),
      nextButton(new QPushButton(tr("&Next"))),
      previousButton(new QPushButton(tr("&Previous"))),
//! [Set up widgets]
//! [Set up the model]
      data{Person{u"Alice"_s,  u"<qt>123 Main Street<br/>Market Town</qt>"_s,                      u"0"_s},
           Person{u"Bob"_s,    u"<qt>PO Box 32<br/>Mail Handling Service<br/>Service City</qt>"_s, u"1"_s},
           Person{u"Carol"_s,  u"<qt>The Lighthouse<br/>Remote Island</qt>"_s,                     u"2"_s},
           Person{u"Donald"_s, u"<qt>47338 Park Avenue<br/>Big City</qt>"_s,                       u"0"_s},
           Person{u"Emma"_s,   u"<qt>Research Station<br/>Base Camp<br/>Big Mountain</qt>"_s,      u"2"_s}},
      model(new QRangeModel(data, this)),
      mapper(new QDataWidgetMapper(this))
//! [Set up the model]
{
//! [Set up type combo box]
    typeComboBox->setModel(new QStringListModel({ tr("Home"), tr("Work"), tr("Other") }, this));
//! [Set up type combo box]

//! [Set up the mapper]
    mapper->setModel(model);
    mapper->addMapping(nameEdit, 0);
    mapper->addMapping(addressEdit, 1);
    mapper->addMapping(typeComboBox, 2, "currentIndex");
//! [Set up the mapper]

//! [Set up connections and layouts]
    connect(previousButton, &QAbstractButton::clicked,
            mapper, &QDataWidgetMapper::toPrevious);
    connect(nextButton, &QAbstractButton::clicked,
            mapper, &QDataWidgetMapper::toNext);
    connect(mapper, &QDataWidgetMapper::currentIndexChanged,
            this, &Window::updateButtons);

    auto *formLayout = new QFormLayout;
    formLayout->addRow(tr("Na&me:"), nameEdit);
    formLayout->addRow(tr("&Address:"), addressEdit);
    formLayout->addRow(tr("&Type:"), typeComboBox);

    auto *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(previousButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addStretch();

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    setWindowTitle(tr("Delegate Widget Mapper"));
    mapper->toFirst();
}
//! [Set up connections and layouts]

//! [Slot for updating the buttons]
void Window::updateButtons(int row)
{
    previousButton->setEnabled(row > 0);
    nextButton->setEnabled(row < model->rowCount() - 1);
}
//! [Slot for updating the buttons]
