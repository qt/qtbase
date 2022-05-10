// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
bool ok;
QFont font = QFontDialog::getFont(
                &ok, QFont("Helvetica [Cronyx]", 10), this);
if (ok) {
    // the user clicked OK and font is set to the font the user selected
} else {
    // the user canceled the dialog; font is set to the initial
    // value, in this case Helvetica [Cronyx], 10
}
//! [0]


//! [1]
myWidget.setFont(QFontDialog::getFont(0, myWidget.font()));
//! [1]


//! [2]
bool ok;
QFont font = QFontDialog::getFont(&ok, QFont("Times", 12), this);
if (ok) {
    // font is set to the font the user selected
} else {
    // the user canceled the dialog; font is set to the initial
    // value, in this case Times, 12.
}
//! [2]


//! [3]
myWidget.setFont(QFontDialog::getFont(0, myWidget.font()));
//! [3]


//! [4]
bool ok;
QFont font = QFontDialog::getFont(&ok, this);
if (ok) {
    // font is set to the font the user selected
} else {
    // the user canceled the dialog; font is set to the default
    // application font, QApplication::font()
}
//! [4]
