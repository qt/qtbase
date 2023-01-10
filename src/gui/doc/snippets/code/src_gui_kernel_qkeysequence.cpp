// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QKeySequence>
#include <QMenu>
#include <QTranslator>


namespace src_gui_kernel_qkeysequence {
struct Wrapper : public QWidget
{ void wrapper(); };

/* Wrap non-compilable code snippet

//! [0]
QKeySequence(QKeySequence::Print);
QKeySequence(tr("Ctrl+P"));
QKeySequence(tr("Ctrl+p"));
QKeySequence(Qt::CTRL | Qt::Key_P);
QKeySequence(Qt::CTRL + Qt::Key_P); // deprecated
//! [0]


//! [1]
QKeySequence(tr("Ctrl+X, Ctrl+C"));
QKeySequence(Qt::CTRL | Qt::Key_X, Qt::CTRL | Qt::Key_C);
QKeySequence(Qt::CTRL + Qt::Key_X, Qt::CTRL + Qt::Key_C); // deprecated
//! [1]

*/ // Wrap non-compilable code snippet

void Wrapper::wrapper() {
//! [2]
QMenu *file = new QMenu(this);
file->addAction(tr("&Open..."), QKeySequence(tr("Ctrl+O", "File|Open")),
                this, &MainWindow::open);
//! [2]

} // Wrapper::wrapper
} // src_gui_kernel_qkeysequence
