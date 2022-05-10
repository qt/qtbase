// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QDataWidgetMapper *mapper = new QDataWidgetMapper;
mapper->setModel(model);
mapper->addMapping(mySpinBox, 0);
mapper->addMapping(myLineEdit, 1);
mapper->addMapping(myCountryChooser, 2);
mapper->toFirst();
//! [0]


//! [1]
QDataWidgetMapper *mapper = new QDataWidgetMapper;
mapper->setModel(myModel);
mapper->addMapping(nameLineEdit, 0);
mapper->addMapping(ageSpinBox, 1);
//! [1]


//! [2]
QDataWidgetMapper *mapper = new QDataWidgetMapper;
connect(myTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
        mapper, &QDataWidgetMapper::setCurrentModelIndex);
//! [2]
