/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
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
