// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace src_gui_kernel_qclipboard {
void wrapper() {
QString newText;
QString image;
QClipboard::Mode mode = QClipboard::Mode::Clipboard;


//! [0]
QClipboard *clipboard = QGuiApplication::clipboard();
QString originalText = clipboard->text();
// etc.
clipboard->setText(newText);
//! [0]


//! [1]
QMimeData *data = new QMimeData;
data->setImageData(image);
clipboard->setMimeData(data, mode);
//! [1]


} // wrapper
} // src_gui_kernel_qclipboard
