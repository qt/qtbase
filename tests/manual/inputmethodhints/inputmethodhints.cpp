// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inputmethodhints.h"

inputmethodhints::inputmethodhints(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    connect(ui.cbDialableOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbDigitsOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbEmailOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbFormattedNumbersOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbHiddenText, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbLowercaseOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbNoAutoUppercase, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbNoPredictiveText, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbPreferLowercase, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbPreferNumbers, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbPreferUpperCase, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbUppercaseOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
    connect(ui.cbUrlOnly, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
}

inputmethodhints::~inputmethodhints()
{

}

void inputmethodhints::checkboxChanged(int)
{
    int flags = 0;
    if (ui.cbDialableOnly->isChecked())
        flags |= Qt::ImhDialableCharactersOnly;
    if (ui.cbDigitsOnly->isChecked())
        flags |= Qt::ImhDigitsOnly;
    if (ui.cbEmailOnly->isChecked())
        flags |= Qt::ImhEmailCharactersOnly;
    if (ui.cbFormattedNumbersOnly->isChecked())
        flags |= Qt::ImhFormattedNumbersOnly;
    if (ui.cbHiddenText->isChecked())
        flags |= Qt::ImhHiddenText;
    if (ui.cbLowercaseOnly->isChecked())
        flags |= Qt::ImhLowercaseOnly;
    if (ui.cbNoAutoUppercase->isChecked())
        flags |= Qt::ImhNoAutoUppercase;
    if (ui.cbNoPredictiveText->isChecked())
        flags |= Qt::ImhNoPredictiveText;
    if (ui.cbPreferLowercase->isChecked())
        flags |= Qt::ImhPreferLowercase;
    if (ui.cbPreferNumbers->isChecked())
        flags |= Qt::ImhPreferNumbers;
    if (ui.cbPreferUpperCase->isChecked())
        flags |= Qt::ImhPreferUppercase;
    if (ui.cbUppercaseOnly->isChecked())
        flags |= Qt::ImhUppercaseOnly;
    if (ui.cbUrlOnly->isChecked())
        flags |= Qt::ImhUrlCharactersOnly;
    ui.lineEdit->clear();
    ui.lineEdit->setInputMethodHints(Qt::InputMethodHints(flags));
}
