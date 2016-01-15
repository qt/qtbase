/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
