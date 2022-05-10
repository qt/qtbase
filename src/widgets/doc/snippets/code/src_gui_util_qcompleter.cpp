// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
QStringList wordList;
wordList << "alpha" << "omega" << "omicron" << "zeta";

QLineEdit *lineEdit = new QLineEdit(this);

QCompleter *completer = new QCompleter(wordList, this);
completer->setCaseSensitivity(Qt::CaseInsensitive);
lineEdit->setCompleter(completer);
//! [0]


//! [1]
QCompleter *completer = new QCompleter(this);
completer->setModel(new QFileSystemModel(completer));
lineEdit->setCompleter(completer);
//! [1]


//! [2]
for (int i = 0; completer->setCurrentRow(i); i++)
    qDebug() << completer->currentCompletion() << " is match number " << i;
//! [2]
