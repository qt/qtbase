// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [21]
qApp->setStyleSheet("QPushButton { color: white }");
//! [21]


//! [22]
myPushButton->setStyleSheet("* { color: blue }");
//! [22]


//! [23]
myPushButton->setStyleSheet("color: blue");
//! [23]


//! [24]
qApp->setStyleSheet("QGroupBox { color: red; } ");
//! [24]

//! [25]
qApp->setStyleSheet("QGroupBox, QGroupBox * { color: red; }");
//! [25]


//! [26]
class MyPushButton : public QPushButton {
    // ...
}

// ...
qApp->setStyleSheet("MyPushButton { background: yellow; }");
//! [26]


//! [27]
namespace ns {
    class MyPushButton : public QPushButton {
        // ...
    }
}

// ...
qApp->setStyleSheet("ns--MyPushButton { background: yellow; }");
//! [27]


//! [32]
void CustomWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
//! [32]


//! [88]
qApp->setStyleSheet("QLineEdit { background-color: yellow }");
//! [88]


//! [89]
myDialog->setStyleSheet("QLineEdit { background-color: yellow }");
//! [89]


//! [90]
myDialog->setStyleSheet("QLineEdit#nameEdit { background-color: yellow }");
//! [90]


//! [91]
nameEdit->setStyleSheet("background-color: yellow");
//! [91]


//! [92]
nameEdit->setStyleSheet("color: blue; background-color: yellow");
//! [92]


//! [93]
nameEdit->setStyleSheet("color: blue;"
                        "background-color: yellow;"
                        "selection-color: yellow;"
                        "selection-background-color: blue;");
//! [93]


//! [95]
QLineEdit *nameEdit = new QLineEdit(this);
nameEdit->setProperty("mandatoryField", true);

QLineEdit *emailEdit = new QLineEdit(this);
emailEdit->setProperty("mandatoryField", true);

QSpinBox *ageSpinBox = new QSpinBox(this);
ageSpinBox->setProperty("mandatoryField", true);
//! [95]

//! [96]
QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, true);
//! [96]
