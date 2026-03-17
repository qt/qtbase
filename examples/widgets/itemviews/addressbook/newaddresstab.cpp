// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "newaddresstab.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

//! [0]
NewAddressTab::NewAddressTab(QWidget *parent)
    : QWidget(parent)
{
    auto *descriptionLabel = new QLabel(tr("There are currently no contacts in your address book. "
                                           "\nClick Add to add new contacts."));

    auto *addButton = new QPushButton(tr("Add"));

    connect(addButton, &QAbstractButton::clicked, this, &NewAddressTab::triggered);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(descriptionLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(addButton, 0, Qt::AlignCenter);
}
//! [0]
