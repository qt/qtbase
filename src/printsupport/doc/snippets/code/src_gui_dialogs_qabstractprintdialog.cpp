// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QPrintDialog printDialog(printer, parent);
if (printDialog.exec() == QDialog::Accepted) {
    // print ...
}
//! [0]
