// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
shortcut = new QShortcut(QKeySequence(tr("Ctrl+O", "File|Open")),
                         parent);
//! [0]


//! [1]
setKey(0);                  // no signal emitted
setKey(QKeySequence());     // no signal emitted
setKey(0x3b1);              // Greek letter alpha
setKey(Qt::Key_D);              // 'd', e.g. to delete
setKey('q');                // 'q', e.g. to quit
setKey(Qt::CTRL | Qt::Key_P);       // Ctrl+P, e.g. to print document
setKey("Ctrl+P");           // Ctrl+P, e.g. to print document
//! [1]
