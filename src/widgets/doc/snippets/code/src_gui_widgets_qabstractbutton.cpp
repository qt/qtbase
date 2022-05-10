// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QPushButton *button = new QPushButton(tr("Ro&ck && Roll"), this);
//! [0]


//! [1]
button->setIcon(QIcon(":/images/print.png"));
button->setShortcut(tr("Alt+F7"));
//! [1]


//! [2]
void MyWidget::reactToToggle(bool checked)
{
   if (checked) {
      // Examine the new button states.
      ...
   }
}
//! [2]
